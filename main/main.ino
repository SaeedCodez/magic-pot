#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SENSOR_AO     A0
#define BUZZER_PIN    12   // GPIO12 (D6)
#define THRESHOLD_DRY 30
#define THRESHOLD_WET 70

// Thirsty melody — notes: {frequency Hz, duration ms}
// 0 Hz = rest. Plays on loop while in DRY state.
const int MELODY[][2] = {
  {659, 200},  // E5
  {587, 200},  // D5
  {523, 200},  // C5
  {494, 200},  // B4
  {440, 400},  // A4 (held)
  {  0, 200},  // rest
  {392, 200},  // G4
  {440, 200},  // A4
  {494, 200},  // B4
  {523, 600},  // C5 (held)
  {  0, 500},  // long rest before repeat
};
const int MELODY_LEN = 11;

// Face layout
#define EYE_LX  36   // left eye center x
#define EYE_RX  92   // right eye center x
#define EYE_Y   30   // eye center y
#define EYE_RAD  9   // eye radius
#define MOUTH_X 64   // mouth center x
#define MOUTH_Y 50   // mouth center y

// Timing
const unsigned long READ_INTERVAL  = 1000;  // sensor read interval
const unsigned long BLINK_INTERVAL = 3500;  // time between blinks
const unsigned long BLINK_DURATION =  130;  // how long eyes stay closed

unsigned long lastRead    = 0;
unsigned long lastBlink   = 0;
unsigned long noteStart   = 0;
unsigned long dryEnteredAt = 0;
bool isBlinking  = false;
int  moisture    = 50;
int  state       = 1;   // 0=DRY, 1=OK, 2=WET
int  prevState   = 1;
bool lastInvert  = false;
int  noteIndex   = 0;

const unsigned long MELODY_DURATION = 5000;  // play melody for 5s on DRY entry

// Arc helper: Smile (∪) = angles 20-160, Frown (∩) = angles 200-340
void drawArc(int cx, int cy, int r, int startDeg, int endDeg) {
  for (int a = startDeg; a <= endDeg; a += 3) {
    float rad = a * PI / 180.0;
    int x = cx + (int)(r * cos(rad));
    int y = cy + (int)(r * sin(rad));
    display.drawPixel(x, y,     WHITE);
    display.drawPixel(x, y + 1, WHITE);
  }
}

// Draws closed eyes (thin horizontal line) used during blink
void drawEyesClosed() {
  display.fillRect(EYE_LX - EYE_RAD, EYE_Y - 2, EYE_RAD * 2, 4, WHITE);
  display.fillRect(EYE_RX - EYE_RAD, EYE_Y - 2, EYE_RAD * 2, 4, WHITE);
}

// WET: big shiny eyes + smile + water drops
void drawWetFace(bool blinking) {
  display.drawRoundRect(2, 2, 124, 60, 12, WHITE);

  // Water drops in yellow zone
  display.fillCircle(20, 10, 3, WHITE);
  display.fillTriangle(18, 10, 22, 10, 20, 5, WHITE);
  display.fillCircle(64, 10, 3, WHITE);
  display.fillTriangle(62, 10, 66, 10, 64, 5, WHITE);
  display.fillCircle(108, 10, 3, WHITE);
  display.fillTriangle(106, 10, 110, 10, 108, 5, WHITE);

  if (blinking) {
    drawEyesClosed();
  } else {
    // Big shiny eyes
    display.fillCircle(EYE_LX, EYE_Y, EYE_RAD, WHITE);
    display.fillCircle(EYE_LX - 3, EYE_Y - 3, 2, BLACK);
    display.fillCircle(EYE_RX, EYE_Y, EYE_RAD, WHITE);
    display.fillCircle(EYE_RX - 3, EYE_Y - 3, 2, BLACK);
  }

  // Smile (∪)
  drawArc(MOUTH_X, MOUTH_Y - 8, 10, 20, 160);
  drawArc(MOUTH_X, MOUTH_Y - 8, 11, 20, 160);
}

// DRY: filled droopy eyes + frown + sweat drop (display inverted for alarm)
// With invertDisplay(true): BLACK pixels appear bright = blends with background
void drawDryFace(bool blinking) {
  display.drawRoundRect(2, 2, 124, 60, 12, WHITE);

  if (blinking) {
    drawEyesClosed();
  } else {
    // Filled droopy eyes: circle WHITE + top masked BLACK (appears bright = open lid)
    display.fillCircle(EYE_LX, EYE_Y, EYE_RAD, WHITE);
    display.fillRect(EYE_LX - EYE_RAD - 1, EYE_Y - EYE_RAD - 1,
                     EYE_RAD * 2 + 2, EYE_RAD - 1, BLACK);
    display.drawLine(EYE_LX - EYE_RAD, EYE_Y + 2,
                     EYE_LX + EYE_RAD, EYE_Y - 2, WHITE);  // sad eyelid ↗

    display.fillCircle(EYE_RX, EYE_Y, EYE_RAD, WHITE);
    display.fillRect(EYE_RX - EYE_RAD - 1, EYE_Y - EYE_RAD - 1,
                     EYE_RAD * 2 + 2, EYE_RAD - 1, BLACK);
    display.drawLine(EYE_RX - EYE_RAD, EYE_Y - 2,
                     EYE_RX + EYE_RAD, EYE_Y + 2, WHITE);  // sad eyelid ↘
  }

  // Frown (∩)
  drawArc(MOUTH_X, MOUTH_Y + 6, 9, 200, 340);
  drawArc(MOUTH_X, MOUTH_Y + 6, 10, 200, 340);

  // Sweat drop
  display.fillCircle(EYE_RX + 14, EYE_Y - 2, 2, WHITE);
  display.drawLine(EYE_RX + 14, EYE_Y - 4, EYE_RX + 16, EYE_Y - 8, WHITE);
}

// OK: closed ^^ eyes + gentle smile
void drawOkFace(bool blinking) {
  display.drawRoundRect(2, 2, 124, 60, 12, WHITE);

  // Yellow zone: subtle dots
  display.fillCircle(32, 10, 2, WHITE);
  display.fillCircle(64, 10, 2, WHITE);
  display.fillCircle(96, 10, 2, WHITE);

  if (blinking) {
    drawEyesClosed();
  } else {
    // Closed ^^ eyes (∩ = ^ character)
    drawArc(EYE_LX, EYE_Y + 4, 9,  200, 340);
    drawArc(EYE_LX, EYE_Y + 4, 10, 200, 340);
    drawArc(EYE_RX, EYE_Y + 4, 9,  200, 340);
    drawArc(EYE_RX, EYE_Y + 4, 10, 200, 340);
  }

  // Gentle smile (∪)
  drawArc(MOUTH_X, MOUTH_Y - 6, 7, 20, 160);
  drawArc(MOUTH_X, MOUTH_Y - 6, 8, 20, 160);
}

void updateMelody() {
  // Detect DRY entry to start the 5s window
  if (state == 0 && prevState != 0) {
    dryEnteredAt = millis();
    noteIndex    = 0;
    noteStart    = millis();
  }
  prevState = state;

  // Only play within the 5-second window
  if (state != 0 || (millis() - dryEnteredAt >= MELODY_DURATION)) {
    noTone(BUZZER_PIN);
    return;
  }

  unsigned long now = millis();
  if (now - noteStart >= (unsigned long)MELODY[noteIndex][1]) {
    noteStart = now;
    noteIndex = (noteIndex + 1) % MELODY_LEN;
    int freq  = MELODY[noteIndex][0];
    if (freq > 0) tone(BUZZER_PIN, freq);
    else          noTone(BUZZER_PIN);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED init failed — check wiring");
    while (true);
  }

  display.clearDisplay();
  display.display();
}

void loop() {
  unsigned long now = millis();

  // Read sensor every second
  if (now - lastRead >= READ_INTERVAL) {
    lastRead = now;
    int raw = constrain(analogRead(SENSOR_AO), 0, 1023);
    moisture = map(raw, 1023, 0, 0, 100);

    if      (moisture <= THRESHOLD_DRY) state = 0;
    else if (moisture >= THRESHOLD_WET) state = 2;
    else                                state = 1;

    const char* level[] = { "DRY", "OK", "WET" };
    Serial.print("Moisture: ");
    Serial.print(moisture);
    Serial.print("%  |  Raw: ");
    Serial.print(raw);
    Serial.print("  |  Level: ");
    Serial.println(level[state]);
  }

  // Blink trigger: start blink
  if (!isBlinking && (now - lastBlink >= BLINK_INTERVAL)) {
    isBlinking = true;
    lastBlink  = now;
  }
  // End blink after duration
  if (isBlinking && (now - lastBlink >= BLINK_DURATION)) {
    isBlinking = false;
    lastBlink  = now;  // reset interval from here
  }

  // Redraw face
  display.clearDisplay();
  if      (state == 0) drawDryFace(isBlinking);
  else if (state == 2) drawWetFace(isBlinking);
  else                 drawOkFace(isBlinking);
  display.display();

  // Only send invertDisplay command when state changes (avoids redundant I2C writes)
  bool shouldInvert = (state == 0);
  if (shouldInvert != lastInvert) {
    display.invertDisplay(shouldInvert);
    lastInvert = shouldInvert;
  }

  updateMelody();
  delay(20);
}
