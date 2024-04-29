#include "DHT.h"
#include <esp_now.h>
#include <WiFi.h>
#include "RTClib.h"
#include <Wire.h>

#define DHTPIN 4     
#define DHTTYPE DHT22   
DHT dht(DHTPIN, DHTTYPE);
#define RainPin 2 

bool bucketPositionA = false;             // one of the two positions of tipping-bucket               
const double bucketAmount = 0.4090909;   // inches equivalent of ml to trip tipping-bucket
double dailyRain = 0.0;                   // rain accumulated for the day
double minuteRain = 0.0;                  // rain accumulated for one hour
double dailyRain_till_LastMinute = 0.0;     // rain accumulated for the day till the last hour          
bool first;                               // as we want readings of the (MHz) loops only at the 0th moment 

RTC_Millis rtc;

// Variables to store sensor readings
float humidity, temperature_C, temperature_F;

// Correction factor for humidity
float humidityCorrectionFactor = 0.87; // Adjust this value as needed

uint8_t broadcastAddress[] = {0xC8, 0xF0, 0x9E, 0x85, 0xA5, 0xC4};

typedef struct struct_message {
    int id; // must be unique for each sender board
    float x;
    float y;
    float z;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create peer interface
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  rtc.begin(DateTime(__DATE__, __TIME__));       // start the RTC
  pinMode(RainPin, INPUT);  

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {

  DateTime now = rtc.now();

  if ((bucketPositionA==false)&&(digitalRead(RainPin)==HIGH)){
    bucketPositionA=true;
    dailyRain+=bucketAmount;                               // update the daily rain
  }
  
  if ((bucketPositionA==true)&&(digitalRead(RainPin)==LOW)){
    bucketPositionA=false;  
  } 

  if(now.second() != 0) first = true;

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  temperature_C = dht.readTemperature();
  temperature_F = dht.readTemperature(true);
  // Read temperature as Celsius (the default)
  humidity = dht.readHumidity();

  // Apply correction factor to humidity reading
  humidity *= humidityCorrectionFactor;

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature_C) || isnan(temperature_F)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    delay(100);
    return;
  }

  if(now.second() == 0 && first == true) {
  minuteRain = dailyRain - dailyRain_till_LastMinute;      // calculate the last hour's rain
  dailyRain_till_LastMinute = dailyRain;                   // update the rain till last hour for next calculation

  Serial.print("Temperature: ");
  Serial.print(temperature_C);
  Serial.println(F("°C"));
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(F("%"));
  Serial.print("Rain in last minute = ");
  Serial.print(minuteRain,2);
  Serial.println("mm");

  first = false;                                        // execute calculations only once per hour

  if(now.minute()== 0) {
  dailyRain = 0.0;                                      // clear daily-rain at midnight
  dailyRain_till_LastMinute = 0.0;                        // we do not want negative rain at 01:00
  }

  myData.id = 1;
  myData.x = temperature_C;
  myData.y = humidity;
  myData.z = minuteRain;
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(1000);

  // Deep sleep for 10 seconds using timer
  //esp_sleep_enable_timer_wakeup(10 * 1000000); // 10 seconds
  //esp_deep_sleep_start();
  }
}