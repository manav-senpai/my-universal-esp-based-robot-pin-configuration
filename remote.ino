#include <esp_now.h>
#include <WiFi.h>

// --- REPLACE WI' ARCEE'S MAC ADDRESS! ---
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ==========================================
// --- T' MAGIC FLIP SWITCHES ---
// If summat moves backwards when it should go forward, 
// joost change 'false' to 'true' here! No solderin' needed.
const bool FLIP_LEFT_MOTOR = false;
const bool FLIP_RIGHT_MOTOR = false;
const bool FLIP_JOY_X = false;
const bool FLIP_JOY_Y = false;
// ==========================================

// --- T' Pins (Yer exact updated layout) ---
const int JOY_X = 34;
const int JOY_Y = 35;
const int JOY_BTN = 21;

const int BTN_L_FWD = 14;
const int BTN_L_REV = 27;
const int BTN_R_FWD = 23;
const int BTN_R_REV = 13; 

const int BTN_SPD_UP = 22;
const int BTN_SPD_DN = 19;

const int BTN_ALL_FWD = 33;
const int BTN_ALL_REV = 32;
const int BTN_ALL_LFT = 26;
const int BTN_ALL_RGT = 25;

const int LED_SPEED = 4;
const int LED_CONN = 18; // T' Blue LED

// --- T' State Variables ---
int speedLevel = 1; 
bool freeMode = false;
bool lastJoyBtn = HIGH;
bool lastSpdUp = HIGH;
bool lastSpdDn = HIGH;
bool isConnected = false;
unsigned long lastBlinkTime = 0;
bool connLedState = LOW;

// --- T' Data Packet ---
typedef struct struct_message {
  int leftMotor;
  int rightMotor;
} struct_message;

struct_message command;
esp_now_peer_info_t peerInfo;

// --- Callbacks ---
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  isConnected = (status == ESP_NOW_SEND_SUCCESS);
}

void flashSpeedLED(int times) {
  for(int i = 0; i < times; i++) {
    digitalWrite(LED_SPEED, HIGH); delay(60);
    digitalWrite(LED_SPEED, LOW); delay(60);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(JOY_BTN, INPUT_PULLUP);
  pinMode(BTN_L_FWD, INPUT_PULLUP); pinMode(BTN_L_REV, INPUT_PULLUP);
  pinMode(BTN_R_FWD, INPUT_PULLUP); pinMode(BTN_R_REV, INPUT_PULLUP);
  pinMode(BTN_SPD_UP, INPUT_PULLUP); pinMode(BTN_SPD_DN, INPUT_PULLUP);
  pinMode(BTN_ALL_FWD, INPUT_PULLUP); pinMode(BTN_ALL_REV, INPUT_PULLUP);
  pinMode(BTN_ALL_LFT, INPUT_PULLUP); pinMode(BTN_ALL_RGT, INPUT_PULLUP);

  pinMode(LED_SPEED, OUTPUT);
  pinMode(LED_CONN, OUTPUT);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;
  
  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  // 1. T' Remote Connection LED
  if (!isConnected) {
    if (millis() - lastBlinkTime > 300) {
      connLedState = !connLedState;
      digitalWrite(LED_CONN, connLedState);
      lastBlinkTime = millis();
    }
  } else {
    digitalWrite(LED_CONN, HIGH); 
  }

  // 2. Free Mode Toggle
  bool currentJoyBtn = digitalRead(JOY_BTN);
  if (lastJoyBtn == HIGH && currentJoyBtn == LOW) {
    freeMode = !freeMode;
    if(freeMode) { digitalWrite(LED_SPEED, HIGH); delay(500); digitalWrite(LED_SPEED, LOW); }
    else { flashSpeedLED(speedLevel); }
  }
  lastJoyBtn = currentJoyBtn;

  // 3. Speed Buttons
  bool currentSpdUp = digitalRead(BTN_SPD_UP);
  if (lastSpdUp == HIGH && currentSpdUp == LOW && !freeMode) {
    if (speedLevel < 5) speedLevel++;
    flashSpeedLED(speedLevel);
  }
  lastSpdUp = currentSpdUp;

  bool currentSpdDn = digitalRead(BTN_SPD_DN);
  if (lastSpdDn == HIGH && currentSpdDn == LOW && !freeMode) {
    if (speedLevel > 1) speedLevel--;
    flashSpeedLED(speedLevel);
  }
  lastSpdDn = currentSpdDn;

  int maxPower = freeMode ? 255 : (speedLevel * 51);

  int reqLeft = 0;
  int reqRight = 0;

  // 4. Read all Buttons
  bool allFwd = (digitalRead(BTN_ALL_FWD) == LOW);
  bool allRev = (digitalRead(BTN_ALL_REV) == LOW);
  bool allLft = (digitalRead(BTN_ALL_LFT) == LOW);
  bool allRgt = (digitalRead(BTN_ALL_RGT) == LOW);
  
  bool lFwd = (digitalRead(BTN_L_FWD) == LOW);
  bool lRev = (digitalRead(BTN_L_REV) == LOW);
  bool rFwd = (digitalRead(BTN_R_FWD) == LOW);
  bool rRev = (digitalRead(BTN_R_REV) == LOW);

  // Is ANY button pressed?
  bool anyOverride = allFwd || allRev || allLft || allRgt || lFwd || lRev || rFwd || rRev;

  if (anyOverride) {
    // BUTTON MODE: Kills t' joystick completely so it can't interfere
    if (allFwd && allLft) { reqLeft = maxPower / 2; reqRight = maxPower; } 
    else if (allFwd && allRgt) { reqLeft = maxPower; reqRight = maxPower / 2; } 
    else if (allRev && allLft) { reqLeft = -maxPower / 2; reqRight = -maxPower; } 
    else if (allRev && allRgt) { reqLeft = -maxPower; reqRight = -maxPower / 2; } 
    else if (allFwd) { reqLeft = maxPower; reqRight = maxPower; }
    else if (allRev) { reqLeft = -maxPower; reqRight = -maxPower; }
    else if (allLft) { reqLeft = -maxPower; reqRight = maxPower; } 
    else if (allRgt) { reqLeft = maxPower; reqRight = -maxPower; } 
    else {
      // Individual tracks safely isolated
      if (lFwd && lRev) reqLeft = 0; 
      else if (lFwd) reqLeft = maxPower;
      else if (lRev) reqLeft = -maxPower;

      if (rFwd && rRev) reqRight = 0;
      else if (rFwd) reqRight = maxPower;
      else if (rRev) reqRight = -maxPower;
    }
  } else {
    // JOYSTICK MODE: Only runs if NO buttons are pressed
    int rawX = analogRead(JOY_X);
    int rawY = analogRead(JOY_Y);
    int outX = 0, outY = 0;

    // Deadzone math that maps purely from center to edges
    if (rawX > 2022) outX = map(rawX, 2022, 4095, 0, maxPower); 
    else if (rawX < 1722) outX = map(rawX, 1722, 0, 0, -maxPower); 

    if (rawY > 1975) outY = map(rawY, 1975, 4095, 0, maxPower); 
    else if (rawY < 1675) outY = map(rawY, 1675, 0, 0, -maxPower); 

    // Apply Magic Joystick Flips
    if (FLIP_JOY_X) outX = -outX;
    if (FLIP_JOY_Y) outY = -outY;

    reqLeft = outY + outX;
    reqRight = outY - outX;
  }

  // 5. Apply Magic Motor Flips (Fixes backwards wiring instantly)
  if (FLIP_LEFT_MOTOR) reqLeft = -reqLeft;
  if (FLIP_RIGHT_MOTOR) reqRight = -reqRight;

  // 6. Lock it down and fire!
  command.leftMotor = constrain(reqLeft, -maxPower, maxPower);
  command.rightMotor = constrain(reqRight, -maxPower, maxPower);

  esp_now_send(broadcastAddress, (uint8_t *) &command, sizeof(command));
  delay(20); 
}
