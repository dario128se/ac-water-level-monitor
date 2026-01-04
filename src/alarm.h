#ifndef ALARM_H
#define ALARM_H

#include "config.h"
#include <Arduino.h>

// Patrones de alarma
enum AlarmPattern {
  ALARM_OFF,      // Sin alarma
  ALARM_ERROR,    // Error de secuencia (pitido intermitente rápido)
  ALARM_WARNING,  // Advertencia (pitido lento)
  ALARM_BEEP_ONCE // Un solo pitido
};

// Estado de la alarma
struct AlarmState {
  AlarmPattern pattern;
  bool ledOn;
  bool buzzerOn;
  unsigned long lastToggle;
};

// Inicializar
void alarm_init();

// Activar alarma con patrón
void alarm_set(AlarmState *state, AlarmPattern pattern);

// Desactivar alarma
void alarm_off(AlarmState *state);

// Actualizar (llamar en loop para generar patrones)
void alarm_update(AlarmState *state);

// Emitir un solo beep
void alarm_beep(AlarmState *state);

#endif // ALARM_H
