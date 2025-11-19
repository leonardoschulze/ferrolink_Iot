#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "env.h"

// ======= Pinos =======
#define DHTPIN      4
#define DHTTYPE     DHT11
#define TRIG_PIN    22
#define ECHO_PIN    23
#define LED_TREM    19
#define LDR_PIN     34
#define LED_R       14
#define LED_G       26
#define LED_B       25

// ======= Objetos =======
WiFiClientSecure client;
PubSubClient mqtt(client);
DHT dht(DHTPIN, DHTTYPE);

// ======= Funções Auxiliares =======
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

float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  if (duracao == 0) return -1; // sem retorno
  float distancia = duracao * 0.034 / 2.0; // cm
  return distancia;
}

void reconnectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Tentando conectar ao broker... ");
    if (mqtt.connect("S1_ESP32", BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");
      piscarCor(0, 1, 0, 200, 3); // Verde 3x
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Tentando novamente em 5s...");
      piscarCor(1, 1, 0, 300, 1); // Laranja piscando
      delay(5000);
    }
  }
}

// ======= Setup =======
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_TREM, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  dht.begin();

  client.setInsecure();

  Serial.println("Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    piscarCor(1, 1, 0, 300, 1); // Laranja piscando
  }

  Serial.println("\nWiFi Conectado!");
  Serial.print("IP Local: ");
  Serial.println(WiFi.localIP());

  mqtt.setServer(BROKER_URL, BROKER_PORT);
  reconnectMQTT();
}

// ======= Loop =======
void loop() {
  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  mqtt.loop();

  // === DHT11 ===
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();

  if (!isnan(temperatura) && !isnan(umidade)) {
    char tempStr[8];
    char umidStr[8];
    dtostrf(temperatura, 4, 1, tempStr);
    dtostrf(umidade, 4, 1, umidStr);

    mqtt.publish(TOPIC_TEMP, tempStr);
    mqtt.publish(TOPIC_UMID, umidStr);
    Serial.printf("Temperatura: %.1f °C | Umidade: %.1f %%\n", temperatura, umidade);
  } else {
    Serial.println("Erro ao ler DHT11");
  }

  // === LDR ===
  int luz = analogRead(LDR_PIN);
  char luzStr[8];
  itoa(luz, luzStr, 10);
  mqtt.publish(TOPIC_LUZ, luzStr);
  Serial.printf("Luminosidade: %d\n", luz);

  // === Ultrassônico ===
  float distancia = medirDistancia();
  if (distancia > 0) {
    Serial.printf("Distância: %.1f cm\n", distancia);
  } else {
    Serial.println("Sem leitura do sensor ultrassônico");
  }

  if (distancia > 0 && distancia < 15) {  // trem detectado
    digitalWrite(LED_TREM, HIGH);
    mqtt.publish(TOPIC_TREM, "Trem detectado");
  } else {
    digitalWrite(LED_TREM, LOW);
    mqtt.publish(TOPIC_TREM, "Sem trem");
  }

  delay(2000);
}