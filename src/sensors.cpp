#include "sensors.h"

// Array de pines de sensores (ordenados por nivel)
static const int sensorPins[NUM_SENSORS] = {
    SENSOR_1_PIN, SENSOR_2_PIN, SENSOR_3_PIN, SENSOR_4_PIN,
    SENSOR_5_PIN, SENSOR_6_PIN, SENSOR_7_PIN};

// Estado anterior para detectar cambios
static bool previousLevels[NUM_SENSORS] = {false};
static unsigned long lastDebounceTime[NUM_SENSORS] = {0};
static bool debouncedLevels[NUM_SENSORS] = {false};

void sensors_init() {
  // Configurar pines como entrada
  // GPIO 34 y 35 son solo entrada, no necesitan pulldown externo
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(sensorPins[i], INPUT);
    previousLevels[i] = false;
    debouncedLevels[i] = false;
    lastDebounceTime[i] = 0;
  }

  Serial.println("[SENSORS] Initialized 7 level sensors");
}

void sensors_read(SensorState *state) {
  unsigned long currentTime = millis();
  int newLevel = 0;

  // Leer y aplicar debounce a cada sensor
  for (int i = 0; i < NUM_SENSORS; i++) {
    bool reading = digitalRead(sensorPins[i]) == HIGH;

    // Si cambió el estado, resetear timer de debounce
    if (reading != previousLevels[i]) {
      lastDebounceTime[i] = currentTime;
    }

    // Si pasó el tiempo de debounce, aceptar el nuevo valor
    if ((currentTime - lastDebounceTime[i]) > DEBOUNCE_TIME_MS) {
      if (reading != debouncedLevels[i]) {
        debouncedLevels[i] = reading;
        state->lastChangeTime = currentTime;
      }
    }

    previousLevels[i] = reading;
    state->levels[i] = debouncedLevels[i];

    // Contar niveles activos
    if (state->levels[i]) {
      newLevel = i + 1; // El nivel es el sensor más alto activo
    }
  }

  // Guardar nivel anterior antes de actualizar
  state->previousLevel = state->currentLevel;
  state->currentLevel = newLevel;

  // Detectar dirección del cambio
  if (state->currentLevel > state->previousLevel) {
    if (state->sequenceState != SEQ_FILLING &&
        state->sequenceState != SEQ_IDLE) {
      // Cambio de dirección inesperado
      if (state->previousLevel != 0) {
        state->sequenceState = SEQ_ERROR;
        state->sequenceError = true;
      }
    } else {
      state->sequenceState = SEQ_FILLING;
    }
  } else if (state->currentLevel < state->previousLevel) {
    if (state->sequenceState != SEQ_EMPTYING &&
        state->sequenceState != SEQ_IDLE) {
      // Cambio de dirección inesperado durante llenado
      state->sequenceState = SEQ_ERROR;
      state->sequenceError = true;
    } else {
      state->sequenceState = SEQ_EMPTYING;
    }
  }

  // Si llegamos a 0 o 7, volver a IDLE
  if (state->currentLevel == 0 || state->currentLevel == NUM_SENSORS) {
    if (!state->sequenceError) {
      state->sequenceState = SEQ_IDLE;
    }
  }
}

bool sensors_validate_sequence(SensorState *state) {
  // Verificar que los sensores activos sean contiguos desde el nivel 1
  // Por ejemplo: nivel 3 debe tener S1, S2, S3 activos

  bool valid = true;
  int expectedActive = state->currentLevel;

  for (int i = 0; i < NUM_SENSORS; i++) {
    bool shouldBeActive = (i < expectedActive);
    if (state->levels[i] != shouldBeActive) {
      // Sensor no contiguo detectado
      valid = false;
      state->sequenceError = true;
      state->sequenceState = SEQ_ERROR;

      Serial.printf(
          "[SENSORS] ERROR: Sensor %d deberia estar %s pero esta %s\n", i + 1,
          shouldBeActive ? "ACTIVO" : "INACTIVO",
          state->levels[i] ? "ACTIVO" : "INACTIVO");
      break;
    }
  }

  return valid;
}

int sensors_get_level(const SensorState *state) { return state->currentLevel; }

bool sensors_is_tank_full(const SensorState *state) {
  return state->currentLevel >= NUM_SENSORS;
}

bool sensors_is_tank_empty(const SensorState *state) {
  return state->currentLevel == 0;
}

void sensors_reset_error(SensorState *state) {
  state->sequenceError = false;
  state->sequenceState = SEQ_IDLE;
  Serial.println("[SENSORS] Error reset");
}

void sensors_debug_print(const SensorState *state) {
  Serial.print("[SENSORS] Niveles: ");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(state->levels[i] ? "1" : "0");
  }
  Serial.printf(" | Nivel: %d | Estado: ", state->currentLevel);

  switch (state->sequenceState) {
  case SEQ_IDLE:
    Serial.print("IDLE");
    break;
  case SEQ_FILLING:
    Serial.print("LLENANDO");
    break;
  case SEQ_EMPTYING:
    Serial.print("VACIANDO");
    break;
  case SEQ_ERROR:
    Serial.print("ERROR");
    break;
  }

  if (state->sequenceError) {
    Serial.print(" [ERROR SECUENCIA]");
  }
  Serial.println();
}
