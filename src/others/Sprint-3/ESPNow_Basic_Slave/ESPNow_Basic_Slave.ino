// Este código serve para recebimento de mensagens diretamente do ESP Master

#include <esp_now.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Define os pinos SDA e SCL para comunicação I2C
#define I2C_SDA 23
#define I2C_SCL 22
LiquidCrystal_I2C  lcd_i2c(0x27, 16, 2);

// Define o canal de comunicação ESP Now
#define CHANNEL 3

// Define os pinos para os LEDs RGB
const int pinBlue = 25;
const int pinRed = 27;
const int pinGreen = 26;

// Função para inicializar o ESP Now
void InitESPNow() {
  // Desconecta do WiFi para evitar conflitos
  WiFi.disconnect();
  
  // Inicializa o ESP Now e verifica se a inicialização foi bem-sucedida
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

// Função para configurar o dispositivo como um ponto de acesso (AP)
void configDeviceAP() {
  const char *SSID = "Slave_1";
  // Configura o dispositivo como ponto de acesso com um SSID e senha específicos
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    Serial.print("AP CHANNEL "); Serial.println(WiFi.channel());
  }
}

void setup() {
  // Inicializa a comunicação serial com uma taxa de 9600 bps
  Serial.begin(9600);

  // Configura os pinos como saídas para controle do LED RGB
  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinBlue, OUTPUT);

  Serial.println("ESPNow/Basic/Slave Example");

  // Configura o dispositivo como um ponto de acesso (AP)
  WiFi.mode(WIFI_AP);
  configDeviceAP();

  // Inicializa o ESP Now
  InitESPNow();

  // Registra a função de callback para receber dados
  esp_now_register_recv_cb(OnDataRecv);

  // Inicializa a comunicação I2C e o visor LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd_i2c.init();
  lcd_i2c.backlight();
  lcd_i2c.setCursor(0,0);
}

// Função de callback chamada quando dados são recebidos via ESP Now
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  // Converte o endereço MAC para uma string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: ");

  // Converte os dados recebidos em uma string
  String receivedData = "";
  for (int i = 0; i < data_len; i++) {
    receivedData += (char)data[i];
  }
  Serial.println(receivedData);
  Serial.println("");

  // Divide os dados recebidos usando a vírgula como delimitador
  int commaIndex = receivedData.indexOf(',');
  if (commaIndex != -1) {
    // Extrai as duas partes da mensagem
    String part1 = receivedData.substring(0, commaIndex);
    String part2 = receivedData.substring(commaIndex + 1);

    // Chama as funções apropriadas com os valores extraídos
    printLCD(part1, part2);
  }
}

// Função para imprimir no visor LCD com base nos dados recebidos
void printLCD(const String& message, const String& room) {
  lcd_i2c.clear();
  // Se a urgência for 1 (Alta), o LED RGB fica vermelho e a mensagem é printada no LCD
  if (message == "1") {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinGreen, LOW);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.setCursor(0, 0);
    lcd_i2c.print("Urgencia: Alta");
    lcd_i2c.setCursor(0, 1);
    lcd_i2c.print(room);

  }
  // Se a urgência for 2 (Média), o LED RGB fica amarelo e a mensagem é printada no LCD
  else if (message == "2") {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinGreen, HIGH);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.setCursor(0, 0);
    lcd_i2c.print("Urgencia: Media");
    lcd_i2c.setCursor(0, 1);
    lcd_i2c.print(room);

  }
  // Se a urgência for 3 (Baixa), o LED RGB fica verde e a mensagem é printada no LCD
  else if (message == "3") {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinGreen, HIGH);
    digitalWrite(pinBlue, LOW);
    lcd_i2c.setCursor(0, 0);
    lcd_i2c.print("Urgencia: Baixa");
    lcd_i2c.setCursor(0, 1);
    lcd_i2c.print(room);

  } 
  // O estado 0 significa que não há emergências, o LED apaga e o visor fica vazio
  else {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinGreen, LOW);
    digitalWrite(pinBlue, LOW);
  }
}

void loop() {
  
}
