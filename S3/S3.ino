#include <WiFi.h>                        // Biblioteca para conexão WiFi no ESP32
#include <WiFiClientSecure.h>            // Biblioteca para conexão segura (SSL/TLS)
#include <PubSubClient.h>                // Biblioteca MQTT para comunicação com broker
#include <ESP32Servo.h>                  // Biblioteca para controlar servos no ESP32
#include <NewPing.h>                     // Biblioteca para o sensor ultrassônico
#include "env.h"                         // Arquivo separado com credenciais e tópicos

// -------------------------
// Pinos do Sistema
// -------------------------
#define SERVO1_PIN 25                    // Pino do servo que abre/fecha o desvio
#define SERVO2_PIN 26                    // Pino do segundo servo (opcional)
#define TRIG_PIN 4                       // Pino TRIG do sensor ultrassônico
#define ECHO_PIN 2                       // Pino ECHO do sensor ultrassônico
#define RGB_R 14                         // LED RGB - Vermelho
#define RGB_G 12                         // LED RGB - Verde
#define RGB_B 27                         // LED RGB - Azul
#define DETECT_LED 33                    // LED que acende quando o trem é detectado

// -------------------------
// Sensor Ultrassônico
// -------------------------
#define MAX_DISTANCE 200                 // Distância máxima de leitura (cm)
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);  // Inicialização do sensor

// -------------------------
// Objetos Servo
// -------------------------
Servo servo1;                            // Servo responsável pelo desvio
Servo servo2;                            // Servo extra

// -------------------------
// WiFi + MQTT
// -------------------------
WiFiClientSecure client;                 // Cliente WiFi seguro (TLS)
PubSubClient mqtt(client);               // Cliente MQTT usando WiFi seguro

// Controle interno
bool esperandoTrem = false;              // Flag indicando que o trem está para passar

// -----------------------------------------------------------
// Função para acender LED RGB com a cor desejada
// -----------------------------------------------------------
void setColor(int r, int g, int b) {
  digitalWrite(RGB_R, r);               // Liga/desliga LED vermelho
  digitalWrite(RGB_G, g);               // Liga/desliga LED verde
  digitalWrite(RGB_B, b);               // Liga/desliga LED azul
}

// -----------------------------------------------------------
// Função para piscar uma cor no LED RGB
// -----------------------------------------------------------
void piscarCor(int r, int g, int b, int tempo, int vezes) {
  for (int i = 0; i < vezes; i++) {
    setColor(r, g, b);                  // Liga a cor escolhida
    delay(tempo);                       // Espera X ms
    setColor(0, 0, 0);                  // Desliga LEDs
    delay(tempo);                       // Espera novamente
  }
}

// -----------------------------------------------------------
// CALLBACK - Quando uma mensagem MQTT chega
// -----------------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {

  String msg = "";                       // Cria string vazia
  for (int i = 0; i < length; i++)       // Converte bytes → texto
    msg += (char)payload[i];

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  // -------------------------------
  // Controle do SERVO 1 (desvio)
  // -------------------------------
  if (String(topic) == TOPIC_SER1) {

    if (msg == "1" || msg == "abrir") {  // Abrir desvio via MQTT

      servo1.write(120);                 // Abre desvio (posição 120°)
      esperandoTrem = true;              // Marca que trem vai passar

      setColor(0, 0, 1);                 // LED azul = aguardando trem
      Serial.println("Servo1 → ABERTO (120°)");
    }

    else if (msg == "0" || msg == "fechar") { // Fechar desvio

      servo1.write(0);                   // Fecha desvio (posição 0°)
      esperandoTrem = false;             // Sai do modo de espera

      setColor(0, 1, 0);                 // Verde = normal
      Serial.println("Servo1 → FECHADO (0°)");
    }
  }

  // -------------------------------
  // Modo círculo
  // -------------------------------
  if (String(topic) == TOPIC_CIRCULO) {

    if (msg == "fazer_circulo") {        // Comando recebido
      setColor(1, 1, 1);                 // LED branco = modo círculo
      Serial.println("Modo: FAZER CÍRCULO");
    }
  }
}

// -----------------------------------------------------------
// Função de reconexão MQTT
// -----------------------------------------------------------
void reconnect() {

  while (!mqtt.connected()) {            // Enquanto não estiver conectado

    Serial.print("Tentando conexão MQTT... ");

    if (mqtt.connect("ESP32_S3_No3", BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");
      piscarCor(0, 1, 0, 200, 2);        // Pisca verde = OK

      mqtt.subscribe(TOPIC_SER1);        // Inscreve no tópico do servo
      mqtt.subscribe(TOPIC_CIRCULO);     // Inscreve no modo círculo

    } else {
      Serial.print("Falhou, rc=");
      Serial.println(mqtt.state());      // Mostra o erro do MQTT
      piscarCor(1, 1, 0, 300, 1);        // Pisca amarelo = erro
      delay(3000);                       // Espera 3s para tentar de novo
    }
  }
}

// -----------------------------------------------------------
// SETUP - Executa uma vez após ligar
// -----------------------------------------------------------
void setup() {
  Serial.begin(115200);                  // Inicia Serial

  // Configura LEDs RGB e LED detector
  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);
  pinMode(DETECT_LED, OUTPUT);
  digitalWrite(DETECT_LED, LOW);

  setColor(1, 1, 0);                     // Amarelo = iniciando

  // Inicializa servos
  servo1.attach(SERVO1_PIN, 500, 2400);  // Sinal PWM servo
  servo2.attach(SERVO2_PIN, 500, 2400);
  servo1.write(0);                       // Posição inicial
  servo2.write(0);

  client.setInsecure();                  // Desabilita checagem SSL (necessário HiveMQ)
  WiFi.begin(WIFI_SSID, WIFI_PASS);      // Inicia WiFi
  Serial.println("Conectando ao WiFi...");

  // Aguarda conexão WiFi
  while (WiFi.status() != WL_CONNECTED) {
    piscarCor(1, 1, 0, 300, 1);           // Pisca amarelo enquanto tenta conectar
  }

  Serial.println("WiFi conectado!");

  mqtt.setServer(BROKER_URL, BROKER_PORT);  // Configura broker MQTT
  mqtt.setCallback(callback);                // Define função callback

  reconnect();                               // Conecta ao MQTT

  setColor(0, 1, 0);                          // Verde = pronto
}

// -----------------------------------------------------------
// LOOP - Executa repetidamente
// -----------------------------------------------------------
void loop() {

  if (!mqtt.connected()) reconnect();     // Garante que MQTT está conectado
  mqtt.loop();                            // Mantém MQTT funcionando

  // --------------------------------------
  // MODO ESPERANDO O TREM PASSAR
  // --------------------------------------
  if (esperandoTrem) {

    int dist = sonar.ping_cm();           // Mede distância do trem

    if (dist > 0 && dist < 50) {          // Trem detectado a menos de 50cm

      Serial.println("TREM DETECTADO!");

      digitalWrite(DETECT_LED, HIGH);     // Acende LED de detecção

      servo1.write(0);                    // Fecha desvio automaticamente
      esperandoTrem = false;              // Sai do modo de espera

      mqtt.publish(TOPIC_TREM_PASSOU, "1");  // Notifica via MQTT

      setColor(0, 1, 0);                  // Verde = normal

      delay(1000);                        // Espera 1s
      digitalWrite(DETECT_LED, LOW);      // Apaga LED
    }

    return;                                // Mantém azul até trem ser detectado
  }

  // MODO NORMAL
  setColor(0, 1, 0);                        // LED verde

  delay(100);                               // Delay básico
}
