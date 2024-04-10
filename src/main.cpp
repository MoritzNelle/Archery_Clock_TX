// COMPETITION VARIABLES (meant to be changed by the user)
#define numRounds       10    // 
//#define numArrows       3
#define secGetToLine    10    // time to get to the shootingline before the shooting starts
#define secShooting     90    // time for shooting
#define numGroups       4     // min. 1, max 4 groups
float maxBrightness   = 2;   // 2(!)-255 (0 = off, 255 = full brightness)


//CLOCK VARIABLES (only meant to be changed by the user if a new/diffrent clock is used AND the user knows what they are doing)
unsigned int colorOfGroups[4] = {0, 21845, 10922, 43681};  // group 1/A: red, group 2/B: green, group 3/C: yellow, group 4/D: blue
int colorOfTimer              = 21845;  // green //enter color in HSV format (0-65535);
int colorOfGetToLine          = 43681;  // blue  //enter color in HSV format (0-65535);
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
    if (HoldState == false){
      HoldState = true;
    } else {
      HoldState = false;
    }
    //Serial.print("Holdbutton pressed, HoldState: ");
    //Serial.println(HoldState);
  }

}

void countDown(float firstPixel, float lastPixel, int color /*HSV*/ ,float duration){

  float iterations = log(maxBrightness) / log(1.05);
  float numPixels = lastPixel - firstPixel;
  float fadeDelay = (duration*1000 / iterations)/numPixels; // delay for each brightness level; 1/numLedGroupIndication = 0 ... WHY?
  fadeDelay = fadeDelay * fadeDelayCorrection;  
  for (int i = firstPixel; i <= lastPixel; i++) {
    clock1.setPixelColor(i, clock1.ColorHSV(color, 255, maxBrightness));
  }
  clock1.show();


  for (int i = lastPixel; i >= firstPixel; i--) {
    for (float brightness = maxBrightness; brightness > 1; brightness = brightness / 1.05) {
      clock1.setPixelColor(i, clock1.ColorHSV(color, 255, brightness));
      clock1.show();
      delay(fadeDelay);
    
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

void fade(int color){
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
    delay(500);                                                            // duration of off
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


//--------------------------------------------------------------------------------

void setup() {

  for (int i = 0; i < numGroups; i++) {
    listOfGroups[i] = i+1;
  } 

  //delay(3000); // 3 second delay for recovery
  clock1.begin();        // initialize NeoPixel strip object
  Serial.begin(9600);    // initialize serial communication at 9600 bits per second

  pinMode(FFWbutton, INPUT_PULLUP);   // both buttons are connected to GROUND (not VCC) and pulled up with internal pullup resistors
  pinMode(Holdbutton, INPUT_PULLUP);

  if (numGroups < 1) {
    nextGroup = 2;
  }

  //hold();         // holds every thing on start up until the hold- or continue-button is pressed for the first time
}

void loop() {
  //checkButtons();
  while (actRound <= numRounds) {
    Serial.print("actRound: ");Serial.println(actRound);  // print the actual round for debugging

    for (subRound = 0; subRound < numGroups; subRound++) {
      for(int j = 0; j < (sizeof(listOfGroups)/sizeof(listOfGroups[0])); j++) {Serial.print(listOfGroups[j]);}Serial.println(); // print the list of groups for debugging

      displayGroup();
      countDown(0,numLedTimer, colorOfGetToLine, secGetToLine); // get to the line
      countDown(0,numLedTimer, colorOfTimer, secShooting);      // shooting
      updateGroups();
    }
    updateGroups();
    fade(0);                                                // indicates the end of shooting 
    hold(); // holds every thing after the last group shoot until the hold- or continue-button is pressed, time to collect the arrows and write down the scores
    actRound++;
  }
  //competition is over
}