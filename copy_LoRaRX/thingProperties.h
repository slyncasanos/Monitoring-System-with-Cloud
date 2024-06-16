// Code generated by Arduino IoT Cloud, DO NOT EDIT.

#include <ArduinoIoTCloud.h>
#include  <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[]  = "1971d7c0-8f87-400e-a7cb-cc3bf148fb2e";

const char SSID[]               = "PLDTHOMEFIBR62600";    // Network SSID (name)
const char PASS[]               = "PLDTWIFItap6k";    // Network password (use for WPA, or use as key  for WEP)
const char DEVICE_KEY[]  = "MhYF5nPapKIDHmr28EC?MfiQI";    //  Secret device password

// Code generated by Arduino IoT Cloud, DO NOT EDIT.

void onCiaoStringChange();

String ciaoString;
CloudTemperatureSensor  temperature;
CloudRelativeHumidity humidity;
float rainfall;
float flow;
float level;

void initProperties(){

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(ciaoString, READWRITE, ON_CHANGE, onCiaoStringChange);
  ArduinoCloud.addProperty(temperature, READ, ON_CHANGE,  NULL);
  ArduinoCloud.addProperty(humidity, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(rainfall, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(flow, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(level, READ, ON_CHANGE, NULL);

}

WiFiConnectionHandler  ArduinoIoTPreferredConnection(SSID, PASS);