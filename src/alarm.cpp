#include "alarm.h"

// Tiempos de patrones (ms)
#define ERROR_TOGGLE_TIME 150   // Rápido para error
#define WARNING_TOGGLE_TIME 500 // Lento para advertencia
#define BEEP_DURATION 100       // Duración de beep único

void alarm_init() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_ERROR_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_ERROR_PIN, LOW);

  Serial.println("[ALARM] Initialized");
}

void alarm_set(AlarmState *state, AlarmPattern pattern) {
  if (state->pattern != pattern) {
    state->pattern = pattern;
    state->lastToggle = millis();

    if (pattern == ALARM_OFF) {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_ERROR_PIN, LOW);
      state->buzzerOn = false;
      state->ledOn = false;
    }

    Serial.printf("[ALARM] Pattern set to: %d\n", pattern);
  }
}

void alarm_off(AlarmState *state) { alarm_set(state, ALARM_OFF); }

void alarm_update(AlarmState *state) {
  if (state->pattern == ALARM_OFF) {
    return;
  }

  unsigned long currentTime = millis();
  unsigned long toggleTime = 0;

  switch (state->pattern) {
  case ALARM_ERROR:
    toggleTime = ERROR_TOGGLE_TIME;
    break;
  case ALARM_WARNING:
    toggleTime = WARNING_TOGGLE_TIME;
    break;
  case ALARM_BEEP_ONCE:
    if (currentTime - state->lastToggle >= BEEP_DURATION) {
      alarm_off(state);
    } else {
      digitalWrite(BUZZER_PIN, HIGH);
    }
    return;
  default:
    return;
  }

  // Toggle buzzer y LED
  if (currentTime - state->lastToggle >= toggleTime) {
    state->lastToggle = currentTime;
    state->buzzerOn = !state->buzzerOn;
    state->ledOn = !state->ledOn;

    digitalWrite(BUZZER_PIN, state->buzzerOn ? HIGH : LOW);
    digitalWrite(LED_ERROR_PIN, state->ledOn ? HIGH : LOW);
  }
}

void alarm_beep(AlarmState *state) {
  state->pattern = ALARM_BEEP_ONCE;
  state->lastToggle = millis();
  digitalWrite(BUZZER_PIN, HIGH);
}
