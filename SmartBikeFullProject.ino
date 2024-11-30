//Libraries
#include <Wire.h> // Wire.h is for the communication with the accelerometer                      
#include <FastLED.h> // FastLED.h library is for the operation of the addressable LEDS
#include "PinChangeInterrupt.h" // PinChangeInterrupt.h library is used for adding more interrupt pins as the Elegoo nano only has 2



//Accelerometer
const int MPU_ADDR = 0x68;
int16_t accelerometer_x, accelerometer_y, accelerometer_z;    // Value from accelerometer
char tmp_str[7];
char* convert_int16_to_str(int16_t i) {      // COnversion of int to strings
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}
int accX = 0;  // Final value from accelerometer after conversion
int accY = 0;  // Final value from accelerometer after conversion
int accZ = 0;




//LEDS
int i, k, j = 0;   // Values used in loops in LEDS
CRGB leds_1[30];   // LEDS that face forward and down 
CRGB leds_2[15];   // Rear LED lights left
CRGB leds_3[15];   // Rear LED lights right
bool lockFlashOnce  = true;  // Allows the Lock flash fucntion to operate once ibstead of every while loop
bool unLockFlashOnce = true; // Allows the Lock flash fucntion to operate once ibstead of every while loop
int brightnessEnvironment;   // Brightness level surrounding environment
int LED_BRIGHTNESS;   // Brightness set depending o brightness variable
CRGBPalette16 currentPalette;
TBlendType currentBlending;




//Rf
bool onOffState = false;  // Boolean for System power on and off (Attaches to interupt)
bool lockUnlockState = false; // Boolean for locking and unlocking (Attaches to interupt)
bool stateLightSensor = false;  // Boolean for switiching light modes in unlock state (Attaches to interupt)
int countLights = 1;  // Int for switiching between modes (Attaches to interupt)
int OnOffPin = 2; // On off pin interrupt
int lockUnlockPin = 3;  // lock unlock pin interrupt
int lightSensor = 4;  //  light sensor interrupt





//Buzzer
int buzzerPin = 12; // Buzzer pins output for security system




void setup() {
  //Accelerometer
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);  // Begins a transmission to the I2C slave 
  Wire.write(0x6B);                  // PWR_MGMT_1 register
  Wire.write(0);                     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);        // End the transmission


  //Leds
  FastLED.addLeds<WS2812, 8, GRB>(leds_1, 30); // Sets up the LEDS
  FastLED.addLeds<WS2812, 10, GRB>(leds_2, 15); // Sets up the LEDS
  FastLED.addLeds<WS2812, 9, GRB>(leds_3, 15); // Sets up the LEDS
  FastLED.setMaxPowerInVoltsAndMilliamps(5,2000); // Sets up the LEDS max volts and amps


//LightSensor
  pinMode(A6, INPUT);  // OInput pin of the light sensor


  //RF
  pinMode(OnOffPin, INPUT); // Input from RF receiver to produce an interrupt
  attachInterrupt(digitalPinToInterrupt(OnOffPin), switchState, RISING); // Interrupt attachment
  pinMode(lockUnlockPin, INPUT); // Input from RF receiver to produce an interrupt 
  attachInterrupt(digitalPinToInterrupt(lockUnlockPin), switchState1, RISING); // Interrupt attachment
  pinMode(lightSensor, INPUT); // Input from RF receiver to produce an interrupt
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(lightSensor), handleLightButtonPress, RISING); // Interrupt attachment


  //Buzzer
  pinMode(buzzerPin, OUTPUT); // Ouput pin for the buzzer
}

void loop() {

  // On/Off: This actives when system is powered back on and illumintaes 15 leds to shwo this
  while (onOffState == false) {
    fill_solid(leds_1, 5, CRGB(200,200,200));
    fill_solid(leds_2, 5, CRGB(200,0,0));
    fill_solid(leds_3, 5, CRGB(200,0,0));
    FastLED.show();
  }


  


  //locked: This is the locked state of the system
  while (lockUnlockState == true) {
    fill_solid(leds_1, 30, CRGB(0,0,0));
    fill_solid(leds_2, 15, CRGB(0,0,0));
    fill_solid(leds_3, 15, CRGB(0,0,0));
    FastLED.show();
    if (lockFlashOnce == true){
    lock();
    lockFlashOnce = false;
    unLockFlashOnce = true;
    }
      
      

    accelerometerMonitor();  // Method for gathering the g forces meaudred on x and y axis

    if (accX > 1 || accY > 1 || accX < -1 || accY < -1 || accZ < -1 || accZ < -1) {
      digitalWrite(buzzerPin, HIGH);
      for(i=0;i<=20;i++){
        Alarm();
        delay(400);
      }
    }
    digitalWrite(buzzerPin, LOW);
  }




//Unlocked: This is the unlocked state of the system
while (lockUnlockState == false) {
    if (unLockFlashOnce == true){
    unLock();
    unLockFlashOnce = false;
    lockFlashOnce = true;
    }
      fill_solid(leds_2, 15, CRGB(200,0,0));
      fill_solid(leds_3, 15, CRGB(200,0,0));
      FastLED.show();
      delay(3000); 
    while (countLights == 1) { // While loop for the light sensitive mode

      fillWhite();
      delay(1000);
      
      escapeCountWhileLoops();  // Method for checking interrups to change modes
    }

    while (countLights == 2) { // While loop for the slow speed through busy area mode
      fill_solid(leds_1, 30, CRGB(255, 255, 255));
      fill_solid(leds_2, 15, CRGB(255, 0, 0));
      fill_solid(leds_3, 15, CRGB(255, 0, 0));
      FastLED.show();
      escapeCountWhileLoops(); // Method for checking interrups to change modes
      
    }
    
  while (countLights == 3) { // While loop for christmas tree lights (green)
    fill_solid(leds_2, 15, CRGB(200,0,0));
    fill_solid(leds_3, 15, CRGB(200,0,0));
    currentPalette = ForestColors_p;
    static uint8_t index = 0;
    index = index + 1; /* motion speed */
    PaletteColors(index);
    FastLED.show();
    FastLED.delay(10);
    

    escapeCountWhileLoops(); // Method for checking interrups to change modes
  }

  while (countLights == 4) { // While loop for christmas tree lights (red)
    currentPalette = LavaColors_p;
    static uint8_t index = 0;
    index = index + 1; /* motion speed */
    PaletteColors(index);
    FastLED.show();
    FastLED.delay(10);
    
    
    escapeCountWhileLoops(); // Method for checking interrups to change modes
  }
  
  while (countLights == 5) { // // While loop for rainbow colors
    currentPalette = RainbowColors_p;
    static uint8_t index = 0;
    index = index + 1; /* motion speed */
    PaletteColors(index);
    FastLED.show();
    FastLED.delay(10);
    

    escapeCountWhileLoops(); // Method for checking interrups to change modes
  }

  }
}


  // Methods 

  //Interrups methods
void escapeCountWhileLoops() { // Used to check states of booleans in unlocked to switch between modes
  if (onOffState == false) {
    countLights = 10;
    lockUnlockState == true;
  }
  if (lockUnlockState == true) {
    countLights = 10;
    onOffState == true;
  }
}


void switchState() { // Interrupt for on/off boolean switch
  if (onOffState == false) {
    onOffState = true;
  } else {
    onOffState = false;
  }
}


void switchState1() { // Interrupt for lock/unlock boolean switch
  if (lockUnlockState == false) {
    lockUnlockState = true;

  } else {
    lockUnlockState = false;
  }
}


void handleLightButtonPress() { // Interrupt for uncloked state lights modes 
  if (countLights >= 5) {
    countLights = 0;
  }
  countLights ++;
}




// Fastled methods
void lock(void) { // Effect done upon locking the smart bike
  k = 204;
  j = 100;

  for (i = 0; i <= 100; i++) {
    fill_solid(leds_1, 30, CRGB(k, j, 0));
    fill_solid(leds_2, 15, CRGB(k, j, 0));
    fill_solid(leds_3, 15, CRGB(k, j, 0));
    FastLED.show();
    k = k - 2;
    j = j - 1;
    delay(20);
  }
  fill_solid(leds_1, 30, CRGB(0, 0, 0));
  fill_solid(leds_2, 15, CRGB(0, 0, 0));
  fill_solid(leds_3, 15, CRGB(0, 0, 0));
  FastLED.show();
}

void Alarm(void){
  fill_solid(leds_1, 30, CRGB(200, 100, 100));
  fill_solid(leds_2, 15, CRGB(0, 255, 0));
  fill_solid(leds_3, 15, CRGB(0, 0, 255));
  FastLED.show();
  delay(400);
  fill_solid(leds_1, 30, CRGB(0, 0, 0));
  fill_solid(leds_2, 15, CRGB(0, 0, 255));
  fill_solid(leds_3, 15, CRGB(0, 255, 0));
  FastLED.show();
}


void unLock(void) { // Effect done upon unlocking the smart bike
  fill_solid(leds_1, 30, CRGB(204, 100, 0));
  fill_solid(leds_2, 15, CRGB(204, 100, 0));
  fill_solid(leds_3, 15, CRGB(204, 100, 0));
  FastLED.show();
  delay(200);
  fill_solid(leds_1, 30, CRGB(0, 0, 0));
  fill_solid(leds_2, 15, CRGB(0, 0, 0));
  fill_solid(leds_3, 15, CRGB(0, 0, 0));
  FastLED.show();
  delay(200);
  fill_solid(leds_1, 30, CRGB(204, 100, 0));
  fill_solid(leds_2, 15, CRGB(204, 100, 0));
  fill_solid(leds_3, 15, CRGB(204, 100, 0));
  FastLED.show();
  delay(200);
  fill_solid(leds_1, 30, CRGB(0, 0, 0));
  fill_solid(leds_2, 15, CRGB(0, 0, 0));
  fill_solid(leds_3, 15, CRGB(0, 0, 0));
  FastLED.show();
  delay(500);
}



void fillWhite() { // Method for reading lightsensor and adjusting LED intensity accordingly
  brightnessEnvironment = analogRead(A6);
  if (brightnessEnvironment <= 100) {
    LED_BRIGHTNESS = 200;
  }
  if (brightnessEnvironment > 100 && brightnessEnvironment <= 250) {
    LED_BRIGHTNESS = 100;
  }
  if (brightnessEnvironment > 250 && brightnessEnvironment <= 750) {
    LED_BRIGHTNESS = 50;
  }
  if (brightnessEnvironment > 750) {
    LED_BRIGHTNESS = 10;
  }

  fill_solid(leds_1, 30, CRGB(LED_BRIGHTNESS, LED_BRIGHTNESS, LED_BRIGHTNESS));
  FastLED.show();
}


void PaletteColors(uint8_t index) { // Method for doing special light effects for unlocked state
  uint8_t brightness = 70;

  for (int i = 0; i < 30; ++i) {
    leds_1[i] = ColorFromPalette(currentPalette, index, brightness, currentBlending);
    index += 10;
    
  }
  
}




//Accelerometer method
void accelerometerMonitor() { // Method for monitoring acceleromater readings
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 7 * 2, true);

  accelerometer_x = Wire.read() << 8 | Wire.read();
  accelerometer_y = Wire.read() << 8 | Wire.read();
  accelerometer_z = Wire.read() << 8 | Wire.read();

  convert_int16_to_str(accelerometer_x);
  convert_int16_to_str(accelerometer_y);
  convert_int16_to_str(accelerometer_z);
  accX = accelerometer_x / 10000;
  accY = accelerometer_y / 10000;
  accZ = accelerometer_z / 10000;
}
