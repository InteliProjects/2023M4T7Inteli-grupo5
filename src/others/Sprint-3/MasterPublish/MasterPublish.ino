// Esse código envia uma mensagem para o Basic Slave, recebe o endereço MAC do ESP conectado à ele e envia ao Ubidots, atualizando sua localização

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "UbidotsEsp32Mqtt.h"

// Estrutura para armazenar informações sobre o dispositivo escravo
esp_now_peer_info_t slave;

// Número do canal para comunicação ESPNow
#define CHANNEL 3

// Flag para imprimir resultados da varredura
#define PRINTSCANRESULTS 0

// Flag para deletar dispositivos antes de parear
#define DELETEBEFOREPAIR 0

// Pino indicador se um dispositivo escravo foi encontrado
const int pinFoundSlave = 17;

// Mensagem a ser enviada para o dispositivo escravo
char message[] = "3, Sala 5";
char macStr[18];
String BSSIDstr;
int key = 0;

// Ubidots configurações
const char *UBIDOTS_TOKEN = "BBUS-k79VIHwXIgv1c8Tn4JQXSHxlsS0AMB";
const char *WIFI_SSID = "Inteli-welcome";
const char *WIFI_PASS = "";
const char *DEVICE_LABEL = "clepinho";
const char *VARIABLE_LABEL = "Renan";

char* character = (char*)malloc(sizeof(char) * 10);

// Taxa de atualização em milissegundos para o envio de dados
const int PUBLISH_FREQUENCY = 5000;

unsigned long timer;

// Pino usado para ler dados do GPIO34 ADC_CH6.
uint8_t analogPin = 34;

Ubidots ubidots(UBIDOTS_TOKEN);

// Callback chamada quando uma mensagem MQTT é recebida
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Função para inicializar o ESPNow
void InitESPNow() {
  Serial.println("Fui chamada");
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    key = 2;
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

// Função para escanear por dispositivos escravos
void ScanForSlave() {
  int16_t scanResults = WiFi.scanNetworks(false, false, false, 300, CHANNEL);
  bool slaveFound = false;
  memset(&slave, 0, sizeof(slave));
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      if (PRINTSCANRESULTS) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(SSID);
        Serial.print(" (");
        Serial.print(RSSI);
        Serial.print(")");
        Serial.println("");
      }
      delay(10);
      // Verifica se o SSID começa com "Slave"
      if (SSID.indexOf("Slave") == 0) {
        Serial.println("Found a Slave.");
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Extrai o endereço MAC do BSSID
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            slave.peer_addr[ii] = (uint8_t) mac[ii];
          }
        }
        // Configura as informações do escravo
        slave.channel = CHANNEL;
        slave.encrypt = 0;
        slaveFound = true;
        break;
      }
    }
  }
  if (slaveFound) {
    Serial.println("Slave Found, processing..");
    digitalWrite(pinFoundSlave, HIGH);
  } else {
    Serial.println("Slave Not Found, trying again.");
    digitalWrite(pinFoundSlave, LOW);
  }
  WiFi.scanDelete();
}

// Função para gerenciar o dispositivo escravo
bool manageSlave() {
  if (slave.channel == CHANNEL) {
    if (DELETEBEFOREPAIR) {
      esp_now_del_peer(slave.peer_addr);
    }
    Serial.print("Slave Status: ");
    bool exists = esp_now_is_peer_exist(slave.peer_addr);
    if (exists) {
      Serial.println("Already Paired");
      return true;
    } else {
      // Adiciona o escravo à lista de pares
      esp_err_t addStatus = esp_now_add_peer(&slave);
      if (addStatus == ESP_OK) {
        Serial.println("Pair success");
        return true;
      } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
        Serial.println("ESPNOW Not Init");
        return false;
      } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
        Serial.println("Invalid Argument");
        return false;
      } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
        Serial.println("Peer list full");
        return false;
      } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
        Serial.println("Out of memory");
        return false;
      } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
        Serial.println("Peer Exists");
        return true;
      } else {
        Serial.println("Not sure what happened");
        return false;
      }
    }
  } else {
    Serial.println("No Slave found to process");
    return false;
  }
}

// Função para deletar um par de dispositivos
void deletePeer() {
  esp_err_t delStatus = esp_now_del_peer(slave.peer_addr);
  Serial.print("Slave Delete Status: ");
  if (delStatus == ESP_OK) {
    Serial.println("Success");
  } else if (delStatus == ESP_ERR_ESPNOW_NOT_INIT) {
    Serial.println("ESPNOW Not Init");
  } else if (delStatus == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (delStatus == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else {
    Serial.println("Not sure what happened");
  }
}

// Função para enviar dados para o dispositivo escravo
void sendData() {
  const uint8_t *peer_addr = slave.peer_addr;
  Serial.print("Sending: "); Serial.println(message);
  esp_err_t result = esp_now_send(peer_addr, (uint8_t*)message, strlen(message) + 1);
  Serial.print("Send Status: ");
  if (result == ESP_OK) {
    Serial.println("Success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    Serial.println("ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else {
    Serial.println("Not sure what happened");
  }
}

// Callback chamada quando os dados são enviados com sucesso
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  Serial.print("Sent Message: "); Serial.println(message);
  esp_now_deinit(); // Desliga o ESPNow
  WiFi.disconnect();
  key = 1;
}

// Configuração inicial do programa
void setup() {
  Serial.begin(9600);
  timer = millis();
  pinMode(pinFoundSlave, OUTPUT);
}

// Configuração inicial do MQTT
void setupMqtt(){
  ubidots.setDebug(true);
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
}

// Configuração inicial do ESPNow
void setupEspNow() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  Serial.println("ESPNow/Basic/Master Example");
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("STA CHANNEL "); Serial.println(WiFi.channel());
  InitESPNow();
  esp_now_register_send_cb(OnDataSent);
}

// Loop principal do programa
void loop() {
  if (key == 1){
    setupMqtt();
    if (abs(static_cast<long>(millis() - timer)) > PUBLISH_FREQUENCY){
      char macStr[18];
      sprintf(macStr, "%02x:%02x:%02x:%02x:%02x:%02x",
              slave.peer_addr[0], slave.peer_addr[1], slave.peer_addr[2],
              slave.peer_addr[3], slave.peer_addr[4], slave.peer_addr[5]);
      char* context = (char*)malloc(sizeof(char) * 30);
      
      // Adiciona o contexto da variável em formato JSON (chave, valor)  
      ubidots.addContext("id", macStr);
      ubidots.addContext("sala","Sala 5");
      ubidots.getContext(context);

      // Adiciona à variável um valor númerico e o contexto. Em seguida, dá publish no Ubidots
      ubidots.add(VARIABLE_LABEL, 0, context);
      ubidots.publish(DEVICE_LABEL);
      timer = millis();
      ubidots.disconnect();
      key = 0;
    }
  }
  else if (key == 0){
    ubidots.disconnect();
    setupEspNow();
    ScanForSlave();
  }
  if (Serial.available() > 0) {
    // Lê dados da porta serial
    String input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
        // Converte String para array de caracteres
        input.toCharArray(message, input.length() + 1);
    }
  }
  if (slave.channel == CHANNEL) {
    bool isPaired = manageSlave();
    if (isPaired) {
      sendData();
    } else {
      Serial.println("Slave pair failed!");
    }
  } else {
    Serial.println("No slave found to process");
  }
  ubidots.loop();
  delay(3000);
}