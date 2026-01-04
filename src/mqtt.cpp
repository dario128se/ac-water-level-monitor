#include "mqtt.h"

#if MQTT_ENABLED

WiFiClient espClient;
PubSubClient mqttClient(espClient);

static unsigned long lastReconnectAttempt = 0;

bool mqtt_init() {
  Serial.println("[MQTT] Initializing...");

  if (!mqtt_connect_wifi()) {
    return false;
  }

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  return mqtt_connect();
}

bool mqtt_connect_wifi() {
  Serial.printf("[MQTT] Connecting to WiFi: %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("[MQTT] WiFi connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());
    return true;
  }

  Serial.println();
  Serial.println("[MQTT] WiFi connection failed!");
  return false;
}

bool mqtt_connect() {
  if (!WiFi.isConnected()) {
    return false;
  }

  Serial.printf("[MQTT] Connecting to broker %s:%d\n", MQTT_SERVER, MQTT_PORT);

  bool connected;
  if (strlen(MQTT_USER) > 0) {
    connected = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
  } else {
    connected = mqttClient.connect(MQTT_CLIENT_ID);
  }

  if (connected) {
    Serial.println("[MQTT] Connected to broker!");

    // Publicar mensaje de conexión
    mqttClient.publish(MQTT_TOPIC, "{\"status\":\"online\"}");
    return true;
  }

  Serial.printf("[MQTT] Connection failed, rc=%d\n", mqttClient.state());
  return false;
}

bool mqtt_is_connected() {
  return mqttClient.connected() && WiFi.isConnected();
}

void mqtt_publish_status(const MqttData *data) {
  if (!mqtt_is_connected()) {
    return;
  }

  // Construir JSON completo
  char payload[512];
  snprintf(payload, sizeof(payload),
           "{"
           "\"level\":%d,"
           "\"max_level\":%d,"
           "\"pump\":{"
           "\"state\":\"%s\","
           "\"running\":%s,"
           "\"runtime_s\":%lu"
           "},"
           "\"error\":%s,"
           "\"sequence\":\"%s\","
           "\"stats\":{"
           "\"cycles_today\":%d,"
           "\"last_cycle_s\":%lu,"
           "\"total_runtime_s\":%lu"
           "}"
           "}",
           data->level, data->maxLevel, data->pumpState,
           data->pumpRunning ? "true" : "false", data->pumpRuntime,
           data->hasError ? "true" : "false", data->sequenceState,
           data->cyclesCompleted, data->lastCycleDuration, data->totalRuntime);

  mqttClient.publish(MQTT_TOPIC, payload);
  Serial.printf("[MQTT] Published: %s\n", payload);
}

void mqtt_loop() {
  if (!WiFi.isConnected()) {
    return;
  }

  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (mqtt_connect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqttClient.loop();
  }
}

#else

// Stubs cuando MQTT está deshabilitado
bool mqtt_init() {
  Serial.println("[MQTT] Disabled in config");
  return false;
}

bool mqtt_connect_wifi() { return false; }
bool mqtt_connect() { return false; }
bool mqtt_is_connected() { return false; }
void mqtt_publish_status(const MqttData *data) { (void)data; }
void mqtt_loop() {}

#endif
