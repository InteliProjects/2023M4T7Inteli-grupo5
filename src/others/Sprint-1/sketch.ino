#include <Arduino.h>

// Definição da classe Chamado
class Chamado {
public:
  Chamado(int id) : chamadoID(id), status(true) {}

  // Método para retornar o ID da instância
  int getID() {
    return chamadoID;
  }

  // Método para retornar o status da instância
  // (true = Alerta ativo; false = Alerta desativado)
  bool getStatus() {
    return status;
  }

  // Método para alterar o status da instância
  void setStatus(bool novoStatus) {
    status = novoStatus;
  }

private:
  int chamadoID;
  bool status;
};

const int ledPinChamado = 13; // Pino do LED do chamado
const int ledPinLoc = 11;     // Pino do LED de localização
const int buttonPin = 12;     // Pino do botão

Chamado chamado(0); // Instância da classe Chamado
unsigned long previousMillisChamado = 0; // Variável para rastrear o tempo para o LED de chamado
unsigned long previousMillisLoc = 0;     // Variável para rastrear o tempo para o LED de localização
const long intervalChamado = 500; // Intervalo para piscar o LED do chamado (500 ms)
const long intervalLoc = 2000;    // Intervalo para piscar o LED de localização (2000 ms)
bool ledStateChamado = LOW; // Estado atual do LED de chamado (inicializado como desligado)
bool ledStateLoc = LOW;     // Estado atual do LED de localização (inicializado como desligado)

void setup() {
  pinMode(ledPinChamado, OUTPUT); // Configura o pino do LED do chamado como saída
  pinMode(ledPinLoc, OUTPUT);     // Configura o pino do LED de localização como saída
  pinMode(buttonPin, INPUT);      // Configura o pino do botão como entrada
  Serial.begin(9600);            // Inicializa a comunicação serial
  Serial.print("Novo Chamado - ID: ");
  Serial.print(chamado.getID()); // Imprime o ID do novo chamado
  Serial.print(", Status: ");
  Serial.print(chamado.getStatus()); // Imprime o status do novo chamado
  Serial.println(", Obtendo localização...."); // Imprime mensagem de obtenção de localização
}

void verificarBotao() {
  if (digitalRead(buttonPin) == HIGH) {
    chamado.setStatus(false);        // Define o status do chamado como falso
    digitalWrite(ledPinChamado, LOW); // Desliga o LED de chamado
    Serial.println("Chamado Respondido"); // Imprime mensagem de chamado respondido
    Serial.println("Digite um ID para abrir um novo chamado"); // Solicita um novo ID
  }
}

void emitirAlerta() {
  bool status = chamado.getStatus(); // Obtém o status do chamado
  unsigned long currentMillisChamado = millis(); // Obtém o tempo atual em milissegundos
  
  if (currentMillisChamado - previousMillisChamado >= intervalChamado) {
    previousMillisChamado = currentMillisChamado; // Atualiza o tempo anterior
    ledStateChamado = !ledStateChamado; // Inverte o estado do LED de chamado
    digitalWrite(ledPinChamado, ledStateChamado); // Atualiza o LED de chamado
  }
}

void obterLocalizacao() {
  bool status = chamado.getStatus(); // Obtém o status do chamado
  unsigned long currentMillisLoc = millis(); // Obtém o tempo atual em milissegundos

  if (currentMillisLoc - previousMillisLoc >= intervalLoc) {
    previousMillisLoc = currentMillisLoc; // Atualiza o tempo anterior
    ledStateLoc = !ledStateLoc; // Inverte o estado do LED de localização
    digitalWrite(ledPinLoc, ledStateLoc); // Atualiza o LED de localização
    if (status){
      Serial.println("Localização Obtida"); // Imprime mensagem de localização obtida
    }
  }
}

void loop() {
  bool status = chamado.getStatus(); // Obtém o status do chamado

  if (status) {
    verificarBotao(); // Verifica se o botão foi pressionado
    emitirAlerta();   // Emite alerta piscando o LED de chamado
  }

  obterLocalizacao(); // Obtém a localização piscando o LED de localização

  if (Serial.available() > 0) {
    // Lê dados da porta serial
    String input = Serial.readStringUntil('\n');
    int id = input.substring(0, input.indexOf(',')).toInt();
    Chamado novoChamado(id); // Cria um novo chamado com o ID fornecido
    chamado = novoChamado;   // Atualiza o chamado atual com o novo chamado
    Serial.print("Novo Chamado - ID: ");
    Serial.print(chamado.getID()); // Imprime o ID do novo chamado
    Serial.print(", Status: ");
    Serial.print(chamado.getStatus()); // Imprime o status do novo chamado
    Serial.println(", Obtendo localização...."); // Imprime mensagem de obtenção de localização
  }
}