#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

// ======= Pinos do driver L9110 (motor do trem) =======
#define PIN_FRENTE 25
#define PIN_TRAS   26

// ======= Pinos do LED RGB =======
#define LED_R 14
#define LED_G 12
#define LED_B 13

WiFiClientSecure client;  // Cliente seguro (HTTPS/TLS)
PubSubClient mqtt(client); // Cliente MQTT usando conexão segura

// Função para definir a cor do LED RGB
void setColor(int r, int g, int b) {
  digitalWrite(LED_R, r);
  digitalWrite(LED_G, g);
  digitalWrite(LED_B, b);
}

// Função para piscar uma cor por X tempo, Y vezes
void piscarCor(int r, int g, int b, int tempo, int repeticoes) {
  for (int i = 0; i < repeticoes; i++) {
    setColor(r, g, b);   // Liga cor
    delay(tempo);
    setColor(0, 0, 0);   // Apaga LED
    delay(tempo);
  }
}

// Liga motor para frente
void tremFrente() {
  digitalWrite(PIN_FRENTE, HIGH);
  digitalWrite(PIN_TRAS, LOW);
}

// Liga motor para trás
void tremTras() {
  digitalWrite(PIN_FRENTE, LOW);
  digitalWrite(PIN_TRAS, HIGH);
}

// Desliga motor
void pararTrem() {
  digitalWrite(PIN_FRENTE, LOW);
  digitalWrite(PIN_TRAS, LOW);
}

// Publica mensagem de status no MQTT
void publicarStatus(const char* status) {
  mqtt.publish(TOPIC_STATUS, status);
  Serial.println(String("Status enviado: ") + status);
}

// Callback: executa quando chega mensagem no MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;

  // Converte bytes recebidos para string
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.println("Mensagem recebida: " + msg);

  // Interpreta comandos recebidos
  if (msg == "Trem_Frente") {
    tremFrente();
    publicarStatus("ANDANDO");
  } 
  else if (msg == "Trem_Tras") {
    tremTras();
    publicarStatus("ANDANDO");
  }
  else if (msg == "Trem_Parar") {
    pararTrem();
    publicarStatus("PARADO");
  }
}

void setup() {
  Serial.begin(115200);

  // Configura pinos como saída
  pinMode(PIN_FRENTE, OUTPUT);
  pinMode(PIN_TRAS, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // Conexão WiFi sem validação de certificado
  client.setInsecure();

  Serial.println("Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Pisca laranja enquanto tenta conectar ao WiFi
  while (WiFi.status() != WL_CONNECTED) {
    piscarCor(1, 1, 0, 300, 1);
  }

  Serial.println("WiFi Conectado!");

  // Configura servidor MQTT e callback
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  mqtt.setCallback(callback);

  Serial.println("Conectando ao Broker...");

  // Pisca laranja até conectar ao broker
  while (!mqtt.connected()) {
    piscarCor(1, 1, 0, 300, 1);
    mqtt.connect("Trem_ESP32", BROKER_USER, BROKER_PASS);
  }

  Serial.println("Broker Conectado!");

  // Pisca verde 3x quando conecta
  piscarCor(0, 1, 0, 200, 3);

  // Assina o tópico de comandos
  mqtt.subscribe(TOPIC_TREM);

  // LED verde indica trem parado
  setColor(0,1,0);
  
  publicarStatus("PARADO");
}

void loop() {
  mqtt.loop(); // Mantém conexão MQTT ativa

  // Se algum dos pinos estiver ligado, o trem está andando
  if (digitalRead(PIN_FRENTE) || digitalRead(PIN_TRAS)) {
    piscarCor(1, 0, 0, 300, 1); // Pisca vermelho indicando movimento
  } 
  else {
    setColor(0, 1, 0); // Verde se estiver parado
  }

  delay(100);
}
