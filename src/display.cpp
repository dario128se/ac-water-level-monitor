#include "display.h"

// Instancia global del display
TFT_eSPI tft = TFT_eSPI();

// Cache para evitar redibujados innecesarios
static DisplayData lastData;
static bool needsFullRedraw = true;

// Dimensiones y posiciones
#define SCREEN_W 240
#define SCREEN_H 320

// Áreas de la UI
#define HEADER_H 40
#define TANK_X 30
#define TANK_Y 48
#define TANK_W 55
#define TANK_H 154 // 22px por nivel (22*7=154)
#define INFO_X 100
#define INFO_Y 48
#define STATS_Y 265

// Colores gradiente para nivel de agua
static const uint16_t waterColors[] = {
    0x5D7F, // Nivel 1 - Azul muy claro
    0x4D5F, // Nivel 2
    0x3D1F, // Nivel 3
    0x2CBF, // Nivel 4
    0x1C5F, // Nivel 5
    0x0C1F, // Nivel 6
    0x001F  // Nivel 7 - Azul oscuro
};

void display_init() {
  tft.init();
  tft.setRotation(0); // Portrait
  tft.fillScreen(COLOR_BG);

  Serial.println("[DISPLAY] TFT ILI9341 initialized (240x320)");

  needsFullRedraw = true;
  memset(&lastData, 0, sizeof(lastData));
}

void drawHeader(bool wifiConnected) {
  // Fondo header
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, COLOR_HEADER);

  // Icono gota de agua
  tft.fillCircle(20, 20, 8, 0x5D7F);
  tft.fillTriangle(20, 10, 12, 22, 28, 22, 0x5D7F);

  // Título
  tft.setTextColor(COLOR_TEXT, COLOR_HEADER);
  tft.setTextFont(2);
  tft.setCursor(35, 12);
  tft.print("MONITOR DE NIVEL");

  // Indicador WiFi
  if (wifiConnected) {
    tft.fillCircle(220, 20, 5, COLOR_OK);
  } else {
    tft.drawCircle(220, 20, 5, COLOR_TEXT_DIM);
  }
}

// Variable para animación
static uint8_t animFrame = 0;

void drawTank(int level) {
  int levelHeight = TANK_H / 7;
  int waterHeight = level * levelHeight;
  int waterY = TANK_Y + TANK_H - waterHeight;

  // Fondo del tanque (vacío) - con bordes redondeados
  // Esto limpia todo incluyendo ondas anteriores
  tft.fillRoundRect(TANK_X, TANK_Y, TANK_W, TANK_H, 8, 0x1082);

  // Dibujar agua con gradiente
  for (int i = 0; i < level; i++) {
    int y = TANK_Y + TANK_H - (i + 1) * levelHeight;
    uint16_t color = waterColors[i];

    if (i == level - 1) {
      // Nivel superior: con bordes redondeados arriba
      tft.fillRoundRect(TANK_X + 2, y, TANK_W - 4, levelHeight, 4, color);
    } else {
      tft.fillRect(TANK_X + 2, y, TANK_W - 4, levelHeight, color);
    }
  }

  // Ondas animadas en la superficie del agua (solo si hay agua)
  if (level > 0 && level < 7) {
    int waveY = waterY + 4;
    uint16_t waveColor = waterColors[level - 1];

    // Centrar ondas dentro del tanque
    int startX = TANK_X + 6;
    int endX = TANK_X + TANK_W - 6;

    // Dibujar ondas sinusoidales
    for (int x = startX; x < endX; x += 8) {
      int offset = (animFrame + x) % 12;
      int waveOffset = (offset < 6) ? offset - 3 : 3 - (offset - 6);
      tft.fillCircle(x, waveY + waveOffset, 2, waveColor);
    }
  }

  // Marco del tanque con bordes redondeados
  tft.drawRoundRect(TANK_X - 1, TANK_Y - 1, TANK_W + 2, TANK_H + 2, 8,
                    COLOR_TEXT);
  tft.drawRoundRect(TANK_X - 2, TANK_Y - 2, TANK_W + 4, TANK_H + 4, 10,
                    COLOR_TEXT_DIM);

  // Líneas de nivel (marcas) - centradas en cada sección
  for (int i = 1; i <= 7; i++) {
    int y = TANK_Y + TANK_H - (i * levelHeight) + (levelHeight / 2);
    tft.drawFastHLine(TANK_X - 10, y, 8, COLOR_TEXT_DIM);

    // Número de nivel
    tft.setTextFont(1);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(TANK_X - 20, y - 3);
    tft.print(i);
  }

  // Indicador de nivel actual (debajo del tanque)
  tft.fillRect(TANK_X - 5, TANK_Y + TANK_H + 5, TANK_W + 40, 25, COLOR_BG);
  tft.setTextFont(4);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(TANK_X + 5, TANK_Y + TANK_H + 8);
  tft.print(level);
  tft.setTextFont(2);
  tft.print("/7");

  // Incrementar frame de animación
  animFrame = (animFrame + 1) % 12;
}

// Dibujar ícono de bomba
void drawPumpIcon(int x, int y, bool running, uint16_t color) {
  // Cuerpo de la bomba (cilindro)
  tft.fillRoundRect(x, y + 8, 30, 20, 4, color);

  // Tubo de entrada (izquierda)
  tft.fillRect(x - 8, y + 14, 10, 8, color);

  // Tubo de salida (arriba)
  tft.fillRect(x + 10, y, 10, 10, color);

  // Motor (círculo)
  tft.fillCircle(x + 15, y + 18, 8, running ? COLOR_PUMP_ON : COLOR_TEXT_DIM);

  // Aspas del motor (rotan si está activa)
  if (running) {
    int angle = (animFrame * 30) % 360;
    int cx = x + 15;
    int cy = y + 18;

    // Dibujar 4 aspas rotando
    for (int i = 0; i < 4; i++) {
      int a = angle + (i * 90);
      int dx = 5 * cos(a * 3.14159 / 180);
      int dy = 5 * sin(a * 3.14159 / 180);
      tft.drawLine(cx, cy, cx + dx, cy + dy, COLOR_TEXT);
    }
  } else {
    // Cruz estática
    tft.drawFastHLine(x + 11, y + 18, 8, COLOR_TEXT);
    tft.drawFastVLine(x + 15, y + 14, 8, COLOR_TEXT);
  }

  // Gotas de agua saliendo (solo si está activa)
  if (running) {
    int dropY = y - 5 - (animFrame % 8);
    tft.fillCircle(x + 15, dropY, 2, 0x5D7F);
    tft.fillCircle(x + 12, dropY - 4, 1, 0x5D7F);
    tft.fillCircle(x + 18, dropY - 3, 1, 0x5D7F);
  }
}

void drawPumpStatus(PumpState state, unsigned long runTime) {
  int y = INFO_Y;

  // Limpiar área (reducida para no tapar ESTADO)
  tft.fillRect(INFO_X, y, SCREEN_W - INFO_X - 5, 70, COLOR_BG);

  // Estado de la bomba
  uint16_t statusColor;
  const char *statusText;
  bool isRunning = false;

  switch (state) {
  case PUMP_OFF:
    statusColor = COLOR_TEXT_DIM;
    statusText = "APAGADA";
    break;
  case PUMP_ON:
    statusColor = COLOR_PUMP_ON;
    statusText = "ACTIVA";
    isRunning = true;
    break;
  case PUMP_EMERGENCY:
    statusColor = COLOR_ERROR;
    statusText = "EMERGENCIA";
    isRunning = true;
    break;
  }

  // Dibujar ícono de bomba
  drawPumpIcon(INFO_X + 5, y, isRunning, statusColor);

  // Texto de estado
  tft.setTextFont(2);
  tft.setTextColor(statusColor, COLOR_BG);
  tft.setCursor(INFO_X + 45, y + 10);
  tft.print(statusText);

  // Tiempo de ejecución si está activa
  if (state != PUMP_OFF) {
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(INFO_X + 45, y + 28);

    unsigned long seconds = runTime / 1000;
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;

    char timeStr[16];
    sprintf(timeStr, "%02lu:%02lu", minutes, seconds);
    tft.print(timeStr);
  }
}

void drawSequenceStatus(SequenceState state, bool hasError) {
  int y = INFO_Y + 75; // Más arriba para no superponerse

  // Limpiar área
  tft.fillRect(INFO_X, y, SCREEN_W - INFO_X - 5, 45, COLOR_BG);

  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
  tft.setCursor(INFO_X, y);
  tft.print("ESTADO:");

  y += 20;

  uint16_t statusColor;
  const char *statusText;

  if (hasError) {
    statusColor = COLOR_ERROR;
    statusText = "ERROR!";
  } else {
    switch (state) {
    case SEQ_IDLE:
      statusColor = COLOR_OK;
      statusText = "NORMAL";
      break;
    case SEQ_FILLING:
      statusColor = 0x5D7F; // Azul
      statusText = "LLENANDO";
      break;
    case SEQ_EMPTYING:
      statusColor = COLOR_WARNING;
      statusText = "VACIANDO";
      break;
    default:
      statusColor = COLOR_TEXT_DIM;
      statusText = "---";
    }
  }

  tft.setTextColor(statusColor, COLOR_BG);
  tft.setCursor(INFO_X, y);
  tft.print(statusText);
}

void drawStats(int cycles, unsigned long lastCycle) {
  int y = STATS_Y;

  // Línea separadora
  tft.drawFastHLine(10, y - 10, SCREEN_W - 20, COLOR_HEADER);

  // Limpiar área
  tft.fillRect(10, y, SCREEN_W - 20, 80, COLOR_BG);

  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);

  // Ciclos hoy
  tft.setCursor(10, y);
  tft.print("Ciclos hoy: ");
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.print(cycles);

  // Último ciclo
  y += 25;
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
  tft.setCursor(10, y);
  tft.print("Ultimo ciclo: ");

  if (lastCycle > 0) {
    unsigned long seconds = lastCycle / 1000;
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    char timeStr[16];
    sprintf(timeStr, "%lum %02lus", minutes, seconds);
    tft.print(timeStr);
  } else {
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print("---");
  }
}

void drawErrorBanner(bool show) {
  int y = STATS_Y + 50; // Justo arriba del borde inferior

  if (show) {
    tft.fillRect(10, y, SCREEN_W - 20, 25, COLOR_ERROR);
    tft.setTextFont(2);
    tft.setTextColor(COLOR_TEXT, COLOR_ERROR);
    tft.setCursor(30, y + 5);
    tft.print("ERROR DE SECUENCIA");
  } else {
    tft.fillRect(10, y, SCREEN_W - 20, 25, COLOR_BG);
  }
}

void display_update(const DisplayData *data) {
  bool doFullRedraw = needsFullRedraw;

  if (doFullRedraw) {
    tft.fillScreen(COLOR_BG);
    drawHeader(data->wifiConnected);
    needsFullRedraw = false;
  }

  // Actualizar solo lo que cambió (o todo si es full redraw)
  if (doFullRedraw || data->level != lastData.level) {
    drawTank(data->level);
  }

  if (doFullRedraw || data->pumpState != lastData.pumpState ||
      (data->pumpState != PUMP_OFF &&
       data->pumpRunTime / 1000 != lastData.pumpRunTime / 1000)) {
    drawPumpStatus(data->pumpState, data->pumpRunTime);
  }

  if (doFullRedraw || data->sequenceState != lastData.sequenceState ||
      data->hasError != lastData.hasError) {
    drawSequenceStatus(data->sequenceState, data->hasError);
  }

  if (doFullRedraw || data->cyclesCompleted != lastData.cyclesCompleted ||
      data->lastCycleDuration != lastData.lastCycleDuration) {
    drawStats(data->cyclesCompleted, data->lastCycleDuration);
  }

  if (doFullRedraw || data->hasError != lastData.hasError) {
    drawErrorBanner(data->hasError);
  }

  if (data->wifiConnected != lastData.wifiConnected) {
    drawHeader(data->wifiConnected);
  }

  // Guardar estado actual
  memcpy(&lastData, data, sizeof(DisplayData));
}

void display_splash() {
  tft.fillScreen(COLOR_HEADER);

  // Logo/Icono grande
  int cx = SCREEN_W / 2;
  int cy = SCREEN_H / 2 - 40;

  // Gota de agua grande
  tft.fillCircle(cx, cy + 20, 40, 0x5D7F);
  tft.fillTriangle(cx, cy - 40, cx - 35, cy + 10, cx + 35, cy + 10, 0x5D7F);

  // Texto centrado
  tft.setTextDatum(TC_DATUM); // Top Center

  tft.setTextFont(4);
  tft.setTextColor(COLOR_TEXT, COLOR_HEADER);
  tft.drawString("AC MONITOR", cx, cy + 80);

  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_HEADER);
  tft.drawString("Sistema de Nivel", cx, cy + 115);

  tft.drawString("v1.0", cx, SCREEN_H - 30);

  tft.setTextDatum(TL_DATUM); // Volver a Top Left para el resto
}

void display_error(const char *message) {
  tft.fillScreen(COLOR_ERROR);

  tft.setTextFont(4);
  tft.setTextColor(COLOR_TEXT, COLOR_ERROR);
  tft.setCursor(70, 140);
  tft.print("ERROR");

  tft.setTextFont(2);
  tft.setCursor(20, 180);
  tft.print(message);
}

void display_force_redraw() { needsFullRedraw = true; }
