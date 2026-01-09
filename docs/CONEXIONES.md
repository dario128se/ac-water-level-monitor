# Diagrama de Conexiones - AC Water Level Monitor

## Pinout ESP32 DevKit V1

Basado en tu placa ESP32-WROOM-32 DevKit V1:

```
                    ┌─────────────────┐
                    │      ESP32      │
                    │    DevKit V1    │
                    │                 │
          EN   [15] │                 │ [15] GPIO23/MOSI ←── Display MOSI
      GPIO36   [14] │                 │ [14] GPIO22/SCL
      GPIO39   [13] │                 │ [13] GPIO1/TX0
      GPIO34   [12] │←── Sensor 1     │ [12] GPIO3/RX0
      GPIO35   [11] │←── Sensor 2     │ [11] GPIO21/SDA
      GPIO32   [10] │←── Sensor 3     │ [10] GPIO19/MISO
      GPIO33    [9] │←── Sensor 4     │  [9] GPIO18/SCK ←── Display SCK
      GPIO25    [8] │←── Sensor 5     │  [8] GPIO5/CS
      GPIO26    [7] │←── Sensor 6     │  [7] GPIO17/TX2
      GPIO27    [6] │←── Sensor 7     │  [6] GPIO16/RX2
      GPIO14    [5] │──→ LED Error    │  [5] GPIO4 ──→ Display RESET
      GPIO12    [4] │──→ Buzzer       │  [4] GPIO2 ──→ Display DC
      GPIO13    [3] │──→ Relé IN      │  [3] GPIO15 ──→ Display CS
         GND    [2] │                 │  [2] GND
         VIN    [1] │←── 5V entrada   │  [1] 3.3V ──→ Sensores/Display
                    │                 │
                    │[EN]  USB  [BOOT]│
                    └─────────────────┘
```

---

## 1. SENSORES DE NIVEL (7 boyas magnéticas NA)

Cada sensor es **normalmente abierto (NA)**. Cuando el agua sube, cierra el circuito.

### Conexión de los 7 sensores

```
    3.3V (pin 1 lado derecho ESP32)
      │
      └─────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
          ┌─┴──┐     ┌─┴──┐     ┌─┴──┐     ┌─┴──┐     ┌─┴──┐     ┌─┴──┐     ┌─┴──┐
          │ S1 │     │ S2 │     │ S3 │     │ S4 │     │ S5 │     │ S6 │     │ S7 │
          └─┬──┘     └─┬──┘     └─┬──┘     └─┬──┘     └─┬──┘     └─┬──┘     └─┬──┘
            │          │          │          │          │          │          │
            ├─► GPIO34 ├─► GPIO35 ├─► GPIO32 ├─► GPIO33 ├─► GPIO25 ├─► GPIO26 ├─► GPIO27
            │          │          │          │          │          │          │          
           [R]        [R]        [R]        [R]        [R]        [R]        [R]
           10K        10K        10K        10K        10K        10K        10K
            │          │          │          │          │          │          │
            └──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──── GND
```

**Resumen:**
- Todos los sensores comparten el **común a 3.3V**
- Cada sensor tiene su propia **resistencia de 10K a GND** (pull-down)
- Cada sensor va a un **GPIO diferente**

**Tabla de conexiones:**
| Sensor    | GPIO   | Pin ESP32 (lado izq) | Función           |
|-----------|--------|----------------------|-------------------|
| S1 (bajo) | GPIO34 | Pin 12               | Primer nivel      |
| S2        | GPIO35 | Pin 11               | Segundo nivel     |
| S3        | GPIO32 | Pin 10               | Tercer nivel      |
| S4        | GPIO33 | Pin 9                | Cuarto nivel      |
| S5        | GPIO25 | Pin 8                | Quinto nivel      |
| S6        | GPIO26 | Pin 7                | Sexto nivel       |
| S7 (alto) | GPIO27 | Pin 6                | Último nivel      |

### Diagrama de UN sensor individual:

```
        3.3V (del ESP32)
          │
          │
      ┌───┴───┐
      │ BOYA  │  ← Sensor Normalmente Abierto (NA)
      │MAGNET.│    Cuando sube el agua, el imán cierra el contacto
      └───┬───┘
          │
          ├─────────────────► GPIO (ej: GPIO34)
          │
         ┌┴┐
         │ │ 10KΩ (resistencia pull-down)
         │ │
         └┬┘
          │
          ▼
         GND
```

### ¿Cómo funciona?

| Estado del agua | Estado del sensor | Resistencia conecta GPIO a... | GPIO lee        |
|-----------------|-------------------|-------------------------------|-----------------|
| **Sin agua**    | Abierto (NA)      | GND (via 10K)                 | **LOW** (0V)    |
| **Con agua**    | Cerrado           | 3.3V (directo)                | **HIGH** (3.3V) |

**El código busca `digitalRead() == HIGH` para detectar agua presente.**

---

## 2. DISPLAY TFT ILI9341 (SPI)

```
    Display TFT 2.4"           ESP32 DevKit V1
    ┌─────────────┐
    │ ILI9341     │
    │             │
    │  VCC  ──────┼────────── 3.3V (pin 1 derecho)
    │  GND  ──────┼────────── GND  (pin 2 derecho)
    │  CS   ──────┼────────── GPIO15 (pin 3 derecho)
    │  RESET ─────┼────────── GPIO4  (pin 5 derecho)
    │  DC   ──────┼────────── GPIO2  (pin 4 derecho)
    │  MOSI ──────┼────────── GPIO23 (pin 15 derecho, D23)
    │  SCK  ──────┼────────── GPIO18 (pin 9 derecho)
    │  LED  ──────┼────────── 3.3V (pin 1 derecho)
    │  MISO ──────┼────────── (no conectar)
    │             │
    └─────────────┘
```

---

## 3. SALIDAS DE CONTROL

### Módulo Relé 5V (para bomba 12V)

```
    ESP32                    Módulo Relé           Bomba 12V
    ┌──────┐                  ┌─────────┐           ┌──────┐
    │      │                  │         │           │      │
    │GPIO13├──────────────────│ IN      │           │      │
    │      │                  │         │     ┌─────┤ +    │
    │ GND  ├──────────────────│ GND     │     │     │      │
    │      │                  │         │     │     │      │
    │ VIN  ├──────────────────│ VCC     │     │     └──────┘
    │ (5V) │                  │         │     │
    └──────┘                  │    COM ─┼──┬──┘
                              │         │  │
         12V DC ──────────────│    NO  ─┼──┘ (Normalmente Abierto)
         (desde fuente)       │         │
                              │    NC   │ (no usar)
                              └─────────┘
```

### Buzzer Activo 5V
```
    ESP32                         Buzzer
    ┌──────┐                       ┌───┐
    │      │                       │ + │
    │GPIO12├───────────────────────┤   │
    │      │                       │ - │
    │ GND  ├───────────────────────┤   │
    └──────┘                       └───┘
```

### LED de Error
```
    ESP32                         LED
    ┌──────┐                       
    │      │         220Ω         ┌───┐
    │GPIO14├────────[===]─────────┤ + │ (ánodo)
    │      │                      │   │
    │ GND  ├──────────────────────┤ - │ (cátodo)
    └──────┘                      └───┘
```

---

## 4. BOTONES EXTERNOS (RESET y BOOT)

Para acceder a las funciones de reset y modo demo desde fuera del gabinete:

### Ubicación en el pinout

```
                    ┌─────────────────┐
                    │      ESP32      │
                    │    DevKit V1    │
                    │                 │
    ┌──►  EN   [15] │                 │ [15]
    │   GPIO36 [14] │                 │ [14]
    │       ...     │                 │  ...
    │      GND  [2] │                 │  [2] GND ◄──┐
    │      VIN  [1] │                 │  [1] 3.3V   │
    │               │                 │             │
    │               │[EN]  USB  [BOOT]│             │
    │               └───┬─────────┬───┘             │
    │                   │         │                 │
    └───────────────────┘         └─► GPIO 0 ───────┤
         (Reset)                      (Boot)        │
                                                    │
                                                   GND
```

### Conexiones

**Botón RESET (EN)** - Reinicia completamente el ESP32:
```
    Pin EN (pin 15 lado izquierdo)
      │
      │
    ┌─┴─┐
    │   │  Botón NA
    │ R │  (Normalmente Abierto)
    └─┬─┘
      │
      ▼
     GND (pin 2 cualquier lado)
```

**Botón BOOT (GPIO 0)** - Modo demo / Limpiar errores:
```
    GPIO 0 (no está en los pines laterales,
            el acceso más fácil es soldando 
            directamente al botón BOOT de la placa,
            o usando un cable jumper en el header)
      │
      │
    ┌─┴─┐
    │   │  Botón NA
    │ B │  (Normalmente Abierto)
    └─┬─┘
      │
      ▼
     GND
```

### Funciones de los botones

| Botón | Acción | Resultado |
|-------|--------|-----------|
| **RESET** | Presionar | Reinicia el ESP32 inmediatamente |
| **BOOT** | Mantener 2 seg (en operación normal) | Si hay error: lo limpia. Si no: reinicia |
| **BOOT** | Mantener durante splash (al encender) | Activa modo DEMO |

### Conexión práctica

```
    ESP32 DevKit                     Panel Frontal
    ┌──────────┐                    ┌─────────────┐
    │          │                    │             │
    │  EN [15] ├────────────────────┤──[RESET]──┐ │
    │          │                    │           │ │
    │ GND [2]  ├────────┬───────────┤───────────┴─┤
    │          │        │           │             │
    │  BOOT*   ├────────┼───────────┤──[BOOT]───┬─┤
    │          │        │           │           │ │
    └──────────┘        │           │           │ │
                        └───────────┼───────────┴─┤
                                    └─────────────┘

    * GPIO 0 / Botón BOOT: Soldar cables al botón existente
      o buscar un testpoint en la placa
```

### Nota sobre GPIO 0

GPIO 0 no tiene un pin dedicado en los headers laterales del ESP32 DevKit V1.
**Opciones para acceder a GPIO 0:**
1. Soldar cables directamente a los terminales del botón BOOT de la placa
2. Buscar un testpoint marcado como "IO0" o "GPIO0" en la placa
3. Usar una placa ESP32 que tenga GPIO 0 en los headers

---

## 5. ALIMENTACIÓN

```
    ┌─────────────┐     ┌──────────────┐     ┌──────────────┐
    │             │     │              │     │              │
    │  220V AC    │────►│   Fuente     │────►│   12V DC     │──┬──► Bomba (via relé)
    │  (enchufe)  │     │   AC-DC      │     │   2A min     │  │
    │             │     │              │     │              │  │
    └─────────────┘     └──────────────┘     └──────┬───────┘  │
                                                    │          │
                                                    ▼          │
                                             ┌──────────────┐  │
                                             │   LM2596     │  │
                                             │   o similar  │  │
                                             │   DC-DC      │  │
                                             │   12V → 5V   │  │
                                             └──────┬───────┘  │
                                                    │          │
                                                    ▼          │
                                             ┌──────────────┐  │
                                             │    5V DC     │  │
                                             │              │  │
                                             │  VIN ESP32   │  │
                                             │  VCC Relé    │  │
                                             └──────────────┘  │
                                                               │
    ⚠️ IMPORTANTE: El GND de 5V y 12V                          │
       deben estar conectados en común ◄───────────────────────┘
```

---

## 6. RESUMEN DE PINES ESP32

| Función       | GPIO | Lado | Pin # | Notas           |
|---------------|------|------|-------|-----------------|
| Sensor 1      | 34   | Izq  | 12    | Solo entrada    |
| Sensor 2      | 35   | Izq  | 11    | Solo entrada    |
| Sensor 3      | 32   | Izq  | 10    | Solo entrada    |
| Sensor 4      | 33   | Izq  | 9     | Solo entrada    |
| Sensor 5      | 25   | Izq  | 8     | Solo entrada    |
| Sensor 6      | 26   | Izq  | 7     | Solo entrada    |
| Sensor 7      | 27   | Izq  | 6     | Solo entrada    |
| Relé          | 13   | Izq  | 3     |                 |
| Buzzer        | 12   | Izq  | 4     |                 |
| LED Error     | 14   | Izq  | 5     |                 |
| Display CS    | 15   | Der  | 3     |                 |
| Display DC    | 2    | Der  | 4     |                 |
| Display RESET | 4    | Der  | 5     |                 |
| Display SCK   | 18   | Der  | 9     |                 |
| Display MOSI  | 23   | Der  | 15    | D23 en tu placa |
| Reset Button  | 0    |  -   | BOOT  | Integrado       |

---

## ⚠️ Notas Importantes

1. **GPIO34, 35**      : Son solo de entrada (input-only), perfectos para sensores
2. **GPIO2**           : Tiene LED integrado, puede parpadear al programar
3. **GPIO0**           : Es el botón BOOT, no mantener presionado al encender
4. **Resistencias 10K**: Imprescindibles para cada sensor (pull-down a GND)
5. **3.3V máximo**     : Los GPIOs del ESP32 son de 3.3V, no conectar 5V directamente
6. **GND común**       : Todos los GND deben estar conectados entre sí
