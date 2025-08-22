#include "Adafruit_TCS34725.h"
#include "Adafruit_I2CDevice.h"
#include "Adafruit_TCS34725.h"
#include "LiquidCrystal_I2C.h"
#include "Wire.h"

// LCD configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address 0x27, 16 columns, 2 rows

// Initialise with default values (int time = 2.4ms, gain = 1x)
Adafruit_TCS34725 tcs = Adafruit_TCS34725();
// Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

const int TCS_LED = 10;

uint16_t r, g, b, c, colorTemp, lux;

const int LED_red = 12; // Red LED
const int LED_yellow = 13; // Yellow LED
const int LED_green = 15; // Green LED

const int encod_CLK = 2;
const int encod_DT = 0;
const int encod_button = 9;

const int button = 14; // Button for LCD Control

int buttonState = 0;
int lastButtonState = 0;
int part = 0;

int encod_lastState_CLK;
int encod_currentState_CLK;
int encod_buttonState = 0;
int encod_lastButtonState = 0;

// low -> green LED on, medium -> yellow LED on, high -> red LED on
int upper_threshold = 70;
int lower_threshold = 20;
String status;

int lux_values = 0;
unsigned long lux_previousMillis = 0; // To store the last time a lux reading was taken
int lux_interval = 1000; // Read the lux value once every second

int lux_numSamples = 0;
int lux_avg = 0;
unsigned long luxAvg_previousMillis = 0; // Timer for sampling
int luxAvg_interval = 5000; // Turn the onboard LED ON if the average value in a window of X seconds is higher than a threshold of Y 
int luxAvg_interval_sec = luxAvg_interval / 1000;

// We have:
  // interval at Y_threshold: x = 20 -> y = 1050 (ms)
  // interval at Z_threshold: x = 150 -> y = 10 (ms)
int Y_threshold = 20;
int Y_threshold_base = 20;
int Y_interval = 1050;
int Z_threshold = 150;
int Z_interval = 10;

int lux_numReadings; // Number of readings in a sliding window of X seconds
int *lux_readings;  // Pointer for dynamic array
int lux_index = 0; // Index of the current reading
int lux_total = 0; // Sum of the readings
int lux_movingAvg = 0; // The calculated moving average

bool led_state = LOW;
int led_interval = 0;
int slope, intercept;
unsigned long led_previousMillis = 0;  // will store last time LED was updated

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

void part1(int ledPin, int lux_value){
  unsigned long luxAvg_currentMillis = millis(); // Track current time
  // Check if it's time to calculate lux average
  if (luxAvg_currentMillis - luxAvg_previousMillis >= luxAvg_interval) {
    luxAvg_previousMillis = luxAvg_currentMillis;
    // Calculate average lux over the window
    lux_avg = lux_values / lux_numSamples;
    // Check if the average lux exceeds the threshold
    if (lux_avg >= Y_threshold) { digitalWrite(ledPin, HIGH); } // Turn on the ESP LED
    else { digitalWrite(ledPin, LOW); } // Turn off the ESP LED
    lux_numSamples = 0;
    lux_values = 0;
  }
  else {
    lux_values += lux_value;
    lux_numSamples ++;
  }
}

void part2 (int lux_value){
  // Check if the average lux is low or medium or high
  // low -> green LED on, medium -> yellow LED on, high -> red LED on
  if (lux <= lower_threshold) {
    // Serial.println("LOW");
    digitalWrite(LED_red, LOW);
    digitalWrite(LED_yellow, LOW);
    digitalWrite(LED_green, HIGH);
    status = "low";
  }
  else if (lux >= upper_threshold) {
    // Serial.println("HIGH");
    digitalWrite(LED_red, HIGH);
    digitalWrite(LED_yellow, LOW);
    digitalWrite(LED_green, LOW);
    status = "high";
  }
  else {
    // Serial.println("MEDIUM");
    digitalWrite(LED_red, LOW);
    digitalWrite(LED_yellow, HIGH);
    digitalWrite(LED_green, LOW);
    status = "norm";
  }
}

void part3(int ledPin, int lux_value, int a, int b){
  if (lux_movingAvg > Y_threshold){
    // Linear function to calculate led_interval: 
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

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(TCS_LED, OUTPUT);
  pinMode(LED_red, OUTPUT);
  pinMode(LED_yellow, OUTPUT);
  pinMode(LED_green, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(encod_CLK, INPUT);
  pinMode(encod_DT, INPUT);
  pinMode(encod_button, INPUT_PULLUP);

  digitalWrite(TCS_LED, LOW);
  digitalWrite(LED_red, LOW);
  digitalWrite(LED_yellow, LOW);
  digitalWrite(LED_green, LOW);

  lux_numReadings = luxAvg_interval / lux_interval;  // Calculate at runtime
  lux_readings = new int[lux_numReadings];  // Dynamically allocate memory for the array

  encod_lastState_CLK = digitalRead(encod_CLK);

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
}

void loop() {
  tcs.getRawData(&r, &g, &b, &c);
  lux = tcs.calculateLux(r, g, b);
  buttonState = digitalRead(button);
  encod_buttonState = digitalRead(encod_button);
  encod_currentState_CLK = digitalRead(encod_CLK);

  // Check if the button is pressed
  if(lastButtonState == LOW && buttonState == HIGH) {
    part = (part % 3) + 1; // Button is pressed, increment part from 1 to 3 cyclically
  }
  lastButtonState = buttonState;

  // Check if the state has changed
  if (encod_currentState_CLK != encod_lastState_CLK) {
    // Check the direction of rotation
    if (digitalRead(encod_DT) != encod_currentState_CLK) {
      Y_threshold++;
    } else {
      Y_threshold--;
    }

    Y_threshold = constrain(Y_threshold, 0, 100); // Constrain the Y_threshold between 0 and 100
  }

  encod_lastState_CLK = encod_currentState_CLK; // Update encod_lastState_CLK with the current state

  // Check if the button is pressed
  if(encod_lastButtonState == LOW && encod_buttonState == HIGH) {
    Y_threshold = Y_threshold_base;
  }
  encod_lastButtonState = encod_buttonState;

  unsigned long lux_currentMillis = millis();
  
  // Switch case based on part
  switch (part) {

    case 1:
      
      // Check if it's time to take a new reading (every one second)
      if (lux_currentMillis - lux_previousMillis >= lux_interval) {
        lux_previousMillis = lux_currentMillis;

        part1(TCS_LED, lux);

        // Display information on LCD
        lcd.clear();
        lcd.setCursor(0, 0); // First row
        lcd.print("P"); lcd.print(part); 
        lcd.print(" W:"); lcd.print(luxAvg_interval_sec); lcd.print("s");
        lcd.print(" T:"); lcd.print(Y_threshold); 

        lcd.setCursor(0, 1); // Second row
        lcd.print("L:"); lcd.print(lux); 
        lcd.print(" A:"); lcd.print(lux_avg);
      
        Serial.print("Part: "); Serial.print(part, DEC); Serial.print(" | ");
        Serial.print("Window: "); Serial.print(luxAvg_interval_sec, DEC); Serial.print("s | ");
        Serial.print("Threshold: "); Serial.print(Y_threshold, DEC); Serial.print(" | ");
        Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" | ");
        Serial.print("Avg Lux: "); Serial.print(lux_avg, DEC);
        Serial.println(" ");
      }
      break;

    case 2:
      // Take a new lux reading every one second
      if (lux_currentMillis - lux_previousMillis >= lux_interval) {
        lux_previousMillis = lux_currentMillis;

        part2(lux);

        // Display information on LCD
        lcd.clear();
        lcd.setCursor(0, 0);  // First row
        lcd.print("P"); lcd.print(part); 
        lcd.print(" T:"); lcd.print(lower_threshold); lcd.print("-"); lcd.print(upper_threshold); 

        lcd.setCursor(0, 1);  // Second row
        lcd.print("L:"); lcd.print(lux); 
        lcd.print(" Lv:"); lcd.print(status);

        Serial.print("Part: "); Serial.print(part, DEC); Serial.print(" | ");
        Serial.print("Upper Threshold: "); Serial.print(upper_threshold, DEC); Serial.print(" | ");
        Serial.print("Lower Threshold: "); Serial.print(lower_threshold, DEC); Serial.print(" | ");
        Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" | ");
        Serial.print("Status: "); Serial.print(status);
        Serial.println(" ");
      }
      break;

    case 3:
      // Get slope and intercept for the linear equation of flashing interval
      getSlopeIntercept(Y_threshold, Y_interval, Z_threshold, Z_interval, &slope, &intercept);
      part3(TCS_LED, lux, slope, intercept);
      // Take a new lux reading every one second
      if (lux_currentMillis - lux_previousMillis >= lux_interval) {
        lux_previousMillis = lux_currentMillis;

        cal_movingAvg (lux);

        // Display information on LCD
        lcd.clear();
        lcd.setCursor(0, 0); // First row
        lcd.print("P"); lcd.print(part);
        lcd.print(" "); lcd.print(luxAvg_interval_sec); lcd.print("s");
        lcd.print(" L:"); lcd.print(lux);
        lcd.print(" T:"); lcd.print(Y_threshold);
        
        lcd.setCursor(0, 1); // Second row
        lcd.print("MA:"); lcd.print(lux_movingAvg);
        lcd.print(" I:"); lcd.print(led_interval); lcd.print("ms");

        Serial.print("Part: "); Serial.print(part, DEC); Serial.print(" | ");
        Serial.print("Window: "); Serial.print(luxAvg_interval_sec, DEC); Serial.print("s | ");
        Serial.print("Threshold: "); Serial.print(Y_threshold, DEC); Serial.print(" | ");
        Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" | ");
        Serial.print("Moving Avg Lux: "); Serial.print(lux_movingAvg, DEC); Serial.print(" | ");
        Serial.print("Flashing Interval: "); Serial.print(led_interval, DEC); Serial.print("ms | ");
        Serial.println(" ");
      }
      break;
  }
}