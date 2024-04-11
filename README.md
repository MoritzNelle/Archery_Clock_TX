# Archery Clock

## Introduction
The Archery Clock program is designed to control an archery clock system. It uses an Arduino microcontroller and a NeoPixel LED strip to display timing and group information for archery competitions. The clock provides visual cues for different phases of the competition, including getting to the line, shooting, and group transitions.

## Requirements
- Arduino microcontroller, ESP, ...
    - The code in `main.cpp` is written for an Arduino Pro Mini, but it can be adapted easily.
- Adafruit NeoPixel LED strip
- Buttons for controlling the clock
- Arduino IDE or PlatformIO

## Installation/Deployment
1. Connect the NeoPixel strip to the Arduino as per the hardware setup instructions.
2. Open the `main.cpp` file in the Arduino IDE or PlatformIO.
3. Compile and upload the program to the Arduino.

## Usage
The program runs automatically once uploaded to the Arduino. It controls the LED strip to display the timing and group information. The colors and timings can be adjusted in the `main.cpp` file.

## Functions
### `setup()`
**Input:** None  
**Output:** None  
**Calculation:** Initializes the NeoPixel strip, clears any previous entries, and sets up the serial communication. It also sets up the buttons and determines the start phase of the competition.  
**Usage:** This function is called once when the program starts. It is responsible for setting up the initial state of the program.

### `loop()`
**Input:** None  
**Output:** None  
**Calculation:** Main program loop. It controls the flow of the competition, including the transitions between different phases.  
**Usage:** This function is called repeatedly after `setup()` finishes. It is the main part of the program.

### `checkColors()`
**Input:** None  
**Output:** None  
**Calculation:** Checks the color settings and adjusts them if necessary.  
**Usage:** This function is called during the setup phase if `startPhase` is set to 1.

### `hold()`
**Input:** None  
**Output:** None  
**Calculation:** Pauses the program until the hold button is released.  
**Usage:** This function is used to pause the program during the setup phase and between different phases of the competition.

### `pingPong()`
**Input:** None  
**Output:** None  
**Calculation:** Controls the ping pong light effect on the LED strip.  
**Usage:** This function is used to display a visual effect during the setup phase if `startPhase` is set to 3.

### `setPixelColor()`
**Input:** Pixel index, color  
**Output:** None  
**Calculation:** Sets the color of a specific pixel on the LED strip.  
**Usage:** This function is used throughout the program to control the color of the LED strip.

For more detailed information about each function, please refer to the inline comments in the `main.cpp` file.