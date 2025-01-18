#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>

// Competition variables
int prac_rounds     = 2;  // Number of Practice rounds (0 - 5 #)
int comp_rounds     = 10; // Number of Competition Rounds (0 - 20 #)
int time_round      = 120; // Time per Round (10 - 210 s)
int time_line       = 10; // Get to the line (0 - 30 s)
int num_groups      = 4;  // Number of Archer Groups (1-4)
int user_brightness = 10; // Clocks LED brightness (1-10) # TODO: Implement brightness control in the code


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
const unsigned long debounceDelay     = 500; // Debounce delay in milliseconds

// Battery measurement variables
#define ALPHA 0.0001 // Smoothing factor for EMA (0 < ALPHA <= 1)
float emaBatteryPercentage = 0.0; // Initialize EMA value //TODO: initialize with the actual value, even if it is unstable, to avoid the initial delay

// Setup complete flag
bool setupComplete = false;
uint8_t rainbowOffset = 0;  // Tracks rainbow animation position

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


void displaySkipMessage() {
  u8g2.drawBox(0, 27, 128, 36); // Clear the display area
  u8g2.setDrawColor(0); // Set draw color to background color
  u8g2.setCursor(8, 42);
  u8g2.print("FWD button pressed");
  u8g2.setCursor(4, 57);
  u8g2.print("Skipping to end of phase");
  u8g2.sendBuffer();
  
  checkButtons(); // Wait for button release, to avoid multiple skips
  while (fwdPressed){checkButtons();}
  
  delay(1000); // Display the message for 1 second
  u8g2.setDrawColor(1); // Set draw color back to foreground color
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


// HSV to RGB conversion helper function
void HSVtoRGB(uint8_t hue, uint8_t sat, uint8_t val, uint8_t* rgb) {
    uint8_t r, g, b;
    
    uint8_t region = hue / 43;
    uint8_t remainder = (hue - (region * 43)) * 6; 

    uint8_t p = (val * (255 - sat)) >> 8;
    uint8_t q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
    uint8_t t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;

    switch(region) {
        case 0:  r = val; g = t;   b = p;   break;
        case 1:  r = q;   g = val; b = p;   break;
        case 2:  r = p;   g = val; b = t;   break;
        case 3:  r = p;   g = q;   b = val; break;
        case 4:  r = t;   g = p;   b = val; break;
        default: r = val; g = p;   b = q;   break;
    }
    
    rgb[0] = r;
    rgb[1] = g;
    rgb[2] = b;
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


void displayRainbowAnimation(uint8_t speed) {
    uint8_t ledColors[NUM_LEDS][3] = {0};
    
    // Generate rainbow pattern
    for(int i = 0; i < NUM_LEDS; i++) {
        uint8_t hue = ((i * 255 / NUM_LEDS) + rainbowOffset) % 255;
        uint8_t rgb[3];
        HSVtoRGB(hue, 255, 255, rgb);
        
        ledColors[i][0] = rgb[0];
        ledColors[i][1] = rgb[1];
        ledColors[i][2] = rgb[2];
    }
    
    // Update rainbow offset for next frame
    rainbowOffset = (rainbowOffset + speed) % 255;
    
    // Send to LED strips
    sendData(0, 0, 0, 0, ledColors);
    
    //delay(20); // Small delay to control animation speed
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
  fwdPressed = false; // Reset button state
}

// Function to display the current variable and its value
void displaySetupVariable(const char* name, int value) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.setCursor(0, 20);
    u8g2.printf("%s: %d", name, value);
    u8g2.sendBuffer();
}

// Function to handle button presses during setup
void handleSetupButtons(int& value, int minValue, int maxValue) {
    checkButtons();
    if (holdPressed) {
        value = min(value + 1, maxValue);
        holdPressed = false;
    }
    if (stopPressed) {
        value = max(value - 1, minValue);
        stopPressed = false;
    }
}

// Setup phase function
void setupPhase() {
    const char* variableNames[] = {
        "Practice Rounds",
        "Competition Rounds",
        "Time per Round (s)",
        "Get to the Line (s)",
        "Number of Groups",
        "LED Brightness"
    };
    int* variables[] = {
        &prac_rounds,
        &comp_rounds,
        &time_round,
        &time_line,
        &num_groups,
        &user_brightness
    };
    int minValues[] = {0, 0, 10, 0, 1, 1};
    int maxValues[] = {5, 20, 300, 30, 4, 10};

    for (int i = 0; i < 6; i++) {
        while (!fwdPressed) {
            displaySetupVariable(variableNames[i], *variables[i]);
            handleSetupButtons(*variables[i], minValues[i], maxValues[i]);
        }
        fwdPressed = false; // Reset button state
    }
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

  // Run setup phase
  setupPhase();

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

  setupComplete = true;  // Indicate that setup is complete
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

  // Wait for the initial FWD button press to start the timers
  static bool initialFwdPressed = false;
  if (!initialFwdPressed) {
    if (setupComplete && fwdPressed) {
      initialFwdPressed = true;
      fwdPressed = false;
      roundStartTime = currentTime; // Initialize the round start time
    } else {
      return; // Exit the loop function until the FWD button is pressed
    }
  }

  // Handle FWD button press to skip current phase
  if (fwdPressed) {
    if (isShooting) {  // Only allow skip during shooting phase
      unsigned long phaseTime = time_round * 1000;
      roundStartTime = currentTime - phaseTime; // Force phase completion
      displaySkipMessage();
    }
    fwdPressed = false; // Reset button state
  }

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
          roundStartTime = millis(); // Reset timer after arrow collection phase
          isShooting = false; // Ensure we start with "get to the line" phase
          return; // Exit loop to prevent timer from running during collection
        }
      }
      isShooting = !isShooting;
      roundStartTime = currentTime; // Reset the round start time
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
    // Competition is over - show endless rainbow
    while(true) {
      displayRainbowAnimation(1);
    }
  }
}

//XBUG: both phases (to the line and shooting) are displayed as "Shooting"
//XTODO: Implement skiping of the shooting phasses with fwd button
//XTODO: Batery charge indicator
//XBUG: The timers start running before the first FWD press
//BUG: The timer is continuing during the "collect arrows" phase
//TODO: Change order of the groups
//TODO: Check batery and buttons during waiting -> wating function?
//TODO: Implement the buzzer sound also for: after get to the line, after the shooting phase
//TODO: Implement rainbow for after there competition
//TODO: Change colors: get to the line - blue, shooting - green, collect arrows - red
//TODO: changee shooting-phase color from blue to orange when there are 10 seconds left
//TODO: Change the way leds are turned off, from num of leds per one sec to period between the leds, avoid a diffrent about of leds in the same time beeing turned off
//xTODO: Implement the set up phase
//TODO: Make the set up phase more user friendly
  //TODO: Enable the user to skip the entire set up phase
  //XTODO: After the last variable is set, display "Press FWD to start the competition"
  //TODO: Store the set up values in the EEPROM
//XBUG: the battery indicator is filled from the wrong side

/*
TestArcheryControlSystem

├── Basic Phase Control
│   ├── Test normal phase progression
│   └── Test phase timing accuracy
├── FWD Button Functionality  
│   ├── Test FWD press during shooting phase
│   └── Test FWD press during line-up phase (should not skip)
├── Group Progression
│   ├── Test group advancement
│   └── Test round completion
└── Practice/Competition Transition
  └── Test transition from practice to competition rounds// Modified loop() function
*/