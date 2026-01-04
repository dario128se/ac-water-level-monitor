#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"
#include <Arduino.h>

// Estados de secuencia
enum SequenceState {
  SEQ_IDLE,     // Sin cambios
  SEQ_FILLING,  // Llenándose (1→7)
  SEQ_EMPTYING, // Vaciándose (7→1)
  SEQ_ERROR     // Secuencia incorrecta
};

// Estructura de estado de sensores
struct SensorState {
  bool levels[NUM_SENSORS];     // Estado actual de cada sensor
  int currentLevel;             // Nivel actual (0-7)
  int previousLevel;            // Nivel anterior
  SequenceState sequenceState;  // Estado de la secuencia
  bool sequenceError;           // Flag de error de secuencia
  unsigned long lastChangeTime; // Tiempo del último cambio
};

// Inicializar pines de sensores
void sensors_init();

// Leer estado actual de todos los sensores
void sensors_read(SensorState *state);

// Validar secuencia de sensores
bool sensors_validate_sequence(SensorState *state);

// Obtener nivel actual (0-7)
int sensors_get_level(const SensorState *state);

// Verificar si el tanque está lleno (nivel 7)
bool sensors_is_tank_full(const SensorState *state);

// Verificar si el tanque está vacío (nivel 0)
bool sensors_is_tank_empty(const SensorState *state);

// Resetear estado de error
void sensors_reset_error(SensorState *state);

// Debug: imprimir estado de sensores
void sensors_debug_print(const SensorState *state);

#endif // SENSORS_H
