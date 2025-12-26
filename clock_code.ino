#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TM1637Display.h>
#include <Adafruit_SHT31.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= RTC =================
RTC_DS3231 rtc;

// ================= TM1637 =================
#define CLK 18
#define DIO 19
TM1637Display seg(CLK, DIO);

// ================= SHT31 =================
Adafruit_SHT31 sht31;

// ================= BUZZER =================
#define BUZZER 25

// ================= TIMER =================
unsigned long prevMillis = 0;
const unsigned long interval = 1000;

// ================= ALARM =================
bool alarmActive = false;
bool alarmFreeze = false;
unsigned long alarmStart = 0;
const unsigned long alarmDuration = 60000;

bool alarm1Triggered = false;
bool alarm2Triggered = false;
const char* alarmText = "";

// ================= HARI =================
const char* dayNameID[] = {
  "MIN", "SEN", "SEL", "RAB", "KAM", "JUM", "SAB"
};

// ================= TM1637 SYMBOL =================
const uint8_t DEGREE = SEG_A | SEG_B | SEG_F | SEG_G;
const uint8_t LETTER_C = SEG_A | SEG_D | SEG_E | SEG_F;

// ================= BUZZER NON BLOCKING =================
unsigned long buzzerPrev = 0;
byte buzzerStep = 0;
int nada[] = {900, 1100, 1300};

void buzzerUpdate() {
  if (!alarmActive) return;

  if (millis() - buzzerPrev >= 200) {
    buzzerPrev = millis();
    if (buzzerStep < 3) {
      tone(BUZZER, nada[buzzerStep]);
      buzzerStep++;
    } else {
      noTone(BUZZER);
      buzzerStep = 0;
    }
  }
}

// ================= ALARM UPDATE =================
void alarmUpdate() {
  if (!alarmActive) return;

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 8);
  display.print(alarmText);
  display.display();

  buzzerUpdate();

  if (millis() - alarmStart >= alarmDuration) {
    alarmActive = false;
    alarmFreeze = false;
    noTone(BUZZER);
  }
}

void setup() {
  Wire.begin(21, 22);
  pinMode(BUZZER, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextColor(SSD1306_WHITE);

  rtc.begin();
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  seg.setBrightness(0x0f);
  seg.clear();

  sht31.begin(0x44);
}

void loop() {
  DateTime now = rtc.now();

  // ===== UPDATE JAM (HANYA JIKA TIDAK FREEZE) =====
  if (!alarmFreeze && millis() - prevMillis >= interval) {
    prevMillis = millis();

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(14, 0);

    if (now.hour() < 10) display.print('0');
    display.print(now.hour());
    display.print(':');

    if (now.minute() < 10) display.print('0');
    display.print(now.minute());
    display.print(':');

    if (now.second() < 10) display.print('0');
    display.print(now.second());

    display.setTextSize(1);
    display.setCursor(22, 22);
    display.print(dayNameID[now.dayOfTheWeek()]);
    display.print(", ");
    display.print(now.day());
    display.print("/");
    display.print(now.month());
    display.print("/");
    display.print(now.year() % 100);

    display.display();

    // ===== SUHU =====
    float suhu = sht31.readTemperature();
    if (!isnan(suhu)) {
      int t = round(suhu);
      uint8_t data[] = {
        seg.encodeDigit(t / 10),
        seg.encodeDigit(t % 10),
        DEGREE,
        LETTER_C
      };
      seg.setSegments(data);
    }
  }

  // ===== TRIGGER ALARM =====
  if (now.hour() == 5 && now.minute() == 0 && !alarm1Triggered) {
    alarmActive = true;
    alarmFreeze = true;
    alarmStart = millis();
    alarmText = "ALARM!";
    alarm1Triggered = true;
  }

  if (now.hour() == 16 && now.minute() == 7 && !alarm2Triggered) {
    alarmActive = true;
    alarmFreeze = true;
    alarmStart = millis();
    alarmText = "BANGUN!";
    alarm2Triggered = true;
  }

  // reset harian
  if (now.hour() == 0 && now.minute() == 0) {
    alarm1Triggered = false;
    alarm2Triggered = false;
  }

  // ===== ALARM DISPLAY =====
  alarmUpdate();
}