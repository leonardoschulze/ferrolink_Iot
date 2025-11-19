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

// ======= Controle do trem =======
bool tremDetectadoAnterior = false;
unsigned long tempoInicioTrem = 0;
bool mensagemParadoEnviada = false;

// ======= Controle do LDR =======
unsigned long ultimoEnvioLDR = 0;
const unsigned long intervaloLDR = 10000;

// ======= Controle do DHT11 =======
unsigned long ultimoEnvioTemp = 0;
const unsigned long intervaloTemp = 10000;

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

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duracao == 0) return -1;

  return duracao * 0.034 / 2.0;
}

// ======= FUNÇÃO COM LÓGICA DE ALERTA =======
void enviarTemperaturaUmidade() {
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();

  if (!isnan(temperatura) && !isnan(umidade)) {

    char tempStr[8];
    char umidStr[8];

    dtostrf(temperatura, 4, 1, tempStr);
    dtostrf(umidade, 4, 1, umidStr);

    // Envia temperatura e umidade normalmente
    mqtt.publish(TOPIC_TEMP, tempStr);
    mqtt.publish(TOPIC_UMID, umidStr);

    // ===== ALERTA DE TEMPERATURA =====
    if (temperatura >= 31.0) {
      mqtt.publish(TOPIC_TEMP, "ALERTA: Temperatura alta detectada!");
      Serial.printf("ALERTA! Temperatura: %.1f °C | Umidade: %.1f %%\n",
                    temperatura, umidade);
    } else {
      Serial.printf("Temperatura enviada: %.1f °C | Umidade enviada: %.1f %%\n",
                    temperatura, umidade);
    }
  }
}

void reconnectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Tentando conectar ao broker... ");
    if (mqtt.connect("S1_ESP32", BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");
      piscarCor(0, 1, 0, 200, 3);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Tentando novamente em 5s...");
      piscarCor(1, 1, 0, 300, 1);
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

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    piscarCor(1, 1, 0, 300, 1);
  }

  Serial.println("\nWiFi Conectado!");
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  reconnectMQTT();
}

// ======= Loop =======
void loop() {

  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  mqtt.loop();

  // === Enviar temperatura/umidade ===
  if (millis() - ultimoEnvioTemp >= intervaloTemp) {
    enviarTemperaturaUmidade();
    ultimoEnvioTemp = millis();
  }

  // === LDR ===
  if (millis() - ultimoEnvioLDR >= intervaloLDR) {

    int luz = analogRead(LDR_PIN);
    char luzStr[8];
    itoa(luz, luzStr, 10);

    mqtt.publish(TOPIC_LUZ, luzStr);
    Serial.printf("LDR enviado: %d\n", luz);

    ultimoEnvioLDR = millis();

    if (luz >= 2500) {
      mqtt.publish(TOPIC_LUZ, "Noite detectada");
      Serial.println("MQTT → Noite detectada");
      setColor(1, 1, 1);
    } else {
      mqtt.publish(TOPIC_LUZ, "Dia detectado");
      Serial.println("MQTT → Dia detectado");
      setColor(0, 0, 0);
    }
  }

  // === Ultrassônico ===
  float distancia = medirDistancia();
  bool tremAgora = (distancia > 0 && distancia < 15);

  if (tremAgora && !tremDetectadoAnterior) {
    mqtt.publish(TOPIC_TREM, "Trem detectado");
    Serial.println("MQTT → Trem detectado");
    digitalWrite(LED_TREM, HIGH);

    tempoInicioTrem = millis();
    mensagemParadoEnviada = false;
  }

  if (tremAgora && tremDetectadoAnterior) {
    if (!mensagemParadoEnviada) {
      unsigned long tempoParado = millis() - tempoInicioTrem;

      if (tempoParado >= 5000) {
        mqtt.publish(TOPIC_TREM, "Trem parado no ponto B");
        Serial.println("MQTT → Trem parado no ponto B");
        mensagemParadoEnviada = true;
      }
    }
  }

  if (!tremAgora && tremDetectadoAnterior) {
    Serial.println("Trem saiu do ponto B");
    digitalWrite(LED_TREM, LOW);
    mensagemParadoEnviada = false;
  }

  tremDetectadoAnterior = tremAgora;

  delay(200);
}
