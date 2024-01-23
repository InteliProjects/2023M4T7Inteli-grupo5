
/****************************************
 * Include Libraries
 ****************************************/
#include "UbidotsEsp32Mqtt.h"
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>

/****************************************
 * Define SDA, SCL, constants and global variables
 ****************************************/
#define CHANNEL 3
#define I2C_SDA 23
#define I2C_SCL 22
LiquidCrystal_I2C  lcd_i2c(0x27, 16, 2);

const int pinBlue = 25;
const int pinRed = 27;
const int pinGreen = 26;
const int buttonAcceptPin = 33; 
const int vibrationMotor = 18;
const int changeFrequency = 20000; // Seconds to change connection between Ubidots and Esp Now

int key = 0; // Controls if the device is connect to ubidots or Esp Now
int buttonState = 0;  // Variable for reading the button (pressed or not)
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
const char *PUBLISH_VARIABLE_LABEL = ""; // Add your Ubidots variable that storages emergency calls
const char *OWNER_VARIABLE_LABEL = "";   // DEVICE'S OWNER VARIABLE
const char *TOPIC = ""; 
// Add your Ubidots variable that storages cleps as "/v2.0/devices/<device_label>/<variable_label>" Example: /v2.0/devices/clepius/emergencyvariable

String receivedValue = "0"; // Urgency int (1 - High, 2 - Medium, 3- Low, 0 - No emergency)
String receivedContexto; // Room where the emergency happened

Ubidots ubidots(UBIDOTS_TOKEN);


/****************************************
 * Main Functions
 ****************************************/

void setup()
{
  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinBlue, OUTPUT);
  pinMode(buttonAcceptPin, INPUT);
  pinMode(vibrationMotor, OUTPUT);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd_i2c.init();
  lcd_i2c.backlight();
  lcd_i2c.setCursor(0,0);

  Serial.begin(115200);

  timer = millis();
}

void loop()
{
  buttonState = digitalRead(buttonAcceptPin);

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

  // If there's an emergency, the vibration motors starts to vibrate
  if(receivedValue != "0") {
    digitalWrite(vibrationMotor, HIGH);
    delay(500);
    digitalWrite(vibrationMotor, LOW);
    delay(500);
  }
}


/****************************************
 * UBIDOTS (key == 0);
 ****************************************/

// Starts the ubidots connection and send the location to the Ubidots variable
void setupUbidots()
{
  ubidots.setDebug(true);
  connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
  ubidots.subscribe(TOPIC);

  sendLocation();
}

// Reconnect, subscribe and gets the button state to answer any emergency call
void loopUbidots()
{
  if (!ubidots.connected())
  {
    ubidots.reconnect();
    ubidots.subscribe(TOPIC);
  }
  if (buttonState)
  {
    ubidots.add(PUBLISH_VARIABLE_LABEL, 0);
    ubidots.publish(PUBLISH_DEVICE_LABEL);

    lcd_i2c.clear();
    lcd_i2c.setCursor(0, 0);
    lcd_i2c.print("Chamado");
    lcd_i2c.setCursor(0, 1);
    lcd_i2c.print("Respondido");

    delay(1000);
    lcd_i2c.clear();
  }

  ubidots.loop();
}

// Connect to wifi and, if couldn't, change the key and retry after
void connectToWifi(const char* ssid, const char* pass) {
  unsigned long startTime = millis();

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
  } else {
    key = 1;
    lcd_i2c.clear();
    lcd_i2c.print("Erro de conexão...");
  }
}

// Receives messages from Ubidots inputs, stores them in variables and issues the emergency alert
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Analisa o payload como JSON para obter o contexto
  DynamicJsonDocument jsonDoc(1024);
  deserializeJson(jsonDoc, payload);

  // Obtém o contexto (se existir)
  JsonObject context = jsonDoc["context"];
  receivedValue = jsonDoc["value"].as<String>();
  receivedContexto = String(context["sala"].as<String>());

  Serial.println(receivedValue);
  Serial.println(receivedContexto);
  Serial.println();

  emitAlert();

}

void emitAlert() {
  lcd_i2c.clear();
  lcd_i2c.setCursor(0, 0);

  // If the urgency is 1 (High), the RGB LED turns red and the message is printed on the LCD
  if (receivedValue == "1") {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinGreen, LOW);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.print("Urgencia: Alta");
  }
  // If the urgency is 2 (Normal), the RGB LED turns yellow and the message is printed on the LCD
  else if (receivedValue == "2") {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinGreen, HIGH);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.print("Urgencia: Media");
  }
  // If the urgency is 3 (Low), the RGB LED turns green and the message is printed on the LCD
  else if (receivedValue == "3") {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinGreen, HIGH);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.print("Urgencia: Baixa");
  } 
  // State 0 means there are no emergencies, the LED goes out and the display is blank
  else {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinGreen, LOW);
    digitalWrite(pinBlue, LOW);
  }

  // If there's no emergency, the display prints "Clepius". Otherwise, prints the room where the attendance needs to go
  lcd_i2c.setCursor(0, 1);
  if(receivedContexto == "null")
  {
    lcd_i2c.print("Clepius");
  }
  else
  {
    lcd_i2c.print(receivedContexto);
  }

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
 * ESP NOW (key == 1)
 ****************************************/

// Connect to the Esp Now as a Slave
void setupEspNow()
{
  Serial.println("ESPNow/Basic/Slave Example");
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // Configure device AP mode
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