#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

// Pinos do driver L9110
#define PIN_FRENTE 25
#define PIN_TRAS   26

// Pinos do LED RGB
#define LED_R 14
#define LED_G 12
#define LED_B 13

WiFiClientSecure client;
PubSubClient mqtt(client);

void setColor(int r, int g, int b) {
  digitalWrite(LED_R, r);
  digitalWrite(LED_G, g);
  digitalWrite(LED_B, b);
}

void piscarCor(int r, int g, int b, int tempo, int repeticoes) {
  for (int i = 0; i < repeticoes; i++) {
    setColor(r, g, b);
    delay(tempo);
    setColor(0, 0, 0);
    delay(tempo);
  }
}

void tremFrente() {
  digitalWrite(PIN_FRENTE, HIGH);
  digitalWrite(PIN_TRAS, LOW);
}

void tremTras() {
  digitalWrite(PIN_FRENTE, LOW);
  digitalWrite(PIN_TRAS, HIGH);
}

void pararTrem() {
  digitalWrite(PIN_FRENTE, LOW);
  digitalWrite(PIN_TRAS, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.println("Mensagem recebida: " + msg);

  if (msg == "Trem_Frente") {
    tremFrente();
  } 
  else if (msg == "Trem_Tras") {
    tremTras();
  }
  else if (msg == "Trem_Parar") {
    pararTrem();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_FRENTE, OUTPUT);
  pinMode(PIN_TRAS, OUTPUT);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  client.setInsecure();
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Piscando LARANJA enquanto conecta
  while (WiFi.status() != WL_CONNECTED) {
    piscarCor(1, 1, 0, 300, 1); // Laranja
  }

  Serial.println("WiFi Conectado!");

  mqtt.setServer(BROKER_URL, BROKER_PORT);
  mqtt.setCallback(callback);

  Serial.println("Conectando ao Broker...");

  // Piscando LARANJA tentando conectar
  while (!mqtt.connected()) {
    piscarCor(1, 1, 0, 300, 1);
    mqtt.connect("Trem_ESP32", BROKER_USER, BROKER_PASS);
  }

  Serial.println("Broker Conectado!");

  piscarCor(0, 1, 0, 200, 3); // Pisca verde 3x quando conecta
  mqtt.subscribe(TOPIC_ILUM);
  setColor(0,1,0); // Verde fixo (parado)
}

void loop() {
  mqtt.loop();

  // Se motor estiver ativo (andando), pisca vermelho
  if (digitalRead(PIN_FRENTE) || digitalRead(PIN_TRAS)) {
    piscarCor(1, 0, 0, 300, 1); // Vermelho alerta
  } else {
    setColor(0, 1, 0); // Verde fixo (parado)
  }

  delay(100);
}
