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
#define TANK_Y 60
#define TANK_W 60
#define TANK_H 140
#define INFO_X 110
#define INFO_Y 60
#define STATS_Y 220

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

void drawTank(int level) {
  // Marco del tanque
  tft.drawRect(TANK_X - 2, TANK_Y - 2, TANK_W + 4, TANK_H + 4, COLOR_TEXT);
  tft.drawRect(TANK_X - 1, TANK_Y - 1, TANK_W + 2, TANK_H + 2, COLOR_TEXT);

  // Fondo vacío
  tft.fillRect(TANK_X, TANK_Y, TANK_W, TANK_H, 0x1082);

  // Dibujar niveles de agua
  int levelHeight = TANK_H / 7;

  for (int i = 0; i < level; i++) {
    int y = TANK_Y + TANK_H - (i + 1) * levelHeight;
    uint16_t color = waterColors[i];
    tft.fillRect(TANK_X, y, TANK_W, levelHeight, color);
  }

  // Líneas de nivel (marcas)
  for (int i = 1; i <= 7; i++) {
    int y = TANK_Y + TANK_H - i * levelHeight;
    tft.drawFastHLine(TANK_X - 8, y, 6, COLOR_TEXT_DIM);

    // Número de nivel
    tft.setTextFont(1);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(TANK_X - 20, y - 4);
    tft.print(i);
  }

  // Indicador de nivel actual
  tft.setTextFont(4);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(TANK_X + 10, TANK_Y + TANK_H + 10);
  tft.print(level);
  tft.setTextFont(2);
  tft.print("/7");
}

void drawPumpStatus(PumpState state, unsigned long runTime) {
  int y = INFO_Y;

  // Limpiar área
  tft.fillRect(INFO_X, y, SCREEN_W - INFO_X - 5, 80, COLOR_BG);

  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
  tft.setCursor(INFO_X, y);
  tft.print("BOMBA:");

  y += 20;

  // Estado con color
  uint16_t statusColor;
  const char *statusText;

  switch (state) {
  case PUMP_OFF:
    statusColor = COLOR_TEXT_DIM;
    statusText = "APAGADA";
    break;
  case PUMP_ON:
    statusColor = COLOR_PUMP_ON;
    statusText = "ACTIVA";
    break;
  case PUMP_EMERGENCY:
    statusColor = COLOR_ERROR;
    statusText = "EMERGENCIA";
    break;
  }

  // Círculo indicador
  tft.fillCircle(INFO_X + 8, y + 8, 6, statusColor);

  tft.setTextFont(2);
  tft.setTextColor(statusColor, COLOR_BG);
  tft.setCursor(INFO_X + 20, y);
  tft.print(statusText);

  // Tiempo de ejecución si está activa
  if (state != PUMP_OFF) {
    y += 25;
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(INFO_X, y);

    unsigned long seconds = runTime / 1000;
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;

    char timeStr[16];
    sprintf(timeStr, "%02lu:%02lu", minutes, seconds);
    tft.print(timeStr);
  }
}

void drawSequenceStatus(SequenceState state, bool hasError) {
  int y = INFO_Y + 90;

  // Limpiar área
  tft.fillRect(INFO_X, y, SCREEN_W - INFO_X - 5, 50, COLOR_BG);

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
  int y = STATS_Y + 60;

  if (show) {
    tft.fillRect(10, y, SCREEN_W - 20, 30, COLOR_ERROR);
    tft.setTextFont(2);
    tft.setTextColor(COLOR_TEXT, COLOR_ERROR);
    tft.setCursor(40, y + 8);
    tft.print("ERROR DE SECUENCIA");
  } else {
    tft.fillRect(10, y, SCREEN_W - 20, 30, COLOR_BG);
  }
}

void display_update(const DisplayData *data) {
  if (needsFullRedraw) {
    tft.fillScreen(COLOR_BG);
    drawHeader(data->wifiConnected);
    needsFullRedraw = false;
  }

  // Actualizar solo lo que cambió
  if (needsFullRedraw || data->level != lastData.level) {
    drawTank(data->level);
  }

  if (needsFullRedraw || data->pumpState != lastData.pumpState ||
      (data->pumpState != PUMP_OFF &&
       data->pumpRunTime / 1000 != lastData.pumpRunTime / 1000)) {
    drawPumpStatus(data->pumpState, data->pumpRunTime);
  }

  if (needsFullRedraw || data->sequenceState != lastData.sequenceState ||
      data->hasError != lastData.hasError) {
    drawSequenceStatus(data->sequenceState, data->hasError);
  }

  if (needsFullRedraw || data->cyclesCompleted != lastData.cyclesCompleted ||
      data->lastCycleDuration != lastData.lastCycleDuration) {
    drawStats(data->cyclesCompleted, data->lastCycleDuration);
  }

  if (needsFullRedraw || data->hasError != lastData.hasError) {
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

  // Texto
  tft.setTextFont(4);
  tft.setTextColor(COLOR_TEXT, COLOR_HEADER);
  tft.setCursor(40, cy + 80);
  tft.print("AC MONITOR");

  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_HEADER);
  tft.setCursor(50, cy + 115);
  tft.print("Sistema de Nivel");

  tft.setCursor(100, SCREEN_H - 30);
  tft.print("v1.0");
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
