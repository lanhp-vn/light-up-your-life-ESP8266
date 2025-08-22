#include "Adafruit_TCS34725.h"
#include "Adafruit_I2CDevice.h"

/* Initialise with default values (int time = 2.4ms, gain = 1x) */
Adafruit_TCS34725 tcs = Adafruit_TCS34725();

const int TCS_LED = 10;
const int ESP_LED = 2; // Built-in LED

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
int lux_avg = 0;
unsigned long luxAvg_previousMillis = 0; // Timer for sampling
int luxAvg_interval = 5000;        // Turn the onboard LED ON if the average value in a window of X seconds is higher than a threshold of Y 

// We have:
  // interval at Y_threshold: x = 20 -> y = 1050 (ms)
  // interval at Z_threshold: x = 150 -> y = 10 (ms)
int Y_threshold = 20;
int Y_interval = 1050;
int Z_threshold = 150;
int Z_interval = 10;

int lux_numReadings; // Number of readings in a sliding window of X seconds
int *lux_readings;  // Pointer for dynamic array
int lux_index = 0;               // Index of the current reading
int lux_total = 0;               // Sum of the readings
int lux_movingAvg = 0;             // The calculated moving average

bool led_state = LOW;
int led_interval = 0;
int slope, intercept;
unsigned long led_previousMillis = 0;  // will store last time LED was updated

void ledColor_change (int lux_value){
  // Check if the average lux is low or medium or high
  // low -> green LED on, medium -> yellow LED on, high -> red LED on
  if (lux <= lower_threshold) {
    // Serial.println("LOW");
    digitalWrite(LED_red, LOW);
    digitalWrite(LED_yellow, LOW);
    digitalWrite(LED_green, HIGH);
  }
  else if (lux >= upper_threshold) {
    // Serial.println("HIGH");
    digitalWrite(LED_red, HIGH);
    digitalWrite(LED_yellow, LOW);
    digitalWrite(LED_green, LOW);
  }
  else {
    // Serial.println("MEDIUM");
    digitalWrite(LED_red, LOW);
    digitalWrite(LED_yellow, HIGH);
    digitalWrite(LED_green, LOW);
  }
}

void cal_movingAvg (int lux_value){
  lux_total = lux_total - lux_readings[lux_index]; // Subtract the last reading from the lux_total
  lux_readings[lux_index] = lux_value; // Take a new sensor reading
  lux_total = lux_total + lux_readings[lux_index]; // Add the new reading to the lux_total
  lux_index = (lux_index + 1) % lux_numReadings; // Advance to the next lux_index (circular buffer)
  lux_movingAvg = lux_total / lux_numReadings; // Calculate the moving average
}

void getSlopeIntercept(int x1, int y1, int x2, int y2, int *a, int *b) {
  // Calculate slope (a)
  *a = (y2 - y1) / (x2 - x1);
  // Calculate intercept (b)
  *b = y1 - (*a) * x1;
}

void ledFlash(int ledPin, int lux_value, int a, int b){
  if (lux_movingAvg > Y_threshold){
    // We have:
      // interval at Y_threshold: x = 20 -> y = 1050 (ms)
      // interval at: x = 150 -> y = 10 (ms)
      // => Linear function to calculate led_interval: 
    led_interval = a*lux_value + b;
    if (led_interval <= 10){ led_interval = 10;}

    unsigned long led_currentMillis = millis();
    if (led_currentMillis - led_previousMillis >= led_interval) {
      led_previousMillis = led_currentMillis;
      if (led_state == LOW){ led_state = HIGH; }
      else{ led_state = LOW; }
      digitalWrite(ledPin, led_state);
    }
  }
  else {
    digitalWrite(ledPin, LOW);
    led_interval = 0;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TCS_LED, OUTPUT);
  pinMode(ESP_LED, OUTPUT);
  pinMode(LED_red, OUTPUT);
  pinMode(LED_yellow, OUTPUT);
  pinMode(LED_green, OUTPUT);

  digitalWrite(TCS_LED, LOW);
  digitalWrite(ESP_LED, HIGH);
  digitalWrite(LED_red, LOW);
  digitalWrite(LED_yellow, LOW);
  digitalWrite(LED_green, LOW);

  lux_numReadings = luxAvg_interval / lux_interval;  // Calculate at runtime
  lux_readings = new int[lux_numReadings];  // Dynamically allocate memory for the array

  // Initialize the lux array to 0
  for (int i = 0; i < lux_numReadings; i++) {
  lux_readings[i] = 0;
  }

  if (tcs.begin()) {
    Serial.println("TCS RGB Sensor connected");
  }
  else {
    Serial.println("TCS RGB Sensor not found -> check connections");
    while (1); // enter an infinite loop and stop further execution
  }

  // Get slope and intercept for the linear equation of flashing interval
  getSlopeIntercept(Y_threshold, Y_interval, Z_threshold, Z_interval, &slope, &intercept);
}

void loop() {
  tcs.getRawData(&r, &g, &b, &c);
  lux = tcs.calculateLux(r, g, b);

  ledFlash(TCS_LED, lux, slope, intercept);
  
  // Check if it's time to take a new reading (every one second)
  unsigned long lux_currentMillis = millis();
  if (lux_currentMillis - lux_previousMillis >= lux_interval) {
    lux_previousMillis = lux_currentMillis;

    ledColor_change(lux);
    cal_movingAvg(lux);

    unsigned long luxAvg_currentMillis = millis(); // Track current time
    // Check if it's time to calculate lux average
    if (luxAvg_currentMillis - luxAvg_previousMillis >= luxAvg_interval) {
      // Calculate average lux over the window
      lux_avg = lux_values / lux_numSamples;
      // Check if the average lux exceeds the threshold
      if (lux_avg >= Y_threshold) { digitalWrite(ESP_LED, LOW); } // Turn on the ESP LED
      else { digitalWrite(ESP_LED, HIGH); } // Turn off the ESP LED
      luxAvg_previousMillis = luxAvg_currentMillis;
      lux_numSamples = 0;
      lux_values = 0;
    }
    else {
      lux_values += lux;
      lux_numSamples ++;
    }

  Serial.print("Time Window: "); Serial.print(luxAvg_interval, DEC); Serial.print("ms | ");
  Serial.print("Y Threshold: "); Serial.print(Y_threshold, DEC); Serial.print(" | ");
  Serial.print("Upper Threshold: "); Serial.print(upper_threshold, DEC); Serial.print(" | ");
  Serial.print("Lower Threshold: "); Serial.print(lower_threshold, DEC); Serial.print(" | ");
  Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" | ");
  Serial.print("Avg Lux: "); Serial.print(lux_avg, DEC); Serial.print(" | ");
  Serial.print("Moving Avg Lux: "); Serial.print(lux_movingAvg, DEC); Serial.print(" | ");
  Serial.print("Flashing Interval: "); Serial.print(led_interval, DEC); Serial.print("ms | ");
  Serial.println(" ");
  }
}