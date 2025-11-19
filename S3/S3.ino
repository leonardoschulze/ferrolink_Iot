#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <NewPing.h>
#include "env.h"

// -------------------------
// Pinos
// -------------------------
#define SERVO1_PIN 25
#define SERVO2_PIN 26
#define TRIG_PIN 4
#define ECHO_PIN 2
#define RGB_R 14
#define RGB_G 12
#define RGB_B 27
#define DETECT_LED 33   // LED indicando trem detectado

// Sensor ultrassônico
#define MAX_DISTANCE 200
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// Objetos servo
Servo servo1;
Servo servo2;

// WiFi + MQTT
WiFiClientSecure client;
PubSubClient mqtt(client);

// Controle interno
bool esperandoTrem = false;

// -------------------------
// Funções auxiliares
// -------------------------
void setColor(int r, int g, int b) {
  digitalWrite(RGB_R, r);
  digitalWrite(RGB_G, g);
  digitalWrite(RGB_B, b);
}

void piscarCor(int r, int g, int b, int tempo, int vezes) {
  for (int i = 0; i < vezes; i++) {
    setColor(r, g, b);
    delay(tempo);
    setColor(0, 0, 0);
    delay(tempo);
  }
}

// -------------------------
// MQTT - Callback
// -------------------------
void callback(char* topic, byte* payload, unsigned int length) {

  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  // ===========================
  // → Abrir / Fechar desvio (Servo1)
  // ===========================
  if (String(topic) == TOPIC_SER1) {

    if (msg == "1" || msg == "abrir") {

      servo1.write(120);  // abre desvio
      esperandoTrem = true;

      setColor(0, 0, 1);  // azul = aguardando trem
      Serial.println("Servo1 → ABERTO (120°)");
    }

    else if (msg == "0" || msg == "fechar") {

      servo1.write(0);    // fecha desvio
      esperandoTrem = false;

      setColor(0, 1, 0); // verde = normal
      Serial.println("Servo1 → FECHADO (0°)");
    }
  }

  // ===========================
  // → Modo círculo
  // ===========================
  if (String(topic) == TOPIC_CIRCULO) {

    if (msg == "fazer_circulo") {
      setColor(1, 1, 1); // branco
      Serial.println("Modo: FAZER CÍRCULO");
    }
  }
}

// -------------------------
// Reconexão MQTT
// -------------------------
void reconnect() {

  while (!mqtt.connected()) {

    Serial.print("Tentando conexão MQTT... ");

    if (mqtt.connect("ESP32_S3_No3", BROKER_USER, BROKER_PASS)) {

      Serial.println("Conectado!");
      piscarCor(0, 1, 0, 200, 2); // verde

      mqtt.subscribe(TOPIC_SER1);
      mqtt.subscribe(TOPIC_CIRCULO);

    } else {
      Serial.print("Falhou, rc=");
      Serial.println(mqtt.state());
      piscarCor(1, 1, 0, 300, 1); // laranja
      delay(3000);
    }
  }
}

// -------------------------
// SETUP
// -------------------------
void setup() {
  Serial.begin(115200);

  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);

  pinMode(DETECT_LED, OUTPUT);
  digitalWrite(DETECT_LED, LOW);

  setColor(1, 1, 0); // amarelo = iniciando

  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN, 500, 2400);
  servo1.write(0);
  servo2.write(0);

  client.setInsecure();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Conectando ao WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    piscarCor(1, 1, 0, 300, 1);
  }

  Serial.println("WiFi conectado!");

  mqtt.setServer(BROKER_URL, BROKER_PORT);
  mqtt.setCallback(callback);

  reconnect();

  setColor(0, 1, 0); // verde = pronto
}

// -------------------------
// LOOP
// -------------------------
void loop() {

  if (!mqtt.connected()) reconnect();
  mqtt.loop();

  // ===============================
  // MODO: Aguardando trem passar
  // ===============================
  if (esperandoTrem) {

    int dist = sonar.ping_cm();

    if (dist > 0 && dist < 50) {

      Serial.println("TREM DETECTADO!");

      digitalWrite(DETECT_LED, HIGH);

      servo1.write(0);       // Fecha desvio
      esperandoTrem = false;

      mqtt.publish(TOPIC_TREM_PASSOU, "1");

      setColor(0, 1, 0);     // volta ao verde

      delay(1000);
      digitalWrite(DETECT_LED, LOW);
    }

    return; // mantém LED azul enquanto espera trem
  }

  // Modo normal
  setColor(0, 1, 0);

  delay(100);
}