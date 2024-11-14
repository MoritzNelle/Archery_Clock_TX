#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>

// Competition variables
int prac_rounds = 2;  // Number of Practice rounds (0 - 5 #)
int comp_rounds = 5; // Number of Competition Rounds (0 - 20 #)
int time_round  = 10; // Time per Round (10 - 210 s)
int time_line   = 5; // Get to the line (0 - 30 s)
int num_groups  = 4;  // Number of Archer Groups (1-4)
int brightness  = 10; // Clocks LED brightness (1-10) # TODO: Implement brightness control in the code

// Pin Definitions
#define FFW_Button      14
#define HLD_UP_Button   13
#define STP_DWN_Button  12
#define BUZZER          25
#define SDA             18
#define SCL             19

// Definitions
#define NUM_LEDS   75

// Create an instance of the U8G2 display with SH1106 driver
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);

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
  Serial.begin(115200);  // Initialize Serial Monitor

  Wire.begin(SDA, SCL);  // Initialize the I2C communication with specified SDA and SCL pins
  u8g2.begin();  // Initialize the OLED display

  WiFi.mode(WIFI_STA);  // Set device as a Wi-Fi Station

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

  // Set button pins as input
  pinMode(FFW_Button, INPUT);
  pinMode(HLD_UP_Button, INPUT);
  pinMode(STP_DWN_Button, INPUT);
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
  }
}

void updateLedStrip(uint8_t group, float progress, bool isShooting) {
  uint8_t ledColors[NUM_LEDS][3] = {0};

  // Define colors for the groups
  uint8_t groupColors[4][3] = {
    {255, 0, 0},   // Red
    {0, 255, 0},   // Green
    {0, 0, 255},   // Blue
    {255, 255, 0}  // Yellow
  };

  // Set the top LED to indicate the upcoming group
  uint8_t nextGroup = (group + 1) % num_groups;
  ledColors[NUM_LEDS - 1][0] = groupColors[nextGroup][0];
  ledColors[NUM_LEDS - 1][1] = groupColors[nextGroup][1];
  ledColors[NUM_LEDS - 1][2] = groupColors[nextGroup][2];

  // Set the three LEDs below the top LED to indicate the current group
  for (int i = 0; i < 3; i++) {
    ledColors[NUM_LEDS - 2 - i][0] = groupColors[group][0];
    ledColors[NUM_LEDS - 2 - i][1] = groupColors[group][1];
    ledColors[NUM_LEDS - 2 - i][2] = groupColors[group][2];
  }

  // Set the LEDs to indicate the progress
  int activeLeds = (1.0 - progress) * (NUM_LEDS - num_groups - 3);
  for (int i = 0; i < activeLeds; i++) {
    if (isShooting) {
      ledColors[i][2] = 255; // Blue for shooting time
    } else {
      ledColors[i][0] = 255; // Red for getting to the line
    }
  }

  sendData(0, 100, 50, 5, ledColors); // Adjust the buzzer parameters as needed
}

void loop() {
  static int currentRound = 0;
  static int currentGroup = 0;
  static unsigned long roundStartTime = 0;
  static bool isShooting = false;
  static bool isPractice = true;
  static unsigned long lastUpdateTime = 0;
  const unsigned long updateInterval = 1000; // Update interval in milliseconds

  unsigned long currentTime = millis();

  if (currentRound < prac_rounds + comp_rounds) {
    if (currentTime - roundStartTime >= (isShooting ? time_round : time_line) * 1000) {
      if (isShooting) { 
        currentGroup++;
        if (currentGroup >= num_groups) {
          currentGroup = 0;
          currentRound++;
          isPractice = currentRound < prac_rounds;
        }
      }
      isShooting = !isShooting;
      roundStartTime = currentTime;
    }

    if (currentTime - lastUpdateTime >= updateInterval) {
      lastUpdateTime = currentTime;
      float progress = (float)(currentTime - roundStartTime) / ((isShooting ? time_round : time_line) * 1000);
      updateLedStrip(currentGroup, progress, isShooting);

      // Update the display
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);

      // Display current round
      u8g2.setCursor(0, 10);
      if (isPractice) {
        u8g2.printf("P%d/%d", currentRound + 1, prac_rounds);
      } else {
        u8g2.printf("R%d/%d", currentRound - prac_rounds + 1, comp_rounds);
      }

      // Display current and next group
      u8g2.setCursor(0, 20);
      u8g2.printf("Current Group: %d", currentGroup);
      u8g2.setCursor(0, 30);
      u8g2.printf("Next Group: %d", (currentGroup + 1) % num_groups);

      // Display current phase
      u8g2.setCursor(0, 40);
      if (isShooting) {
        u8g2.print("Phase: Shoot");
      } else {
        u8g2.print("Phase: To the line");
      }

      // Display remaining time and progress bar
      int remainingTime = ((isShooting ? time_round : time_line) * 1000) - (currentTime - roundStartTime);
      u8g2.setCursor(0, 50);
      u8g2.printf("Time: %d s", remainingTime / 1000);

      int progressBarWidth = (int)(progress * 128);
      u8g2.drawFrame(0, 54, 128, 10);
      u8g2.drawBox(0, 54, progressBarWidth, 10);

      u8g2.sendBuffer();
    }
  } else {
    // Indicate that archers can collect their arrows
    uint8_t ledColors[NUM_LEDS][3] = {0};
    for (int i = 0; i < NUM_LEDS; i++) {
      ledColors[i][1] = 255; // Green to indicate collection time
    }
    sendData(3, 100, 50, 5, ledColors);

    // Update the display to show collection phase
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0, 0);
    u8g2.print("Phase: Collect arrows");
    u8g2.sendBuffer();
  }
}