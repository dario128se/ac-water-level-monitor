/*
 * AC Water Level Monitor
 * ========================
 * Sistema de monitoreo de nivel de agua para depósito de descarga
 * de aire acondicionado con control automático de bomba de achique.
 *
 * Hardware:
 * - ESP32-WROOM-32
 * - 7 sensores de boya magnética (NA)
 * - Display TFT ILI9341 2.4"
 * - Bomba 12V tipo "sapito"
 * - Buzzer activo 5V
 *
 * Autor: AC Monitor Project
 * Versión: 1.0
 */

#include "alarm.h"
#include "config.h"
#include "display.h"
#include "mqtt.h"
#include "pump.h"
#include "sensors.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft; // Declarado en display.cpp

// Estados de la máquina de estados principal
enum SystemState {
  STATE_INIT,    // Inicialización
  STATE_IDLE,    // Esperando llenado
  STATE_FILLING, // Llenándose
  STATE_PUMPING, // Bomba activa (vaciando)
  STATE_ERROR    // Error de secuencia
};

// Variables globales de estado
SystemState systemState = STATE_INIT;
SensorState sensorState;
PumpStatus pumpStatus;
AlarmState alarmState;
DisplayData displayData;

// Timers
unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastMqttPublish = 0;
unsigned long fillStartTime = 0;

// Reset button
unsigned long buttonPressStart = 0;
bool buttonWasPressed = false;
#define BUTTON_HOLD_TIME_MS 2000 // Mantener 2 segundos para reset

// Demo mode
bool demoMode = false;
unsigned long lastDemoUpdate = 0;
int demoLevel = 0;
bool demoFilling = true;
#define DEMO_SPEED_MS 800 // Velocidad de simulación

// Prototipos
void updateStateMachine();
void updateDisplay();
void publishMqtt();
void checkResetButton();
void updateDemoMode();
const char *getPumpStateString(PumpState state);
const char *getSequenceStateString(SequenceState state);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n================================");
  Serial.println("   AC Water Level Monitor v1.0");
  Serial.println("================================\n");

  // Inicializar display primero para mostrar splash
  display_init();
  display_splash();

  // Botón de reset (GPIO 0 tiene pull-up interno)
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  // Mostrar instrucciones para modo demo durante el splash
  Serial.println("[INFO] Hold BOOT button now for DEMO MODE...");
  delay(2000);

  // Detectar modo demo: si BOOT está presionado AL FINAL del splash
  // (después de que el ESP ya arrancó normalmente)
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    demoMode = true;
    Serial.println("[DEMO] Demo mode activated!");

    // Mostrar mensaje en display
    tft.fillScreen(0x001F); // Fondo azul
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0xFFFF, 0x001F);
    tft.setTextFont(4);
    tft.drawString("MODO DEMO", 120, 140);
    tft.setTextFont(2);
    tft.drawString("Simulacion activa", 120, 180);
    tft.setTextDatum(TL_DATUM);
    delay(1500);
  }

  // Inicializar módulos
  sensors_init();
  pump_init();
  alarm_init();

  // Inicializar estructuras
  memset(&sensorState, 0, sizeof(sensorState));
  memset(&pumpStatus, 0, sizeof(pumpStatus));
  memset(&alarmState, 0, sizeof(alarmState));
  memset(&displayData, 0, sizeof(displayData));

  // Si es modo demo, inicializar estado
  if (demoMode) {
    sensorState.sequenceError = false;
    sensorState.currentLevel = 0;
    sensorState.sequenceState = SEQ_IDLE;
  }

// Inicializar MQTT (opcional)
#if MQTT_ENABLED
  if (mqtt_init()) {
    displayData.wifiConnected = true;
  }
#endif

  // Forzar redibujado inicial
  display_force_redraw();

  systemState = STATE_IDLE;
  Serial.println("[MAIN] System initialized - entering IDLE state\n");
}

void loop() {
  unsigned long currentTime = millis();

  // 0. Verificar botón de reset
  checkResetButton();

  // MODO DEMO: Simular en lugar de leer sensores reales
  if (demoMode) {
    updateDemoMode();
  } else {
    // 1. Leer sensores periódicamente (modo normal)
    if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
      lastSensorRead = currentTime;
      sensors_read(&sensorState);
      sensors_validate_sequence(&sensorState);
    }

    // 2. Actualizar máquina de estados
    updateStateMachine();

    // 3. Actualizar bomba (tiempos)
    pump_update(&pumpStatus);

    // 4. Actualizar alarma (patrones de sonido)
    alarm_update(&alarmState);
  }

  // 5. Actualizar display periódicamente
  if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
    lastDisplayUpdate = currentTime;
    updateDisplay();
  }

// 6. Publicar MQTT periódicamente
#if MQTT_ENABLED
  mqtt_loop();
  if (currentTime - lastMqttPublish >= MQTT_PUBLISH_INTERVAL_MS) {
    lastMqttPublish = currentTime;
    publishMqtt();
  }
#endif
}

void updateStateMachine() {
  SystemState previousState = systemState;

  switch (systemState) {
  case STATE_INIT:
    // No debería llegar aquí después del setup
    systemState = STATE_IDLE;
    break;

  case STATE_IDLE:
    // Esperando que empiece a llenarse
    if (sensorState.sequenceError) {
      systemState = STATE_ERROR;
    } else if (sensorState.currentLevel > 0) {
      systemState = STATE_FILLING;
      fillStartTime = millis();
      Serial.println("[MAIN] Water detected - entering FILLING state");
    }
    break;

  case STATE_FILLING:
    // Llenándose, esperar nivel 7
    if (sensorState.sequenceError) {
      systemState = STATE_ERROR;
    } else if (sensors_is_tank_full(&sensorState)) {
      // ¡Tanque lleno! Encender bomba
      systemState = STATE_PUMPING;
      pump_on(&pumpStatus);
      alarm_beep(&alarmState); // Beep de inicio
      Serial.println("[MAIN] Tank FULL - PUMP ON");
    }
    break;

  case STATE_PUMPING:
    // Bomba activa, esperar que llegue a vacío
    if (sensorState.sequenceError) {
      // Error durante bombeo
      systemState = STATE_ERROR;
    } else if (sensors_is_tank_empty(&sensorState)) {
      // ¡Tanque vacío! Apagar bomba
      pump_off(&pumpStatus);
      sensors_reset_error(&sensorState); // Limpiar estados

      // Registrar tiempo de llenado para cálculo de emergencia
      unsigned long fillDuration = millis() - fillStartTime;
      pump_register_cycle(&pumpStatus, fillDuration);

      systemState = STATE_IDLE;
      alarm_beep(&alarmState); // Beep de fin de ciclo
      Serial.println("[MAIN] Tank EMPTY - PUMP OFF - Cycle complete");
    }
    break;

  case STATE_ERROR:
    // Modo de error
    if (!pumpStatus.isRunning) {
      // Encender bomba en modo emergencia
      pump_emergency_on(&pumpStatus);
      alarm_set(&alarmState, ALARM_ERROR);
      Serial.println("[MAIN] ERROR STATE - Emergency pump activated!");
    }

    // Verificar si terminó el tiempo de emergencia
    if (pump_emergency_timeout(&pumpStatus)) {
      pump_off(&pumpStatus);
      alarm_off(&alarmState);
      sensors_reset_error(&sensorState);
      systemState = STATE_IDLE;
      Serial.println("[MAIN] Emergency timeout - returning to IDLE");
    }

    // También terminar si todos los sensores están apagados
    if (sensors_is_tank_empty(&sensorState) &&
        pumpStatus.runTime > 5000) { // Al menos 5 segundos
      pump_off(&pumpStatus);
      alarm_off(&alarmState);
      sensors_reset_error(&sensorState);
      systemState = STATE_IDLE;
      Serial.println("[MAIN] Tank empty during emergency - returning to IDLE");
    }
    break;
  }

  // Log de cambio de estado
  if (previousState != systemState) {
    Serial.printf("[MAIN] State changed: %d -> %d\n", previousState,
                  systemState);
  }
}

void updateDisplay() {
  displayData.level = sensorState.currentLevel;
  displayData.pumpState = pumpStatus.state;
  displayData.hasError = sensorState.sequenceError;
  displayData.sequenceState = sensorState.sequenceState;
  displayData.cyclesCompleted = pumpStatus.cyclesCompleted;
  displayData.lastCycleDuration = pumpStatus.lastCycleDuration;
  displayData.pumpRunTime = pumpStatus.runTime;

#if MQTT_ENABLED
  displayData.wifiConnected = mqtt_is_connected();
#else
  displayData.wifiConnected = false;
#endif

  display_update(&displayData);
}

void publishMqtt() {
#if MQTT_ENABLED
  MqttData mqttData;
  mqttData.level = sensorState.currentLevel;
  mqttData.maxLevel = NUM_SENSORS;
  mqttData.pumpState = getPumpStateString(pumpStatus.state);
  mqttData.pumpRunning = pumpStatus.isRunning;
  mqttData.pumpRuntime = pumpStatus.runTime / 1000; // a segundos
  mqttData.hasError = sensorState.sequenceError;
  mqttData.sequenceState = getSequenceStateString(sensorState.sequenceState);
  mqttData.cyclesCompleted = pumpStatus.cyclesCompleted;
  mqttData.lastCycleDuration =
      pumpStatus.lastCycleDuration / 1000;                // a segundos
  mqttData.totalRuntime = pumpStatus.totalRunTime / 1000; // a segundos

  mqtt_publish_status(&mqttData);
#endif
}

const char *getPumpStateString(PumpState state) {
  switch (state) {
  case PUMP_OFF:
    return "off";
  case PUMP_ON:
    return "on";
  case PUMP_EMERGENCY:
    return "emergency";
  default:
    return "unknown";
  }
}

const char *getSequenceStateString(SequenceState state) {
  switch (state) {
  case SEQ_IDLE:
    return "idle";
  case SEQ_FILLING:
    return "filling";
  case SEQ_EMPTYING:
    return "emptying";
  case SEQ_ERROR:
    return "error";
  default:
    return "unknown";
  }
}

// Verificar botón de reset
// Mantener presionado 2 segundos: si hay error lo limpia, si no reinicia ESP
void checkResetButton() {
  bool buttonPressed = (digitalRead(RESET_BUTTON_PIN) == LOW); // Activo en bajo

  if (buttonPressed && !buttonWasPressed) {
    // Botón recién presionado
    buttonPressStart = millis();
    buttonWasPressed = true;
  } else if (buttonPressed && buttonWasPressed) {
    // Botón mantenido
    if (millis() - buttonPressStart >= BUTTON_HOLD_TIME_MS) {
      Serial.println("[RESET] Button held for 2 seconds");

      // Si hay error, limpiarlo
      if (sensorState.sequenceError || systemState == STATE_ERROR) {
        Serial.println("[RESET] Clearing error state...");
        pump_off(&pumpStatus);
        alarm_off(&alarmState);
        sensors_reset_error(&sensorState);
        systemState = STATE_IDLE;
        display_force_redraw();
        alarm_beep(&alarmState); // Beep de confirmación

        // Evitar múltiples triggers
        buttonPressStart = millis();
      } else {
        // No hay error, reiniciar ESP
        Serial.println("[RESET] Restarting ESP32...");
        delay(100);
        ESP.restart();
      }
    }
  } else {
    // Botón liberado
    buttonWasPressed = false;
  }
}

// Función para modo demo - simula llenado y vaciado
void updateDemoMode() {
  unsigned long currentTime = millis();

  // Asegurarse de que no haya errores en modo demo
  sensorState.sequenceError = false;

  if (currentTime - lastDemoUpdate >= DEMO_SPEED_MS) {
    lastDemoUpdate = currentTime;

    if (demoFilling) {
      // Llenando
      demoLevel++;
      sensorState.currentLevel = demoLevel;
      sensorState.sequenceState = SEQ_FILLING;

      if (demoLevel >= 7) {
        // Tanque lleno, encender bomba
        demoFilling = false;
        pumpStatus.state = PUMP_ON;
        pumpStatus.isRunning = true;
        pumpStatus.startTime = currentTime;
        sensorState.sequenceState = SEQ_EMPTYING;
        Serial.println("[DEMO] Tank full - PUMP ON");
      }
    } else {
      // Vaciando
      demoLevel--;
      sensorState.currentLevel = demoLevel;
      sensorState.sequenceState = SEQ_EMPTYING;

      // Actualizar tiempo de bomba
      pumpStatus.runTime = currentTime - pumpStatus.startTime;

      if (demoLevel <= 0) {
        // Tanque vacío, apagar bomba
        demoFilling = true;
        demoLevel = 0;
        pumpStatus.state = PUMP_OFF;
        pumpStatus.isRunning = false;
        pumpStatus.cyclesCompleted++;
        pumpStatus.lastCycleDuration = pumpStatus.runTime;
        sensorState.sequenceState = SEQ_IDLE;
        Serial.println("[DEMO] Tank empty - PUMP OFF - Cycle complete");
      }
    }
  }

  // Mantener actualizado el tiempo de bomba si está corriendo
  if (pumpStatus.isRunning) {
    pumpStatus.runTime = currentTime - pumpStatus.startTime;
  }
}
