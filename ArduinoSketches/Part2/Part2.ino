#include "Adafruit_TCS34725.h"
#include "Adafruit_I2CDevice.h"

/* Initialise with default values (int time = 2.4ms, gain = 1x) */
Adafruit_TCS34725 tcs = Adafruit_TCS34725();

const int TCS_SCL = 5; // Yellow wire
const int TCS_SDA = 4; // Blue wire
const int TCS_LED = 0; // Green wire

uint16_t r, g, b, c, colorTemp, lux;

const int LED_red = 12; // Red LED
const int LED_yellow = 13; // Yellow LED
const int LED_green = 15; // Green LED

// low -> green LED on, medium -> yellow LED on, high -> red LED on
int upper_threshold = 70;
int lower_threshold = 20;

int lux_values = 0;
unsigned long lux_previousMillis = 0; // To store the last time a lux reading was taken
int lux_interval = 1000;        // Read the lux value once every second

int lux_numSamples = 0;
int avg_lux = 0;
unsigned long luxAvg_previousMillis = 0; // Timer for sampling
int luxAvg_interval = 5000;        // Turn the onboard LED ON if the average value in a window of X seconds is higher than a threshold of Y 
int Y_threshold = 40;

void setup() {
  Serial.begin(115200);

  pinMode(TCS_LED, OUTPUT);
  pinMode(LED_red, OUTPUT);
  pinMode(LED_yellow, OUTPUT);
  pinMode(LED_green, OUTPUT);

  digitalWrite(TCS_LED, LOW);
  digitalWrite(LED_red, LOW);
  digitalWrite(LED_yellow, LOW);
  digitalWrite(LED_green, LOW);

  if (tcs.begin()) {
    Serial.println("TCS RGB Sensor connected");
  } 
  else {
    Serial.println("TCS RGB Sensor not found -> check connections");
    while (1); // enter an infinite loop and stop further execution
  }
}

void loop() {
  unsigned long lux_currentMillis = millis();
  
  // Check if it's time to take a new reading (every one second)
  if (lux_currentMillis - lux_previousMillis >= lux_interval) {
    tcs.getRawData(&r, &g, &b, &c);
    colorTemp = tcs.calculateColorTemperature(r, g, b);
    lux = tcs.calculateLux(r, g, b);

    // Check if the average lux is low or medium or high
    // low -> green LED on, medium -> yellow LED on, high -> red LED on
    if (lux <= lower_threshold) {
      Serial.println("LOW");
      digitalWrite(LED_red, LOW);
      digitalWrite(LED_yellow, LOW);
      digitalWrite(LED_green, HIGH);
    }
    else if (lux >= upper_threshold) {
      Serial.println("HIGH");
      digitalWrite(LED_red, HIGH);
      digitalWrite(LED_yellow, LOW);
      digitalWrite(LED_green, LOW);
    }
    else {
      Serial.println("MEDIUM");
      digitalWrite(LED_red, LOW);
      digitalWrite(LED_yellow, HIGH);
      digitalWrite(LED_green, LOW);
    }

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

    Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" | ");
    Serial.println(" ");
    lux_previousMillis = lux_currentMillis;
  }
}