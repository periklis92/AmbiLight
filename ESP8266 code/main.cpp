#include <Arduino.h> // Needed for Platformio, i don't think it is needed for Arduino IDE
#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <EEPROM.h>
#include <string>

#define PORT_NUM 11400 // Server port

//Simple instructions to communicate through the serial port
#define INST_CREDENTIALS "$0001"
#define INST_GETIP "$0003"

//Return codes for the serial port
#define CODE_OK "$1000"
#define CODE_WAIT "$2000"
#define CODE_DENIED "$3000"
#define CODE_READY "$4000"
#define CODE_ERROR "$5000"

#define MAX_TRIES 20 // Wifi connect timeout
#define NUM_LEDS 18

//Left side LEDS START AND END INDEX (0-6)
#define LED_LS 0
#define LED_LE NUM_LEDS / 3
//Front side (6-12)
#define LED_FS NUM_LEDS / 3
#define LED_FE 2 * NUM_LEDS / 3
//Right side (12-18)
#define LED_RS 2 * NUM_LEDS / 3
#define LED_RE NUM_LEDS

//LED signal pin
#define DATA_PIN D2
//Tactile switch to change between modes
#define BUTTON_PIN D6
//Interpolation between colors
#define SMOOTHNESS 20.0f

typedef enum lightState_t{
  CONFIG, CONNECTED, WAITING
} lightState_t;

void listenServer();
void configure(String s);
void changeLeds(int startIndex , int endIndex, CRGB color);
void ledsLeft(CRGB color);
void ledsRight(CRGB color);
void ledsFront(CRGB color);
void confLeds();
void checkColors();
void changeLed(int index, CRGB color);
void ICACHE_RAM_ATTR toggleConfig();
bool connectWiFi(String ssid, String wpa);
void successLeds();
void initLeds();
CRGB smoothColor(CRGB from, CRGB to, float t);

lightState_t curLights;
volatile bool config_mode = true;
volatile bool isConfigured = false;
bool config_authetication = false;

WiFiServer server(PORT_NUM);
CRGB leds[NUM_LEDS];

CRGB colL = CRGB::White;
CRGB prevcolL = CRGB::White;
CRGB colR = CRGB::White;
CRGB prevcolR = CRGB::White;
CRGB colF = CRGB::White;
CRGB prevcolF = CRGB::White;
int pos = 0;

struct {
  char ssid[20] = "";
  char wpa[20] = "";
} credentials;

uint8_t eepromAddr = 0;
volatile unsigned long button_counter = 0;
volatile bool pressed = false;

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  delay(10);
  initLeds();
  Serial.begin(74880);
  Serial.setTimeout(100);
  EEPROM.begin(256);
  EEPROM.get(eepromAddr, credentials);
  if (strlen(credentials.ssid) > 0 && strlen(credentials.ssid) > 0) {
    if (connectWiFi(credentials.ssid, credentials.wpa) && WiFi.status() == WL_CONNECTED) {
      isConfigured = true;
      config_mode = false;
      successLeds();
    } else {
      confLeds();
    }
  } else {
    confLeds();
  }
  pinMode(BUTTON_PIN, INPUT);
  delay(10);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), toggleConfig, CHANGE);
  delay(10);
  interrupts();
}

void toggleConfig() {
  if (!pressed) {
    pressed = true;
    button_counter = millis();
  } else {
    if (millis() - button_counter > 250) {
      config_mode = !config_mode;
      pressed = false;
      button_counter = millis();
    }
  }
}

void getTokens(String& s, String* buffer, char delim=':') {
  String* tempBuffer = new String[10];
  int delimInd = 0;
  int curIndex = 0;
  for (unsigned int i = 0; i < s.length(); i++) {
    int nextDelimInd = s.indexOf(delim, delimInd + 1);
    if (nextDelimInd < 0) break;
    String t = s.substring(delimInd + 1, nextDelimInd);
    tempBuffer[curIndex++] = t;
    delimInd = nextDelimInd;
  }
  for (int i = 0; i < curIndex + 1; ++i) {
    buffer[i] = tempBuffer[i];
  }
}

void loop() {
  if (config_mode){
    if (curLights != CONFIG) confLeds();
    if (Serial.available() > 0) {
      String s = Serial.readString();
      if (!s.isEmpty()){
        configure(s);
      }
    }
  } else {
    if (curLights != CONNECTED) successLeds();
    if (isConfigured) {
      if (WiFi.status() == WL_CONNECTED) {
        listenServer();
      } else {
        config_mode = true;
      }
    } else {
      config_mode = true;
    }
  }
}

void configure(String s) {

  String* toks = new String[5];
  getTokens(s, toks);
  if (toks[0] == INST_CREDENTIALS) {
    connectWiFi(toks[1], toks[2]);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(CODE_OK);
    } else {
      Serial.print(CODE_ERROR);
    }
  } else if (toks[0] == INST_GETIP) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(WiFi.localIP().toString());
      config_mode = false;    
    }
    else Serial.print(CODE_ERROR);
    } else {
    Serial.print(CODE_ERROR);
  }
}

void listenServer() {
  WiFiClient client = server.available();
  if (client) {
    client.setNoDelay(true);
    while(client.connected()) {
      if (config_mode == true) {
        client.stopAll();
        break;
      }
      while(client.available() > 0) {
        uint8* req = new uint8_t[2];
        client.readBytes(req, 2);
        if (req[0] == 0 && req[1] == 1) {
          uint8* l = new uint8_t[3];
          uint8* f = new uint8_t[3];
          uint8* r = new uint8_t[3];
          client.readBytes(l, 3);
          client.write('O');
          client.readBytes(f, 3);
          client.write('O');
          client.readBytes(r, 3);
          client.write('O');
          prevcolF = colF;
          prevcolL = colL;
          prevcolR = colR;
          pos = 1;
          colL = (CRGB(l[0], l[1], l[2]));
          colF = (CRGB(f[0], f[1], f[2]));
          colR = (CRGB(r[0], r[1], r[2]));
        }
      }
      ledsFront(smoothColor(prevcolF, colF, pos / SMOOTHNESS));
      ledsRight(smoothColor(prevcolR, colR, pos / SMOOTHNESS));
      ledsLeft(smoothColor(prevcolL, colL, pos / SMOOTHNESS));
      if (pos < SMOOTHNESS) pos++;
    }

    client.stop();
  }
}

bool connectWiFi(String ssid, String wpa) {
  uint8_t tries = 0; 
  if (WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, wpa);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      tries++;
      if (tries >= MAX_TRIES) return false;
    }
    strcpy(credentials.ssid, ssid.c_str());
    strcpy(credentials.wpa, wpa.c_str());
    EEPROM.put(eepromAddr, credentials);
    EEPROM.commit();
    server.begin();
  }
  return true;
}

void changeLeds(int startIndex , int endIndex, CRGB color) {
    for (int i = startIndex; i < endIndex; i++) {
      leds[i] = color;
    }
    FastLED.show();
}

CRGB smoothColor(CRGB from, CRGB to, float t) {
  uint8_t finalr = t * (float)to.r + (1.0f - t) * (float)from.r;
  uint8_t finalg = t * (float)to.g + (1.0f - t) * (float)from.g;
  uint8_t finalb = t * (float)to.b + (1.0f - t) * (float)from.b;

  return (CRGB(finalr, finalg, finalb));
}

void changeLed(int index, CRGB color) {
  leds[index] = color;
  FastLED.show();
}
void ledsLeft(CRGB color) {
  changeLeds(LED_LS, LED_LE, color);
}

void ledsRight(CRGB color) {
  changeLeds(LED_RS, LED_RE, color);
}

void ledsFront(CRGB color) {
  changeLeds(LED_FS, LED_FE, color);
}

void confLeds() {
  changeLeds(0, NUM_LEDS, CRGB::BlueViolet);
  curLights = CONFIG;
}

void successLeds() {
  changeLeds(0, NUM_LEDS, CRGB::Green);
  curLights = CONNECTED;
}

void initLeds() {
  changeLeds(0, NUM_LEDS, CRGB::FloralWhite);
  curLights = WAITING;
}