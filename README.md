# AC Water Level Monitor

Sistema IoT para monitoreo de nivel de agua en depósito de descarga de aire acondicionado con control automático de bomba de achique.

## 🌟 Características

- **7 sensores de nivel** con detección de secuencia
- **Display TFT a color** (ILI9341 240x320)
- **Control automático de bomba** con modo de emergencia
- **Alarma sonora** para errores de secuencia
- **MQTT opcional** para integración con Home Assistant / Raspberry Pi

## 📦 Hardware Necesario

| Componente | Cantidad | Notas |
|------------|----------|-------|
| ESP32-WROOM-32 DevKit | 1 | O ESP32-S3 |
| Display TFT IPS 2.4" ILI9341 | 1 | SPI, 240x320 |
| Sensor boya magnética NA | 7 | Normalmente abierto |
| Módulo relé 5V 1 canal | 1 | Para bomba 12V |
| Buzzer activo 5V | 1 | |
| LED rojo 5mm | 1 | Indicador error |
| Resistencias 10kΩ | 7 | Pull-down sensores |
| Fuente 12V 2A | 1 | |
| Regulador DC-DC 5V | 1 | LM2596 o similar |
| Bomba 12V | 1 | Tipo "sapito" de auto |

## 🔌 Conexiones

### Sensores de Nivel
```
Sensor 1 (bajo)  → GPIO 34
Sensor 2         → GPIO 35
Sensor 3         → GPIO 32
Sensor 4         → GPIO 33
Sensor 5         → GPIO 25
Sensor 6         → GPIO 26
Sensor 7 (alto)  → GPIO 27
```

### Display TFT (SPI)
```
VCC   → 3.3V
GND   → GND
CS    → GPIO 15
RESET → GPIO 4
DC    → GPIO 2
MOSI  → GPIO 23
SCK   → GPIO 18
LED   → 3.3V
```

### Control
```
Relé Bomba  → GPIO 13
Buzzer      → GPIO 12
LED Error   → GPIO 14
Reset Button → GPIO 0 (BOOT button integrado)
```

## 🚀 Instalación

1. Instalar [PlatformIO](https://platformio.org/install)

2. Clonar/copiar el proyecto

3. Compilar y subir:
```bash
cd ac-water-level-monitor
pio run -t upload
```

4. Monitor serial:
```bash
pio device monitor -b 115200
```

## ⚙️ Configuración

Editar `include/config.h`:

### WiFi/MQTT (opcional)
```cpp
#define MQTT_ENABLED true
#define WIFI_SSID "TU_WIFI"
#define WIFI_PASSWORD "TU_PASSWORD"
#define MQTT_SERVER "192.168.1.100"
```

### Tiempos
```cpp
#define MIN_EMERGENCY_PUMP_TIME_S 60  // Tiempo mínimo emergencia
#define SAFETY_TIME_FACTOR 1.5        // Factor de seguridad
```

## 📊 Funcionamiento

### Ciclo Normal
1. Agua sube → sensores 1,2,3,4,5,6,7 se activan en orden
2. Al llegar a nivel 7 → **bomba se enciende**
3. Agua baja → sensores 7,6,5,4,3,2,1 se desactivan en orden
4. Al llegar a nivel 0 → **bomba se apaga**
5. Ciclo se repite

### Modo Error
Si los sensores no siguen la secuencia correcta:
- 🔴 LED de error parpadea
- 🔊 Buzzer suena intermitente
- 💧 Bomba se activa por tiempo de seguridad

### Botón de Reset
**Mantener presionado 2 segundos:**
- Si hay error → Limpia el error y vuelve a IDLE
- Si no hay error → Reinicia el ESP32

## 📡 MQTT

Topic único: `ac-monitor/status`

### Payload JSON
```json
{
  "level": 5,
  "max_level": 7,
  "pump": {
    "state": "on",
    "running": true,
    "runtime_s": 45
  },
  "error": false,
  "sequence": "emptying",
  "stats": {
    "cycles_today": 12,
    "last_cycle_s": 180,
    "total_runtime_s": 2160
  }
}
```

## 📄 Licencia

MIT License - Libre para uso personal y comercial.

