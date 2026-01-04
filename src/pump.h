#ifndef PUMP_H
#define PUMP_H

#include "config.h"
#include <Arduino.h>

// Estados de la bomba
enum PumpState {
  PUMP_OFF,      // Bomba apagada
  PUMP_ON,       // Bomba encendida (ciclo normal)
  PUMP_EMERGENCY // Bomba en modo emergencia (por error)
};

// Estructura de estado de la bomba
struct PumpStatus {
  PumpState state;                 // Estado actual
  bool isRunning;                  // ¿Está funcionando?
  unsigned long startTime;         // Tiempo de inicio
  unsigned long runTime;           // Tiempo corriendo (ms)
  unsigned long totalRunTime;      // Tiempo total de funcionamiento hoy
  int cyclesCompleted;             // Ciclos completados hoy
  unsigned long lastCycleDuration; // Duración del último ciclo
  unsigned long avgCycleDuration;  // Duración promedio de ciclos
  unsigned long emergencyDuration; // Duración de emergencia calculada
};

// Inicializar control de bomba
void pump_init();

// Encender bomba (modo normal)
void pump_on(PumpStatus *status);

// Encender bomba (modo emergencia)
void pump_emergency_on(PumpStatus *status);

// Apagar bomba
void pump_off(PumpStatus *status);

// Actualizar estado (llamar en loop)
void pump_update(PumpStatus *status);

// Obtener tiempo de emergencia calculado
unsigned long pump_get_emergency_time(const PumpStatus *status);

// Verificar si debe apagar por timeout de emergencia
bool pump_emergency_timeout(const PumpStatus *status);

// Registrar ciclo completado (para calcular tiempo promedio)
void pump_register_cycle(PumpStatus *status, unsigned long fillTime);

// Resetear estadísticas diarias
void pump_reset_daily_stats(PumpStatus *status);

// Debug
void pump_debug_print(const PumpStatus *status);

#endif // PUMP_H
