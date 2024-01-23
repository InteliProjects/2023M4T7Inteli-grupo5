
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
 * Define SDA, SCL and RGB Led
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
const int changeFrequency = 20000; // Update rate in milliseconds

int key = 0; // control if the device is connect to ubidots or espNow
int buttonState = 0;  // variable for reading the pushbutton status
int controlSetup = 0;
char macStr[18];
char receivedData[20];  // Receives the location 

unsigned long timer;

/****************************************
 * Define Constants
 ****************************************/
const char *UBIDOTS_TOKEN = "BBUS-k79VIHwXIgv1c8Tn4JQXSHxlsS0AMB"; // Add your Ubidots Token
const char *WIFI_SSID = "Inteli-welcome"; // Add your Wi-Fi SSID
const char *WIFI_PASS = ""; // Add your Wi-Fi password
const char *PUBLISH_DEVICE_LABEL = "clepinho"; // Add your Ubidots Device
const char *PUBLISH_VARIABLE_LABEL = "varChamado"; // Add your Ubidots variable that storages cleps
const char *PUBLISH_OWNER_VARIABLE_LABEL = "Renan";   // DEVICE'S OWNER VARIABLE
const char *TOPIC = "/v2.0/devices/clepinho/varchamado"; // Add your Ubidots variable that storages cleps as "/v2.0/devices/<device_label>/<variable_label>"

String receivedValue = "0";
String receivedContexto;

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

  // put your setup code here, to run once:
  Serial.begin(115200);

  timer = millis();
}

void loop()
{
  buttonState = digitalRead(buttonAcceptPin);

  if(key == 0)
  {
    if(controlSetup == 0)
    {
      setupUbidots();
      controlSetup = 1;
    }

    loopUbidots();
  }

  else if(key == 1)
  {
    if(controlSetup == 0)
    {
      setupEspNow();
      controlSetup = 1;
    }
  }
  
  if (millis() - timer > changeFrequency && key == 0) // triggers the routine every 5 seconds
  {
    ubidots.disconnect();
    key = 1;
    controlSetup = 0;
    timer = millis();
  }

  
  if(receivedValue != "0") {
    digitalWrite(vibrationMotor, HIGH);
    delay(500);
    digitalWrite(vibrationMotor, LOW);
    delay(500);
  }
}


/****************************************
 * UBIDOTS (key = 0);
 ****************************************/

void setupUbidots()
{
  ubidots.setDebug(true);  // uncomment this to make debug messages available
  connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
  ubidots.subscribe(TOPIC);

  sendLocation();
}

void loopUbidots()
{
  if (!ubidots.connected())
  {
    ubidots.reconnect();
    ubidots.subscribe(TOPIC);
  }
  if (buttonState) // triggers the routine every 5 seconds
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

void connectToWifi(const char* ssid, const char* pass) {
  unsigned long startTime = millis();  // Obtém o tempo inicial

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

  // Se a urgência for 1 (Alta), o LED RGB fica vermelho e a mensagem é printada no LCD
  if (receivedValue == "1") {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinGreen, LOW);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.print("Urgencia: Alta");
  }
  // Se a urgência for 2 (Média), o LED RGB fica amarelo e a mensagem é printada no LCD
  else if (receivedValue == "2") {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinGreen, HIGH);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.print("Urgencia: Media");
  }
  // Se a urgência for 3 (Baixa), o LED RGB fica verde e a mensagem é printada no LCD
  else if (receivedValue == "3") {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinGreen, HIGH);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.print("Urgencia: Baixa");
  } 
  // O estado 0 significa que não há emergências, o LED apaga e o visor fica vazio
  else {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinGreen, LOW);
    digitalWrite(pinBlue, LOW);
  }

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

void sendLocation()
{
  char* context = (char*)malloc(sizeof(char) * 60);

  ubidots.addContext("id", macStr);
  ubidots.addContext("sala", receivedData);
  ubidots.getContext(context);
  free(context);
  ubidots.add(PUBLISH_OWNER_VARIABLE_LABEL, 0, context);
  ubidots.publish(PUBLISH_DEVICE_LABEL);
}



/****************************************
 * ESP NOW (key = 1)
 ****************************************/

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
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
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

// config AP SSID
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