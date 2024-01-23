#include <esp_now.h>
#include <WiFi.h>

// Canal de comunicação entre os ESP32
#define CHANNEL 1

const int pinBlue = 25;
const int pinRed = 27;
const int pinGreen = 26;

// Inicializa o ESP Now com fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("Inicialização do ESPNow bem-sucedida");
  }
  else {
    Serial.println("Falha na inicialização do ESPNow");
    ESP.restart();
  }
}

// Configurações do ponto de acesso (AP)
void configDeviceAP() {
  const char *SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "Senha_Slave_1", CHANNEL, 0);
  if (!result) {
    Serial.println("Falha na configuração do AP.");
  } else {
    Serial.println("Configuração do AP bem-sucedida. Transmitindo com AP: " + String(SSID));
    Serial.print("Canal do AP: "); Serial.println(WiFi.channel());
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinBlue, OUTPUT);

  Serial.println("Exemplo ESPNow/Basic/Slave");
  WiFi.mode(WIFI_AP);
  configDeviceAP();
  Serial.print("MAC do AP: "); Serial.println(WiFi.softAPmacAddress());
  InitESPNow();
  esp_now_register_recv_cb(OnDataRecv);
}

// Função de retorno de chamada executada quando os dados são recebidos
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  // Converte o endereço MAC do remetente para uma string legível
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  Serial.print("Último pacote recebido de: "); 
  Serial.println(macStr);

  // Converte os dados recebidos em uma string para facilitar a manipulação
  String receivedData = "";
  for (int i = 0; i < data_len; i++) {
    receivedData += (char)data[i];
  }

  // Imprime no console os dados recebidos em formato de string
  Serial.print("Dados do último pacote recebido: ");
  Serial.println(receivedData);
  Serial.println();

  // Chama uma função para determinar a cor do LED com base nos dados recebidos
  determineLEDColour(receivedData);
}


// Função para determinar a cor do LED com base na mensagem recebida
void determineLEDColour(const String& message) {
  // message 1 representa urgência alta (Cor vermelha) 
  if (message == "1") {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinGreen, LOW);
    digitalWrite(pinBlue, LOW);

  } 
  // message 2 representa urgência média (Cor amrela) 
  else if (message == "2") {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinGreen, HIGH);
    digitalWrite(pinBlue, LOW);

  } 
  // message 3 representa urgência baixa (Cor verde) 
  else if (message == "3") {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinGreen, HIGH);
    digitalWrite(pinBlue, LOW);
  } 
  // Caso enviada uma mensagem que não seja um código pré-definido de emergência, os LEDs apagam
  else {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinGreen, LOW);
    digitalWrite(pinBlue, LOW);
  }
}

void loop() {
  // Não há nada necessário para a função loop neste protótipo
}
