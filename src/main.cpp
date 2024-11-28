#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>

// Competition variables
int prac_rounds     = 2;  // Number of Practice rounds (0 - 5 #)
int comp_rounds     = 5; // Number of Competition Rounds (0 - 20 #)
int time_round      = 10; // Time per Round (10 - 210 s)
int time_line       = 5; // Get to the line (0 - 30 s)
int num_groups      = 4;  // Number of Archer Groups (1-4)
int user_brightness = 3; // Clocks LED brightness (1-10) # TODO: Implement brightness control in the code


// Pin Definitions
#define FWD_Button      14
#define HLD_UP_Button   13
#define STP_DWN_Button  12
#define BUZZER          25
#define SDA             18
#define SCL             19
#define BATTERY_PIN     35

// Definitions
#define NUM_LEDS        75

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

// Button states
volatile bool fwdPressed = false;
volatile bool holdPressed = false;
volatile bool stopPressed = false;

// Debounce variables
volatile unsigned long lastFWDPress   = 0;
volatile unsigned long lastHoldPress  = 0;
volatile unsigned long lastStopPress  = 0;
const unsigned long debounceDelay     = 50; // Debounce delay in milliseconds

// Battery measurement variables
#define ALPHA 0.0001 // Smoothing factor for EMA (0 < ALPHA <= 1)
float emaBatteryPercentage = 0.0; // Initialize EMA value //TODO: initialize with the actual value, even if it is unstable, to avoid the initial delay

void checkButtons() {
    unsigned long currentTime = millis();

    // Check FWD button
    bool fwdReading = digitalRead(FWD_Button);
    if (fwdReading != fwdPressed && (currentTime - lastFWDPress) > debounceDelay) {
        lastFWDPress = currentTime;
        if (fwdReading == HIGH) {
            Serial.println("FWD button pressed");
            fwdPressed = true;
        } else {
            fwdPressed = false;
        }
    }

    // Check HOLD button
    bool holdReading = digitalRead(HLD_UP_Button);
    if (holdReading != holdPressed && (currentTime - lastHoldPress) > debounceDelay) {
        lastHoldPress = currentTime;
        if (holdReading == HIGH) {
            Serial.println("HLD_UP button pressed");
            holdPressed = true;
        } else {
            holdPressed = false;
        }
    }

    // Check STOP button
    bool stopReading = digitalRead(STP_DWN_Button);
    if (stopReading != stopPressed && (currentTime - lastStopPress) > debounceDelay) {
        lastStopPress = currentTime;
        if (stopReading == HIGH) {
            Serial.println("STP_DWN button pressed");
            stopPressed = true;
        } else {
            stopPressed = false;
        }
    }
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\rLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void IRAM_ATTR handleFWDPress() {
  unsigned long currentTime = millis();
  if (currentTime - lastFWDPress > debounceDelay) {
    fwdPressed = true;
    lastFWDPress = currentTime;
  }
}

void IRAM_ATTR handleHoldPress() {
  unsigned long currentTime = millis();
  if (currentTime - lastHoldPress > debounceDelay) {
    holdPressed = true;
    lastHoldPress = currentTime;
  }
}

void IRAM_ATTR handleStopPress() {
  unsigned long currentTime = millis();
  if (currentTime - lastStopPress > debounceDelay) {
    stopPressed = true;
    lastStopPress = currentTime;
  }
}


float readBatteryLevel() {
    // Read the analog value
    int analogValue = analogRead(BATTERY_PIN);
    // Convert the analog value to voltage (3.3V reference and 12-bit ADC)
    float voltage = analogValue * (3.3 / 4095.0);
    // Since the voltage is halved by the voltage divider
    float batteryVoltage = voltage * 2;

    // Convert battery voltage to percentage
    float minVoltage = 3.0;
    float maxVoltage = 4.2;
    float batteryPercentage = ((batteryVoltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0;

    // Ensure the percentage is within 0-100%
    if (batteryPercentage > 100.0) {
        batteryPercentage = 100.0;
    } else if (batteryPercentage < 0.0) {
        batteryPercentage = 0.0;
    }

    return batteryPercentage;
}

float calculateEmaBatteryPercentage() {
    float batteryPercentage;
    batteryPercentage = readBatteryLevel();
    emaBatteryPercentage = (ALPHA * batteryPercentage) + ((1 - ALPHA) * emaBatteryPercentage);

    return emaBatteryPercentage;
}

void displayBatteryLevel() {
  // Draw battery level indicator
  int batteryLevel = (int)calculateEmaBatteryPercentage();
  int batteryWidth = map(batteryLevel, 0, 100, 0, 16); // Map battery level to width (0-16 pixels)
  
  // Draw battery outline
  u8g2.drawFrame(112, 0, 16, 8); // x, y, width, height
  u8g2.drawBox(110, 2, 2, 4); // Battery terminal

  // Draw battery level
  if (batteryLevel < 100) {
    u8g2.drawBox(112, 0, batteryWidth, 8); // x, y, width, height
  } else {
    u8g2.drawBox(112, 0, 16, 8); // Full battery
  }

  if (batteryLevel == 100) {
    u8g2.setCursor(85, 8); // Adjusted cursor position to avoid writing over the battery
  } else {
    u8g2.setCursor(90, 8);
  }
  u8g2.printf("%d%%", batteryLevel);

}


void setup() { //MARK: set-up
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

  // Set pinModes
  pinMode(FWD_Button,     INPUT_PULLDOWN);
  pinMode(HLD_UP_Button,  INPUT_PULLDOWN);
  pinMode(STP_DWN_Button, INPUT_PULLDOWN);
  pinMode(BATTERY_PIN,    INPUT);
  pinMode(BUZZER,         OUTPUT);

  emaBatteryPercentage = readBatteryLevel(); // Initialize EMA value with the actual value, to avoid the initial delay

  // Wait for fwd button press
  u8g2.setFont(u8g2_font_helvB08_tr);
  u8g2.drawBox(0, 0, 128, 64); // Draw a filled rectangle to invert the screen
  u8g2.setDrawColor(0); // Set draw color to background color
  u8g2.setCursor(35, 25);
  u8g2.print("Press FWD");
  u8g2.setCursor(10, 40);
  u8g2.print("to Start Competition");
  u8g2.sendBuffer();
  u8g2.setDrawColor(1); // Set draw color back to foreground color

  while (!fwdPressed) {
    checkButtons();
  }
  delay(200); // Wait to avoid multiple button presses
  
// End of setup
}

int mapBrightness(int user_brightness) {
  switch (user_brightness) {
    case 1:   return 1;    // ~0.4%
    case 2:   return 5;    // ~2%
    case 3:   return 10;   // ~4%
    case 4:   return 20;   // ~8%
    case 5:   return 40;   // ~16%
    case 6:   return 70;   // ~27%
    case 7:   return 110;  // ~43%
    case 8:   return 150;  // ~59%
    case 9:   return 200;  // ~78%
    case 10:  return 255;  // 100%
    default:  return 255;  // Default full brightness
  }
}

void sendData(uint8_t numBuzzerBeeps, uint8_t buzzerDuration, uint8_t buzzerBreak, uint8_t buzzerPitch, uint8_t ledColors[NUM_LEDS][3]) {
    // Map brightness value
    float brightnessScale = (float)mapBrightness(user_brightness) / 255.0f;
    
    // Prepare data to send
    dataToSend.numBuzzerBeeps = numBuzzerBeeps;
    dataToSend.buzzerDuration = buzzerDuration;
    dataToSend.buzzerBreak = buzzerBreak;
    dataToSend.buzzerPitch = buzzerPitch;

    // Apply brightness scaling when copying LED colors
    for(int i = 0; i < NUM_LEDS; i++) {
        dataToSend.ledColors[i][0] = (uint8_t)(ledColors[i][0] * brightnessScale);
        dataToSend.ledColors[i][1] = (uint8_t)(ledColors[i][1] * brightnessScale);
        dataToSend.ledColors[i][2] = (uint8_t)(ledColors[i][2] * brightnessScale);
    }

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

// Define the colors for each group
const char* groupColors[] = {"Red", "Green", "Blue", "Yellow"};

void enterCollectArrowsPhase() {
  // Indicate that archers can collect their arrows
  uint8_t ledColors[NUM_LEDS][3] = {0};
  for (int i = 0; i < NUM_LEDS; i++) {
    ledColors[i][1] = 255; // Green to indicate collection time
  }
  sendData(3, 100, 50, 5, ledColors);

  u8g2.drawBox(0, 27, 128, 36); 

  u8g2.setDrawColor(0); // Set draw color to background color
  u8g2.setCursor(8, 42);
  u8g2.print("Phase: Collect arrows");
  u8g2.setCursor(4, 57);
  u8g2.print("Press FWD to continue");
  u8g2.sendBuffer();
  u8g2.setDrawColor(1); // Set draw color back to foreground color

  // Stay in the collect arrows phase until the FWD button is pressed
  while (!fwdPressed) {
    checkButtons();
  }
  fwdPressed = false;

  // calculate system brightness
  int system_brightness = mapBrightness(user_brightness);

}


void loop() { //MARK:loop
  static int currentRound             = 0;
  static int currentGroup             = 0;
  static unsigned long roundStartTime = 0;
  static bool isShooting              = false;
  static bool isPractice              = true;
  static unsigned long lastUpdateTime = 0;
  const unsigned long updateInterval  = 1000; // Update interval in milliseconds
  unsigned long currentTime           = millis();

  checkButtons();

  if (currentRound < prac_rounds + comp_rounds) {
    // Check if the current phase time has elapsed
    if (currentTime - roundStartTime >= (isShooting ? time_round : time_line) * 1000) {
      if (isShooting) {
        currentGroup++;
        if (currentGroup >= num_groups) {
          currentGroup = 0;
          currentRound++;
          isPractice = currentRound < prac_rounds;

          // Enter "collect your arrows" phase
          enterCollectArrowsPhase();
        }
      }
      isShooting = !isShooting;
      roundStartTime = currentTime;
    }

    // Update the display and LED strip at regular intervals
    if (currentTime - lastUpdateTime >= updateInterval) {
      lastUpdateTime = currentTime;
      float progress = (float)(currentTime - roundStartTime) / ((isShooting ? time_round : time_line) * 1000);
      updateLedStrip(currentGroup, progress, isShooting);

      // Update the display
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_helvB08_tr); // sans-serif font

      // Display current round
      u8g2.setCursor(0, 8);
      if (isPractice) {
        u8g2.printf("P%d/%d", currentRound + 1, prac_rounds);
      } else {
        u8g2.printf("R%d/%d", currentRound - prac_rounds + 1, comp_rounds);
      }
      u8g2.printf(" (Group: %d/%d)", currentGroup + 1, num_groups); // Display current and next group with colors

      // Display current shooting phase
      u8g2.setCursor(0, 22);
      u8g2.printf("Group: %s -> %s", groupColors[currentGroup], groupColors[(currentGroup + 1) % num_groups]); // Display current and next group with colors

      // Display current phase
      u8g2.setCursor(0, 36);
      if (isShooting) {
        u8g2.print("Phase: Shoot");
      } else {
        u8g2.print("Phase: To the line");
      }

      // Display remaining time and progress bar
      int remainingTime = ((isShooting ? time_round : time_line) * 1000) - (currentTime - roundStartTime);
      u8g2.setCursor(0, 50);
      u8g2.printf("Time: %d/%d s", remainingTime / 1000, (isShooting ? time_round : time_line));

      int progressBarWidth = (int)(progress * 128);
      u8g2.drawFrame(0, 52, 128, 12);
      u8g2.drawBox(0, 52, progressBarWidth, 12);

      displayBatteryLevel();      // Display/update battery level indicator

      u8g2.sendBuffer();
    }
  } else {
    // Enter "collect your arrows" phase after all rounds are completed
    enterCollectArrowsPhase();
  }
}

//BXUG: both phases (to the line and shooting) are displayed as "Shooting"
//TODO: Implement skiping of the phasses with fwd button
//TODO: Batery charge indicator
//TODO: Change order of the groups
//TODO: Check batery and buttons during waiting -> wating function?