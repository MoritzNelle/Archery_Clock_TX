#include <esp_now.h>
#include <WiFi.h>

// Define the pin and number of LEDs
#define NUM_LEDS   75

// Define the MAC addresses of the receivers
const uint8_t peerAddresses[][6] = {
  {0x30, 0xc6, 0xf7, 0x30, 0x21, 0x5c},
  {0x30, 0xc6, 0xf7, 0x30, 0x27, 0x94},
  {0x30, 0xc6, 0xf7, 0x30, 0x2b, 0xc4},
  {0xc8, 0xc9, 0xa3, 0xc9, 0x61, 0xcc}
};

// Define the data structure
typedef struct {
  uint8_t numBuzzerBeeps;
  uint8_t buzzerDuration;
  uint8_t buzzerBreak;
  uint8_t buzzerPitch; // New variable to adjust the tune/pitch of the buzzer
  uint8_t ledColors[NUM_LEDS][3]; // RGB values for each LED
} DataPackage;

DataPackage dataToSend;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //TODO: print extensive debugging information to Serial Monitor 

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Add peers
  for (int i = 0; i < sizeof(peerAddresses) / sizeof(peerAddresses[0]); i++) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, peerAddresses[i], 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.print("Failed to add peer: ");
      Serial.println(i);
      return;
    }
  }
}

void sendData(uint8_t numBuzzerBeeps, uint8_t buzzerDuration, uint8_t buzzerBreak, uint8_t buzzerPitch, uint8_t ledColors[NUM_LEDS][3]) {
  // Prepare data to send
  dataToSend.numBuzzerBeeps = numBuzzerBeeps;
  dataToSend.buzzerDuration = buzzerDuration;
  dataToSend.buzzerBreak = buzzerBreak;
  dataToSend.buzzerPitch = buzzerPitch;
  memcpy(dataToSend.ledColors, ledColors, sizeof(dataToSend.ledColors));

  // Send data to all peers
  for (int i = 0; i < sizeof(peerAddresses) / sizeof(peerAddresses[0]); i++) {
    esp_err_t result = esp_now_send(peerAddresses[i], (uint8_t *) &dataToSend, sizeof(dataToSend));

    if (result == ESP_OK) {
      Serial.print("Sent with success to peer ");
      Serial.println(i);
    } else {
      Serial.print("Error sending the data to peer ");
      Serial.println(i);
    }
  }
}

void loop() {

  // Change color
  uint8_t ledColors[NUM_LEDS][3];

  for (int i = 0; i < NUM_LEDS; i++) {
    ledColors[i][0] = 0;
    ledColors[i][1] = 0;
    ledColors[i][2] = 255;
  }

  sendData(1, 100, 220, 5, ledColors);

  delay(5000);





  for (int i = 0; i < NUM_LEDS; i++) {
    ledColors[i][0] = 0;
    ledColors[i][1] = 255;
    ledColors[i][2] = 0;
  }

  sendData(0, 0, 0, 0, ledColors);

  delay(5000);
  
}