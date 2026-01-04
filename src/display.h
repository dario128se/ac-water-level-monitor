#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include "pump.h"
#include "sensors.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

// Estructura para datos a mostrar
struct DisplayData {
  int level;                       // Nivel actual 0-7
  PumpState pumpState;             // Estado de bomba
  bool hasError;                   // ¿Hay error?
  SequenceState sequenceState;     // Estado de secuencia
  int cyclesCompleted;             // Ciclos completados
  unsigned long lastCycleDuration; // Duración último ciclo (ms)
  unsigned long pumpRunTime;       // Tiempo bomba encendida
  bool wifiConnected;              // Estado WiFi
};

// Inicializar display
void display_init();

// Actualizar toda la pantalla
void display_update(const DisplayData *data);

// Mostrar pantalla de inicio
void display_splash();

// Mostrar error
void display_error(const char *message);

// Forzar redibujado completo
void display_force_redraw();

#endif // DISPLAY_H
