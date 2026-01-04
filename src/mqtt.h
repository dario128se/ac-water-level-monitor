#ifndef MQTT_H
#define MQTT_H

#include "config.h"
#include <Arduino.h>

#if MQTT_ENABLED
#include <PubSubClient.h>
#include <WiFi.h>
#endif

// Estructura de datos para publicar
struct MqttData {
  int level;
  int maxLevel;
  const char *pumpState;
  bool pumpRunning;
  unsigned long pumpRuntime; // segundos
  bool hasError;
  const char *sequenceState;
  int cyclesCompleted;
  unsigned long lastCycleDuration; // segundos
  unsigned long totalRuntime;      // segundos de bomba hoy
};

// Inicializar WiFi y MQTT
bool mqtt_init();

// Conectar a WiFi
bool mqtt_connect_wifi();

// Conectar al broker MQTT
bool mqtt_connect();

// Verificar conexión
bool mqtt_is_connected();

// Publicar estado
void mqtt_publish_status(const MqttData *data);

// Loop de mantenimiento (llamar frecuentemente)
void mqtt_loop();

#endif // MQTT_H
