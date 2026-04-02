#include <esp_now.h>
#include <WiFi.h>

const int L_RPWM = 26;
const int L_LPWM = 25;
const int R_RPWM = 27;
const int R_LPWM = 14;

const int LED_GREEN = 33;  // Solid when connected
const int LED_YELLOW = 2;  // Blinks when lost

typedef struct struct_message {
  int leftMotor;
  int rightMotor;
} struct_message;

struct_message incomingCmd;

unsigned long lastRecvTime = 0;
unsigned long lastBlinkTime = 0;
bool yellowState = LOW;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingCmd, incomingData, sizeof(incomingCmd));
  lastRecvTime = millis(); 

  // T' 10% Noise Filter (Stops whine and stall)
  if (abs(incomingCmd.leftMotor) < 26) incomingCmd.leftMotor = 0;
  if (abs(incomingCmd.rightMotor) < 26) incomingCmd.rightMotor = 0;

  if (incomingCmd.leftMotor > 0) {
    analogWrite(L_RPWM, incomingCmd.leftMotor);
    analogWrite(L_LPWM, 0);
  } else {
    analogWrite(L_RPWM, 0);
    analogWrite(L_LPWM, abs(incomingCmd.leftMotor));
  }

  if (incomingCmd.rightMotor > 0) {
    analogWrite(R_RPWM, incomingCmd.rightMotor);
    analogWrite(R_LPWM, 0);
  } else {
    analogWrite(R_RPWM, 0);
    analogWrite(R_LPWM, abs(incomingCmd.rightMotor));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(L_RPWM, OUTPUT); pinMode(L_LPWM, OUTPUT);
  pinMode(R_RPWM, OUTPUT); pinMode(R_LPWM, OUTPUT);
  pinMode(LED_GREEN, OUTPUT); pinMode(LED_YELLOW, OUTPUT);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;
  
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);
}

void loop() {
  // T' Watchdog
  if (millis() - lastRecvTime > 500) {
    // She's lost connection
    digitalWrite(LED_GREEN, LOW); 
    
    // Blink t' Yellow Eye
    if (millis() - lastBlinkTime > 300) {
      yellowState = !yellowState;
      digitalWrite(LED_YELLOW, yellowState);
      lastBlinkTime = millis();
    }

    // Stop t' tracks
    analogWrite(L_RPWM, 0); analogWrite(L_LPWM, 0);
    analogWrite(R_RPWM, 0); analogWrite(R_LPWM, 0);
  } else {
    // She hears ye loud and clear
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
  }
  delay(10);
}
