#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

extern "C" {
#include "user_interface.h"
}

// ===== SETTINGS ===== //
#define LED 2
#define BUZZER_PIN D1
#define LED_INVERT true
#define SERIAL_BAUD 115200
#define CH_TIME 140
#define PKT_RATE 5
#define PKT_TIME 1
#define BUZZER_THRESHOLD 5
#define BUZZER_DURATION 50000
#define BOOT_IGNORE_TIME 5000  // 5s delay to ignore false attacks at startup



const short channels[] = { 1,2,3,4,5,6,7,8,9,10,11,12,13 };

// ===== Runtime variables ===== //
int ch_index = 0;
int packet_rate = 0;
unsigned long update_time = 0;
unsigned long ch_time = 0;
unsigned long buzzer_start_time = 0;
bool buzzer_active = false;
bool attack_detected = false;
unsigned long attack_start_time = 0;
unsigned long boot_time = 0;

bool display_initialized = false;

#define TFT_CS   D8
#define TFT_DC   D2
#define TFT_RST  D4

Adafruit_ST7735 tft = Adafruit_ST7735(
  TFT_CS,
  TFT_DC,
  TFT_RST
);

// ===== Display Setup ===== //
void setupDisplay() {

  tft.initR(INITR_BLACKTAB);  
  tft.setRotation(0);

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(10,10);
  tft.println("DeAuth");

  tft.setCursor(10,35);
  tft.println("Detector");

  tft.drawRect(5,60,118,60,ST77XX_WHITE);

  display_initialized = true;
}


// ===== Draw Screens ===== //
void drawNormalScreen(){

 if(!display_initialized) return;

 tft.fillRect(6,61,116,58,ST77XX_BLACK);

 tft.setTextColor(ST77XX_GREEN);
 tft.setTextSize(1);

 tft.setCursor(15,70);
 tft.println("NO ATTACK");

 tft.setCursor(10,90);
 tft.print("Pkts/s: ");
 tft.println(packet_rate);

 tft.setCursor(10,105);
 tft.print("CH: ");
 tft.println(channels[ch_index]);
}

void drawAttackScreen(){

 if(!display_initialized) return;

 tft.fillRect(6,61,116,58,ST77XX_RED);

 tft.setTextColor(ST77XX_WHITE);
 tft.setTextSize(1);

 tft.setCursor(8,70);
 tft.println("ATTACK!");

 tft.setCursor(8,90);
 tft.print("Time:");

 tft.print((millis()-attack_start_time)/1000);
 tft.println("s");
}

void updateDisplay() {
  static unsigned long last_update = 0;
  if (millis() - last_update < 500) return;
  last_update = millis();

  if (attack_detected) {
    drawAttackScreen();
  } else {
    drawNormalScreen();
  }
}

// ===== Sniffer ===== //
void sniffer(uint8_t *buf, uint16_t len) {
  if (!buf || len < 28) return;
  byte pkt_type = buf[12];
  if (pkt_type == 0xA0 || pkt_type == 0xC0) {
    ++packet_rate;
  }
}

// ===== Buzzer ===== //
void activate_buzzer() {
  if (!buzzer_active) {
    digitalWrite(BUZZER_PIN, LOW);   // ON
    buzzer_start_time = millis();
    buzzer_active = true;
    Serial.println("BUZZER ACTIVATED");
  }
}

void deactivate_buzzer() {
  if (buzzer_active) {
    digitalWrite(BUZZER_PIN, HIGH);  // OFF
    buzzer_active = false;
    Serial.println("BUZZER OFF");
  }
}

void check_buzzer_timeout() {
  if (buzzer_active && (millis() - buzzer_start_time >= BUZZER_DURATION)) {
    deactivate_buzzer();
  }
}

// ===== Attack Logic ===== //
void attack_started() {
  if (!attack_detected) {
    digitalWrite(LED, !LED_INVERT);
    Serial.println("ATTACK DETECTED");
    attack_detected = true;
    attack_start_time = millis();
    drawAttackScreen();
    if (packet_rate >= BUZZER_THRESHOLD) {
      activate_buzzer();
    }
  }
}

void attack_stopped() {
  if (attack_detected) {
    digitalWrite(LED, LED_INVERT);
    Serial.println("ATTACK STOPPED");
    attack_detected = false;
    drawNormalScreen();
    deactivate_buzzer();
  }
}

// ===== Setup ===== //
void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LED_INVERT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);   // Keep buzzer OFF at startup
  setupDisplay();

  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_set_promiscuous_rx_cb(sniffer);
  wifi_set_channel(channels[0]);
  wifi_promiscuous_enable(true);

  tft.fillRect(6, 61, 116, 58, ST77XX_BLACK);

 tft.setTextColor(ST77XX_GREEN);
 tft.setTextSize(2);
 tft.setCursor(40, 70);
 tft.print("READY");

 tft.setTextColor(ST77XX_WHITE);
 tft.setTextSize(1);
 tft.setCursor(20, 90);
 tft.print("Monitoring...");

  Serial.println("\nDeAuth Detector Ready.");
  boot_time = millis();
}

// ===== Loop ===== //
void loop() {
  unsigned long current_time = millis();

  check_buzzer_timeout();
  updateDisplay();

  // Avoid false triggers for 5s
  if (current_time - boot_time < BOOT_IGNORE_TIME) return;

  // Packet check every full cycle
  if (current_time - update_time >= (sizeof(channels)*CH_TIME)) {
    update_time = current_time;

    if (packet_rate >= PKT_RATE) {
      attack_started();
    } else {
      attack_stopped();
    }

    Serial.print("Deauth packets/s: ");
    Serial.println(packet_rate);
    packet_rate = 0;
  }

  // Channel hopping
  if (sizeof(channels) > 1 && current_time - ch_time >= CH_TIME) {
    ch_time = current_time;
    ch_index = (ch_index + 1) % (sizeof(channels)/sizeof(channels[0]));
    wifi_set_channel(channels[ch_index]);
  }
}
