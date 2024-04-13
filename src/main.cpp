/* Archery Clock
 * 
 * This program controls an archery clock system using an Arduino and a NeoPixel LED strip. It displays timing and group information for archery competitions, providing visual cues for different phases of the competition.
 * 
 * Requirements:
 * - Arduino microcontroller
 * - Adafruit NeoPixel LED strip
 * - 2 buttons (Fast Forward, Hold)
 * - Arduino IDE or PlatformIO
 * 
 * Installation:
 * 1. Connect the NeoPixel strip and Buttons to the Arduino.
 * 2. Open `main.cpp` in the Arduino IDE or PlatformIO.
 * 3. Compile and upload the program to the Arduino.
 * 
 * Usage:
 * The program runs automatically once uploaded. It controls the LED strip to display timing and group information. Adjust colors and timings in `main.cpp`. Don't touch the other variables unless you know what you're doing.*/


// COMPETITION VARIABLES (meant to be changed by the user)
#define numRounds       10    // 
//#define numArrows      3
#define secGetToLine    10    // time to get to the shootingline before the shooting starts
#define secShooting     20    // time for shooting
#define numGroups        4     // min. 1, max 4 groups
float maxBrightness   =  255;   // 2(!)-255 (2 = lowest, 255 = full brightness)
#define startPhase       3     // 0: no start phase (R/B/G); 1: checkColors + hold; 2: hold; 3: pingPong + hold


//CLOCK VARIABLES (only meant to be changed by the user if a new/diffrent clock is used AND the user knows what they are doing)
unsigned int colorOfGroups[4] = {0, 21845, 10922, 43681};  // group 1/A: red, group 2/B: green, group 3/C: yellow, group 4/D: blue
int colorOfTimer              = 21845;  // green //enter color in HSV format (0-65535);
int colorOfGetToLine          = 43681;  // blue  //enter color in HSV format (0-65535);
int warningColor              = 5000;
int warningSec                = 10;     // time in seconds before the end of the shooting phase when the warning color is displayed       
#define NUM_PIXELS              18
float numLedGroupIndication =   2;
float numLedNextGroup      =    1;
#define numLedGap               1
int numLedTimer = NUM_PIXELS - numLedGroupIndication - numLedNextGroup - numLedGap -1;
int listOfGroups[numGroups];


//SYSTEM VARIABLES (NO TOUCHY TOUCHY!...NEVER! otherwise you will break the code and the world will end...or something like that)
bool HoldState = false;
bool FFWState =  false;
int currentGroup = 1;
int nextGroup = 1;
int actRound = 1;
//unsigned long timerOne = 0;
int subRound = 0;
float fadeDelayCorrection = 0.925; // delay correction for the fade function, in milliseconds; the higher the value, the slower the fade


#include <Arduino.h>              // only needed if you are using PlatformIO, the Arduino IDE already includes this library by defualt in the background
#include <Adafruit_NeoPixel.h>    // include the Adafruit NeoPixel library, alternative to the FastLED library

#define LEDPIN 6                  // 
#define FFWbutton 2               // all GPIO pins can be used
#define Holdbutton 3              // all GPIO pins can be used

Adafruit_NeoPixel clock1(NUM_PIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

//--------------------------------------------------------------------------------

void checkButtons(){
  int debounceDuration = 50;

  if (!digitalRead(FFWbutton)) {
    while (!digitalRead(FFWbutton)) {
      delay(debounceDuration);
    }
    delay(debounceDuration);

    if (FFWState == false){
      FFWState = true;
    } else {
      FFWState = false;
    }
    //Serial.print("FFWbutton pressed, FFWState: ");
    //Serial.println(FFWState);
  }

  if (!digitalRead(Holdbutton)) {
    while (!digitalRead(Holdbutton)) {
      delay(debounceDuration);
    }
    delay(debounceDuration);

    if (HoldState == false){
      HoldState = true;
    } else {
      HoldState = false;
    }
    //Serial.print("Holdbutton pressed, HoldState: ");
    //Serial.println(HoldState);
  }

}

void countDown(float firstPixel, float lastPixel, int color /*HSV*/ ,float duration, bool warning){

  float iterations = log(maxBrightness) / log(1.05);
  float numPixels = lastPixel - firstPixel;
  float fadeDelay = (duration*1000 / iterations)/numPixels; // delay for each brightness level; 1/numLedGroupIndication = 0 ... WHY?
  fadeDelay = fadeDelay * fadeDelayCorrection;

  int warningPixel = (numPixels/duration * warningSec);
  Serial.print("warningPixel: ");Serial.println(warningPixel);

  for (int i = firstPixel; i <= lastPixel; i++) {
    clock1.setPixelColor(i, clock1.ColorHSV(color, 255, maxBrightness));
  }
  clock1.show();

  for (int i = lastPixel; i >= firstPixel; i--) {

    if (warning == true && i == warningPixel) {
      for (int j = i; j >= firstPixel; j--) {
        clock1.setPixelColor(j, clock1.ColorHSV(warningColor, 255, maxBrightness));
      }
      clock1.show();
      color = warningColor;
    }

    for (float brightness = maxBrightness; brightness > 1; brightness = brightness / 1.05) {
      clock1.setPixelColor(i, clock1.ColorHSV(color, 255, brightness));
      clock1.show();

    unsigned long startTime = millis();
      while(millis() - startTime < fadeDelay) {
        checkButtons();
      }
    
      checkButtons();
      while (HoldState==true)
      {
        checkButtons();
      }

      if (FFWState==true)
      {
        FFWState = false;
        return;
      }
  }
    clock1.setPixelColor(i, clock1.Color(0, 0, 0)); // set LED color to off
    clock1.show();
  }  
}

void fade(int color){                                                       // can not be skipped nor hold, this is by design!!!
  clock1.clear();                                                           // clear all registers, in theory not necessary
  for (int i = 0; i < 3; i++) {                                             // repeat the fade 3 times
    for (int j = 0; j < NUM_PIXELS; j++) {                                  // fade in
      clock1.setPixelColor(j, clock1.ColorHSV(color, 255, maxBrightness));  // set LED color to maxBrightness
    }
    clock1.show();
    delay(500);                                                             // duration of maxBrightness

    for (int j = 0; j < NUM_PIXELS; j++) {                                  // fade out           
      clock1.setPixelColor(j, clock1.ColorHSV(color, 255, 0));              // set LED color to off
    }
    clock1.show();
    delay(500);                                                             // duration of off
  }
}

// void fade(int color){
//   int fadeDuration = 1000;      // fade duration in milliseconds
//   int fadeDelay = 10;           // delay between each fade step in milliseconds

//   for (int i = 0; i < 3; i++) { // repeat the fade 3 times
//     for (int brightness = 0; brightness <= maxBrightness; brightness++) { // fade in
//       for (int j = 0; j < NUM_PIXELS; j++) {
//         clock1.setPixelColor(j, clock1.ColorHSV(color, 255, brightness));
//       }
//       clock1.show();
//       delay(fadeDelay);
//     }

//     for (int brightness = maxBrightness; brightness >= 0; brightness--) { // fade out
//       for (int j = 0; j < NUM_PIXELS; j++) {
//         clock1.setPixelColor(j, clock1.ColorHSV(color, 255, brightness));
//       }
//       clock1.show();
//       delay(fadeDelay);
//     }
//   }
// }



void displayGroup(){
  for (int i = NUM_PIXELS-numLedGroupIndication; i < NUM_PIXELS; i++) {
    clock1.setPixelColor(i, clock1.ColorHSV(colorOfGroups[listOfGroups[0]-1], 255, maxBrightness));
  }
  clock1.show();

  for (int j = NUM_PIXELS-numLedGroupIndication-numLedNextGroup; j < NUM_PIXELS-numLedGroupIndication; j++) {
    clock1.setPixelColor(j, clock1.ColorHSV(colorOfGroups[listOfGroups[1]-1], 255, maxBrightness));
  }

//Serial.print("currentGroup: ");Serial.println(currentGroup);

  if (subRound+1 == numGroups) {
    for (int k = NUM_PIXELS-numLedGroupIndication-numLedNextGroup; k < NUM_PIXELS-numLedGroupIndication; k++) {
      clock1.setPixelColor(k, clock1.ColorHSV(colorOfGroups[listOfGroups[2]-1], 255, maxBrightness));
    }
  }

  clock1.show();

}


void hold(){

  displayGroup();
  HoldState = true;
  while (HoldState==true && FFWState==false)
  {
    checkButtons();
  }
  FFWState = false;
  HoldState = false;
}

void updateGroups(){
  int temp = listOfGroups[0];
  for (int i = 0; i < numGroups - 1; i++) {
    listOfGroups[i] = listOfGroups[i + 1];
  }
  listOfGroups[numGroups - 1] = temp;
}

void checkColors() {
  clock1.clear();

  for (int i = 0; i < NUM_PIXELS; i++) {clock1.setPixelColor(i, clock1.Color(maxBrightness,0,0));}
  clock1.show();
  delay(1000);

  for (int i = 0; i < NUM_PIXELS; i++) {clock1.setPixelColor(i, clock1.Color(0,maxBrightness,0));}
  clock1.show();
  delay(1000);

  for (int i = 0; i < NUM_PIXELS; i++) {clock1.setPixelColor(i, clock1.Color(0,0,maxBrightness));}
  clock1.show();
  delay(1000);

  clock1.clear();
  clock1.show();
}

void pingPong(){
  int startPos = 0;
  int endPos = NUM_PIXELS - 1;
  int currentPos = startPos;
  int direction = 1; // 1 for moving right, -1 for moving left
  int colorIndex = 0; // index to keep track of the current color
  int colorOfPixel = 0;

  while (HoldState == false && FFWState == false) {
    clock1.clear();
    clock1.setPixelColor(currentPos, clock1.ColorHSV(colorOfPixel, 255, maxBrightness));
    clock1.show();
    delay(100);

    currentPos += direction;
    if (currentPos > endPos) {
      currentPos = endPos - 1;
      direction = -1;
      colorIndex = (colorIndex + 1) % 3; // increment color index
    } else if (currentPos < startPos) {
      currentPos = startPos + 1;
      direction = 1;
      colorIndex = (colorIndex + 1) % 3; // increment color index
    }

    // Set color based on color index
    if (colorIndex == 0) {
      colorOfPixel = 0; // red
    } else if (colorIndex == 1) {
      colorOfPixel = 21845; // green
    } else if (colorIndex == 2) {
      colorOfPixel = 43681; // blue
    }

    checkButtons();
  }
  clock1.clear();
  clock1.show();

  HoldState = false;
  FFWState = false;
}

//--------------------------------------------------------------------------------

void setup() {

  for (int i = 0; i < numGroups; i++) { // fill the list of groups with the numbers 1 to numGroups [1,2,3,...,numGroups]
    listOfGroups[i] = i+1;
  } 

  delay(500);            // for recovery
  clock1.begin();        // initialize NeoPixel strip object
  clock1.clear();        // clear all registers from previous entries
  clock1.show();         // display the cleared registers

  Serial.begin(9600);    // initialize serial communication at 9600 bits per second

  pinMode(FFWbutton, INPUT_PULLUP);   // both buttons are connected to GROUND (not VCC) and pulled up with internal pullup resistors
  pinMode(Holdbutton, INPUT_PULLUP);

  if (numGroups < 1) {
    nextGroup = 2;
  }

  switch (startPhase) {
    case 0:
      // No start phase, just start the competition right away
      break;

    case 1:
      checkColors();
      for (int i = 0; i <= numLedTimer; i++) {clock1.setPixelColor(i, clock1.ColorHSV(colorOfGetToLine, 255, maxBrightness));}  // demonstrate the get to the line-timer
      clock1.show();
      hold();
      break;
    
    case 2:
      for (int i = 0; i <= numLedTimer; i++) {clock1.setPixelColor(i, clock1.ColorHSV(colorOfGetToLine, 255, maxBrightness));}  // demonstrate the get to the line-timer
      clock1.show();
      hold();
      break;
    
    case 3:
      pingPong();
      for (int i = 0; i <= numLedTimer; i++) {clock1.setPixelColor(i, clock1.ColorHSV(colorOfGetToLine, 255, maxBrightness));}  // demonstrate the get to the line-timer
      clock1.show();
      hold();
      break;

    default:
      // No start phase, just start the competition right away
      break;
  }
}

void loop() {
  //checkButtons();
  while (actRound <= numRounds) {
    Serial.print("actRound: ");Serial.println(actRound);  // print the actual round for debugging

    for (subRound = 0; subRound < numGroups; subRound++) {
      for(int j = 0; j < (sizeof(listOfGroups)/sizeof(listOfGroups[0])); j++) {Serial.print(listOfGroups[j]);}Serial.println(); // print the list of groups for debugging

      displayGroup();
      countDown(0,numLedTimer, colorOfGetToLine, secGetToLine, false); // get to the line
      countDown(0,numLedTimer, colorOfTimer,     secShooting,  true );      // shooting
      updateGroups();
    }
    updateGroups();
    fade(0);                                                // indicates the end of shooting 
    hold(); // holds every thing after the last group shoot until the hold- or continue-button is pressed, time to collect the arrows and write down the scores
    actRound++;
  }
  //competition is over
}