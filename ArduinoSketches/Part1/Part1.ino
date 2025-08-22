#include "Adafruit_TCS34725.h"
#include "Adafruit_I2CDevice.h"

const int TCS_SCL = 5; // Yellow wire
const int TCS_SDA = 4; // Blue wire
const int TCS_LED = 10;

uint16_t r, g, b, c, colorTemp, lux;

/* Initialise with default values (int time = 2.4ms, gain = 1x) */
Adafruit_TCS34725 tcs = Adafruit_TCS34725();

int lux_values = 0;
unsigned long lux_previousMillis = 0; // To store the last time a lux reading was taken
int lux_interval = 1000;        // Read the lux value once every second

int lux_numSamples = 0;
int avg_lux = 0;
unsigned long luxAvg_previousMillis = 0; // Timer for sampling
int luxAvg_interval = 5000;        // Turn the onboard LED ON if the average value in a window of X seconds is higher than a threshold of Y 
int Y_threshold = 40;

void setup(void) {
  Serial.begin(115200);

  pinMode(TCS_LED, OUTPUT);
  digitalWrite(TCS_LED, LOW);

  if (tcs.begin()) {
    Serial.println("TCS RGB Sensor connected");
  } 
  else {
    Serial.println("TCS RGB Sensor not found -> check connections");
    while (1); // enter an infinite loop and stop further execution
  }
}

void loop(void) {

  unsigned long lux_currentMillis = millis();
  
  // Check if it's time to take a new reading (every one second)
  if (lux_currentMillis - lux_previousMillis >= lux_interval) {
    tcs.getRawData(&r, &g, &b, &c);
    lux = tcs.calculateLux(r, g, b);

    unsigned long luxAvg_currentMillis = millis(); // Track current time
    
    // Check if it's time to calculate lux average
    if (luxAvg_currentMillis - luxAvg_previousMillis >= luxAvg_interval) {
      // Calculate average lux over the window
      avg_lux = lux_values / lux_numSamples;
      Serial.print("Avg Lux: "); Serial.print(avg_lux, DEC);
      Serial.println(" ");
      // Check if the average lux exceeds the threshold
      if (avg_lux >= Y_threshold) {
        digitalWrite(TCS_LED, HIGH); // Turn on the LED
      } 
      else {
        digitalWrite(TCS_LED, LOW);  // Turn off the LED
      }
      luxAvg_previousMillis = luxAvg_currentMillis;
      lux_numSamples = 0;
      lux_values = 0;
    }
    else {
      lux_values += lux;
      lux_numSamples ++;
    }  

    Serial.print("Lux: "); Serial.print(lux, DEC);
    Serial.println(" ");
    lux_previousMillis = lux_currentMillis;
  }
}
