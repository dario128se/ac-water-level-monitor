#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PINES DE SENSORES DE NIVEL (7 boyas NA)
// ============================================
#define SENSOR_1_PIN 34 // Nivel 1 (más bajo)
#define SENSOR_2_PIN 35 // Nivel 2
#define SENSOR_3_PIN 32 // Nivel 3
#define SENSOR_4_PIN 33 // Nivel 4
#define SENSOR_5_PIN 25 // Nivel 5
#define SENSOR_6_PIN 26 // Nivel 6
#define SENSOR_7_PIN 27 // Nivel 7 (más alto - activa bomba)

#define NUM_SENSORS 7

// ============================================
// PINES DE CONTROL
// ============================================
#define PUMP_RELAY_PIN 13  // Relé de la bomba
#define BUZZER_PIN 12      // Buzzer de alarma
#define LED_ERROR_PIN 14   // LED indicador de error
#define RESET_BUTTON_PIN 0 // Botón reset (GPIO 0 = BOOT en ESP32)

// ============================================
// PINES DISPLAY TFT ILI9341 (SPI)
// Configurados en platformio.ini build_flags
// ============================================
// TFT_CS    = 15
// TFT_RST   = 4
// TFT_DC    = 2
// TFT_MOSI  = 23
// TFT_SCLK  = 18

// ============================================
// CONFIGURACIÓN DE TIEMPOS
// ============================================
#define SENSOR_READ_INTERVAL_MS 100    // Lectura de sensores cada 100ms
#define DISPLAY_UPDATE_INTERVAL_MS 500 // Actualizar display cada 500ms
#define DEBOUNCE_TIME_MS 50            // Debounce para sensores
#define MQTT_PUBLISH_INTERVAL_MS 5000  // Publicar MQTT cada 5 segundos

// Tiempo mínimo de funcionamiento de bomba en emergencia (segundos)
#define MIN_EMERGENCY_PUMP_TIME_S 60 // 1 minuto mínimo
// Factor de seguridad para cálculo de tiempo de vaciado
#define SAFETY_TIME_FACTOR 1.5

// ============================================
// CONFIGURACIÓN MQTT (Opcional)
// ============================================
#define MQTT_ENABLED false // Cambiar a true para habilitar

#define MQTT_SERVER "192.168.1.39" // IP de Raspberry Pi
#define MQTT_PORT 1883
#define MQTT_USER "nodered"
#define MQTT_PASSWORD "nodered1234"
#define MQTT_CLIENT_ID "ac-water-monitor"

// Topic MQTT (único)
#define MQTT_TOPIC "ac-monitor/status"

// ============================================
// CONFIGURACIÓN WiFi
// ============================================
#define WIFI_SSID "TU_WIFI_SSID"
#define WIFI_PASSWORD "TU_WIFI_PASSWORD"

// ============================================
// COLORES TFT (RGB565)
// ============================================
#define COLOR_BG 0x1082         // Fondo oscuro azulado
#define COLOR_HEADER 0x2945     // Header más oscuro
#define COLOR_TEXT 0xFFFF       // Texto blanco
#define COLOR_TEXT_DIM 0x8410   // Texto gris
#define COLOR_WATER_LOW 0x5D7F  // Azul claro
#define COLOR_WATER_HIGH 0x001F // Azul oscuro
#define COLOR_PUMP_ON 0x07E0    // Verde brillante
#define COLOR_PUMP_OFF 0x4208   // Gris
#define COLOR_ERROR 0xF800      // Rojo
#define COLOR_WARNING 0xFD20    // Naranja
#define COLOR_OK 0x07E0         // Verde

#endif // CONFIG_H
