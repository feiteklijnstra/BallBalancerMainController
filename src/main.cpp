#include "Arduino.h"
#include <esp_now.h>
#include <WiFi.h>

// MAC of motor controller: 08:3a:8d:bb:72:48
uint8_t broadcastAddress[] = {0x08, 0x3a, 0x8b, 0xbb, 0x72, 0x48};

// Structure to receive data from controller
typedef struct messageFromController_t
{
  float targetAngle1;
  float targetAngle2;
  bool enable;
} messageFromController_t;

// Create a messageFromController_t called dataFromController
messageFromController_t dataToController;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Touch screen library with X Y and Z (pressure) readings as well
// as oversampling to avoid 'bouncing'
// This demo code returns raw readings, public domain

#include <stdint.h>
#include "TouchScreen.h"

// These are the pins for the shield! 4 xp 0 ym 2 xm 15 yp
// YP Blue    27
// YM Orange  14
// XP Yellow  26
// XM Green   12
#define YP 32 // must be an analog pin, use "An" notation!
#define XM 33 // must be an analog pin, use "An" notation!
#define YM 25 // can be a digital pin
#define XP 26 // can be a digital pin

#define MINPRESSURE 2
#define MAXPRESSURE 2000

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 670);

bool targetAnglePositive = true;

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Starting Serial");
  analogReadResolution(10);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_MODE_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop(void)
{
  // a point object holds x y and z coordinates
  TSPoint p = ts.getPoint();

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE)
  {
    Serial.print("X = ");
    Serial.print(p.x);
    Serial.print("\tY = ");
    Serial.print(p.y);
    Serial.print("\tPressure = ");
    Serial.println(p.z);
  }

  dataToController.enable = true;
  if (targetAnglePositive)
  {
    dataToController.targetAngle1 = 1.5;
    dataToController.targetAngle2 = 1.5;
  }
  else
  {
    dataToController.targetAngle1 = -1.5;
    dataToController.targetAngle2 = -1.5;
  }

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&dataToController, sizeof(dataToController));

  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
  targetAnglePositive = !targetAnglePositive;
  delay(2000);
}