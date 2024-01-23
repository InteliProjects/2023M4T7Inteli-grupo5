/****************************************
 * Include Libraries
 ****************************************/
#include "UbidotsEsp32Mqtt.h"
#include <esp_now.h>
#include <WiFi.h>

/****************************************
 * Define constants and global variables
 ****************************************/
#define CHANNEL 3

const int pinBlue = 25;

int key = 0; // control if the device is connect to ubidots or espNow
const int changeFrequency = 20000; // Seconds to change connection between Ubidots and Esp Now
int controlSetup = 0; // Variable to ensure that the setup function for each connection is executed only once - Ubidots or Esp Now

char macStr[18]; // Receives the MAC address
char receivedData[20];  // Receives the location of the emergency

unsigned long timer;

/****************************************
 * Define Ubidots Constants
 ****************************************/
const char *UBIDOTS_TOKEN = ""; // Add your Ubidots Token
const char *WIFI_SSID = ""; // Add your Wi-Fi SSID
const char *WIFI_PASS = ""; // Add your Wi-Fi password
const char *PUBLISH_DEVICE_LABEL = ""; // Add your Ubidots Device
const char *OWNER_VARIABLE_LABEL = "";   // DEVICE'S OWNER VARIABLE

Ubidots ubidots(UBIDOTS_TOKEN);


/****************************************
 * Main Functions
 ****************************************/

void setup()
{
  pinMode(pinBlue, OUTPUT);
  Serial.begin(115200);
  timer = millis();
}

void loop()
{

  // key == 0 -> Connect to Ubidots
  // Run setupUbidots only once and loop repeatedly
  if(key == 0)
  {
    if(controlSetup == 0)
    {
      setupUbidots();
      controlSetup = 1;
    }

    loopUbidots();
  }

  // key == 1 -> Connect to Esp Now
  // Run setupEspNow only once and has no loop
  else if(key == 1)
  {
    if(controlSetup == 0)
    {
      setupEspNow();
      controlSetup = 1;
    }
  }
  
  if (millis() - timer > changeFrequency && key == 0) // Changes the key to 1 every 20 seconds
  {
    ubidots.disconnect();
    key = 1;
    controlSetup = 0;
    timer = millis();
  }
}


/****************************************
 * UBIDOTS (key = 0);
 ****************************************/

// Starts the ubidots connection and send the location to the Ubidots variable
void setupUbidots()
{
  ubidots.setDebug(true);
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setup();
  ubidots.reconnect();

  sendLocation();
}

// Reconnect and subscribe
void loopUbidots()
{
  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }

  ubidots.loop();
}

// Update the owner location every 20 seconds
void sendLocation()
{
  char* context = (char*)malloc(sizeof(char) * 60);

  ubidots.addContext("id", macStr);
  ubidots.addContext("sala", receivedData);
  ubidots.getContext(context);
  ubidots.add(OWNER_VARIABLE_LABEL, 0, context);
  ubidots.publish(PUBLISH_DEVICE_LABEL);
}



/****************************************
 * ESP NOW (key = 1)
 ****************************************/

// Connect to the Esp Now as a Slave
void setupEspNow()
{
  Serial.println("ESPNow/Basic/Slave Example");
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);
}

void InitESPNow()
{
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

// Config AP SSID
void configDeviceAP()
{
  const char *SSID = "Slave_1";
  
  WiFi.mode(WIFI_STA);
  delay(100);

  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    Serial.print("AP CHANNEL "); Serial.println(WiFi.channel());
  }
}

// When the Slave receives the room's name from the Esp Master, changes the key to 0 and reconnect to Ubidots, sending this info to the dashboard
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  for (int i = 0; i < data_len; i++) {
    receivedData[i] = (char)data[i];
  }
  receivedData[data_len] = '\0';  // Null-terminate the string

  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(receivedData);
  Serial.println("");

  WiFi.disconnect();
  key = 0;
  controlSetup = 0;
}
