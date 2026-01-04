#include "pump.h"

void pump_init() {
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Bomba apagada inicialmente

  Serial.println("[PUMP] Initialized - Relay on GPIO " +
                 String(PUMP_RELAY_PIN));
}

void pump_on(PumpStatus *status) {
  if (status->state != PUMP_ON) {
    digitalWrite(PUMP_RELAY_PIN, HIGH);
    status->state = PUMP_ON;
    status->isRunning = true;
    status->startTime = millis();

    Serial.println("[PUMP] ON - Normal mode");
  }
}

void pump_emergency_on(PumpStatus *status) {
  if (status->state != PUMP_EMERGENCY) {
    digitalWrite(PUMP_RELAY_PIN, HIGH);
    status->state = PUMP_EMERGENCY;
    status->isRunning = true;
    status->startTime = millis();

    // Calcular tiempo de emergencia
    status->emergencyDuration = pump_get_emergency_time(status);

    Serial.printf("[PUMP] EMERGENCY ON - Duration: %lu seconds\n",
                  status->emergencyDuration / 1000);
  }
}

void pump_off(PumpStatus *status) {
  if (status->isRunning) {
    digitalWrite(PUMP_RELAY_PIN, LOW);

    unsigned long runDuration = millis() - status->startTime;
    status->runTime = runDuration;
    status->totalRunTime += runDuration;

    if (status->state == PUMP_ON) {
      // Ciclo normal completado
      status->cyclesCompleted++;
      status->lastCycleDuration = runDuration;

      // Actualizar promedio
      if (status->cyclesCompleted == 1) {
        status->avgCycleDuration = runDuration;
      } else {
        status->avgCycleDuration =
            (status->avgCycleDuration * (status->cyclesCompleted - 1) +
             runDuration) /
            status->cyclesCompleted;
      }

      Serial.printf("[PUMP] OFF - Cycle completed in %lu ms\n", runDuration);
    } else {
      Serial.printf("[PUMP] OFF - Emergency ended after %lu ms\n", runDuration);
    }

    status->state = PUMP_OFF;
    status->isRunning = false;
  }
}

void pump_update(PumpStatus *status) {
  if (status->isRunning) {
    status->runTime = millis() - status->startTime;
  }
}

unsigned long pump_get_emergency_time(const PumpStatus *status) {
  // Si tenemos datos de ciclos previos, usar promedio * factor de seguridad
  if (status->avgCycleDuration > 0) {
    return (unsigned long)(status->avgCycleDuration * SAFETY_TIME_FACTOR);
  }

  // Si no hay datos, usar tiempo mínimo de emergencia
  return MIN_EMERGENCY_PUMP_TIME_S * 1000;
}

bool pump_emergency_timeout(const PumpStatus *status) {
  if (status->state != PUMP_EMERGENCY) {
    return false;
  }

  return status->runTime >= status->emergencyDuration;
}

void pump_register_cycle(PumpStatus *status, unsigned long fillTime) {
  // Este método se puede usar para registrar el tiempo de llenado
  // y mejorar el cálculo del tiempo de emergencia
  Serial.printf("[PUMP] Fill cycle registered: %lu ms\n", fillTime);
}

void pump_reset_daily_stats(PumpStatus *status) {
  status->cyclesCompleted = 0;
  status->totalRunTime = 0;
  Serial.println("[PUMP] Daily stats reset");
}

void pump_debug_print(const PumpStatus *status) {
  Serial.print("[PUMP] Estado: ");
  switch (status->state) {
  case PUMP_OFF:
    Serial.print("OFF");
    break;
  case PUMP_ON:
    Serial.print("ON");
    break;
  case PUMP_EMERGENCY:
    Serial.print("EMERGENCY");
    break;
  }

  if (status->isRunning) {
    Serial.printf(" | Running: %lu s", status->runTime / 1000);
  }

  Serial.printf(" | Cycles: %d | Avg: %lu s\n", status->cyclesCompleted,
                status->avgCycleDuration / 1000);
}
