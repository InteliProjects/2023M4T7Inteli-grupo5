
// Esse código recebe um input enviado do Ubidots e liga um LED e printa uma mensagem no visor LCD baseado na mensagem recebida 

/******************************************
 *
 * This example works for both Industrial and STEM users.
 *
 * Developed by Jose Garcia, https://github.com/jotathebest/
 *
 * ****************************************/

/****************************************
 * Incluir bibliotecas
 ****************************************/
#include "UbidotsEsp32Mqtt.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define I2C_SDA 23
#define I2C_SCL 22
LiquidCrystal_I2C  lcd_i2c(0x27, 16, 2);

/****************************************
 * Definir constantes
 ****************************************/
const char *UBIDOTS_TOKEN = "BBUS-k79VIHwXIgv1c8Tn4JQXSHxlsS0AMB";
const char *WIFI_SSID = "Inteli-welcome";
const char *WIFI_PASS = "";
const char *DEVICE_LABEL = "clepinho";   // Nome do dispositivo do Ubidots
const char *VARIABLE_LABEL = "varChamado"; // Variável que está se subscrevendo

const int pinBlue = 25;
const int pinRed = 27;
const int pinGreen = 26;

Ubidots ubidots(UBIDOTS_TOKEN);

/****************************************
 * Auxiliar Functions
 ****************************************/

// Função de retorno de chamada executada quando uma mensagem é recebida do Ubidots
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    lcd_i2c.clear();
    // Se a urgência for 1 (Alta), o LED RGB fica vermelho e a mensagem é printada no LCD
    if ((char)payload[0] == '1') {
      digitalWrite(pinRed, HIGH);
      digitalWrite(pinGreen, LOW);
      digitalWrite(pinBlue, LOW);
      lcd_i2c.print("Urgencia: Alta");
    }
    // Se a urgência for 2 (Média), o LED RGB fica amarelo e a mensagem é printada no LCD
    else if ((char)payload[0] == '2') {
      digitalWrite(pinRed, HIGH);
      digitalWrite(pinGreen, HIGH);
      digitalWrite(pinBlue, LOW);
      lcd_i2c.print("Urgencia: Media");
    }
    // Se a urgência for 3 (Baixa), o LED RGB fica verde e a mensagem é printada no LCD
    else if ((char)payload[0] == '3') {
      digitalWrite(pinRed, LOW);
      digitalWrite(pinGreen, HIGH);
      digitalWrite(pinBlue, LOW);
      lcd_i2c.print("Urgencia: Baixa");
    }
    // O estado 0 significa que não há emergências, o LED apaga e o visor fica vazio
    else if ((char)payload[0] == '0') {
      digitalWrite(pinRed, LOW);
      digitalWrite(pinGreen, LOW);
      digitalWrite(pinBlue, LOW);
    }
  }
  Serial.println();
}

/****************************************
 * Funções Principais
 ****************************************/

void setup()
{
  // Inicializa a comunicação serial com uma taxa de 115200 bps
  Serial.begin(115200);
  
  // Configura os pinos como saídas para controle do LED RGB
  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinBlue, OUTPUT);

  // Define as portas SDA e SCL e inicia o visor LCD via I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd_i2c.init();
  lcd_i2c.backlight();
  lcd_i2c.setCursor(0,0);

  // Inicia a conexão com a rede Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);

  // Configuração do cliente Ubidots
  Serial.println("Setting up Ubidots...");
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();

  // Subscreve para receber as atualizações da variável no Ubidots
  Serial.println("Subscribing to Ubidots variable...");
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL);

  // Indica que a configuração inicial foi concluída
  Serial.println("Setup complete!");
}

void loop()
{
  // Verifica se a conexão com Ubidots está ativa
  if (!ubidots.connected())
  {
    // Se não estiver conectado, tenta reconectar
    Serial.println("Reconnecting to Ubidots...");
    ubidots.reconnect();
    ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL);
  }

  // Executa as operações necessárias no cliente Ubidots
  ubidots.loop();

  // Imprime uma mensagem indicando que está no loop
  Serial.println("Looping...");

  // Aguarda 1 segundo antes de repetir o loop
  delay(1000);
}
