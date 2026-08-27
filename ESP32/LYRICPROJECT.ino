// ====================================================================
// SMART BOTANICAL IRRIGATION SYSTEM (VIRTUAL UNPLUG FIX)
// ====================================================================

#include "driver/rtc_io.h" 

const int SENSOR_POWER_PIN = 25;  
const int SENSOR_ANALOG_PIN = 34; 
const int PUMP_RELAY_PIN = 13;   // Using Pin 13

const int DRY_THRESHOLD = 2450; 
const int WATERING_DURATION = 5000; 

void setup() {
  Serial.begin(115200);
  
  // THE VIRTUAL UNPLUG: Set as INPUT to mimic pulling the wire out.
  // This physically disconnects the signal, forcing the 5V relay to stay OFF.
  pinMode(PUMP_RELAY_PIN, INPUT); 
  pinMode(SENSOR_POWER_PIN, OUTPUT);

  delay(2000); 
  Serial.println("\n--- System Waking Up ---");

  // Power up sensor
  Serial.println("Powering up the moisture sensor...");
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  delay(250); 

  int moistureValue = analogRead(SENSOR_ANALOG_PIN);
  Serial.print("Current Soil Moisture Reading: ");
  Serial.println(moistureValue);

  // Watering Logic
  if (moistureValue >= DRY_THRESHOLD) {
    Serial.println("Result: Soil is DRY! Turning pump ON...");
    
    // To turn ON: Reconnect the pin as an output and pull to Ground
    pinMode(PUMP_RELAY_PIN, OUTPUT);
    digitalWrite(PUMP_RELAY_PIN, LOW);   
    
    delay(WATERING_DURATION); 
    
    // To turn OFF: "Unplug" the pin internally again
    pinMode(PUMP_RELAY_PIN, INPUT);  
    Serial.println("Watering cycle complete.");
  } else {
    Serial.println("Result: Soil is WET. Keeping pump OFF.");
    pinMode(PUMP_RELAY_PIN, INPUT); // Ensure it stays "unplugged"
  }

  // Shut down sensor power
  Serial.println("Shutting down moisture sensor power...");
  digitalWrite(SENSOR_POWER_PIN, LOW); 
  
  gpio_hold_dis((gpio_num_t)SENSOR_POWER_PIN);

  // Isolate pins and sleep
  esp_sleep_config_gpio_isolate(); 
  Serial.println("Entering deep sleep for 1 hour to conserve power...");
  esp_sleep_enable_timer_wakeup(15ULL * 1000000ULL); 
  esp_deep_sleep_start();
}

void loop() {
  // Stays empty
}