#include <esp_now.h>
#include <WiFi.h>
#include <NewPing.h>

#define SENSOR  27 
#define echoPin 18 // attach pin GPIO18 to pin Echo of JSN-SR04
#define trigPin 5  // attach pin GPIO5 ESP32 to pin Trig of JSN-SR04                     
#define MAX_DISTANCE 200

long duration; // Time taken for the pulse to reach the receiver
int distance; 

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
NewPing sonar(trigPin, echoPin, MAX_DISTANCE); // NewPing object

// REPLACE WITH THE RECEIVER'S MAC Address -- 0xC8, 0xF0, 0x9E, 0x85, 0xA5, 0xC4
uint8_t broadcastAddress[] = {0xC8, 0xF0, 0x9E, 0x85, 0xA5, 0xC4};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    int id; // must be unique for each sender board
    float x;
    float y;
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

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200); 

  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

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
  
  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {

  pulse1Sec = pulseCount;
  pulseCount = 0;

  flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;   previousMillis = millis();
  flowMilliLitres = (flowRate / 60) * 1000;
  totalMilliLitres += flowMilliLitres;
    // Print the flow rate for this second in litres / minute

  digitalWrite(trigPin, LOW);
  delayMicroseconds(5); 

  digitalWrite(trigPin, HIGH); // turn on the Trigger to generate pulse
  delayMicroseconds(10); // keep the trigger "ON" for 10 ms to generate pulse
  digitalWrite(trigPin, LOW); // Turn off the pulse trigger to stop pulse

  // If pulse reached the receiver echoPin
  // become high Then pulseIn() returns the
  // time taken by the pulse to reach the receiver
  //duration = pulseIn(echoPin, HIGH);
  //distance = duration * 0.0344 / 2; 
  unsigned int distance = sonar.ping_cm();

  // Speed of sound in air at 20 degrees Celsius is approximately 343 meters per second
  // Convert microseconds to seconds by dividing by 1000000
  // Divide by 2 to account for round trip travel time
  float waterlevel = MAX_DISTANCE - distance;

  Serial.print("Flow rate: ");
  Serial.print(float(flowRate));  // Print the integer part of the variable
  Serial.println(" L/min");
  Serial.print("Water Level: ");
  Serial.print(waterlevel);
  Serial.println(" cm");
  delay(1000);

  // Set values to send
  myData.id = 2;
  myData.x = flowRate;
  myData.y = waterlevel;

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
  esp_sleep_enable_timer_wakeup(10 * 1000000); // 10 seconds
  esp_deep_sleep_start();
}
}
