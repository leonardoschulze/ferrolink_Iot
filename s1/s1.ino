#include <WiFi.h>                 // Biblioteca para conectar ao Wi-Fi
#include <WiFiClientSecure.h>     // Cliente HTTPS, necessário para MQTT seguro
#include <PubSubClient.h>         // Biblioteca para comunicação MQTT
#include <Adafruit_Sensor.h>      // Suporte geral para sensores Adafruit
#include <DHT.h>                  // Biblioteca do sensor DHT11
#include "env.h"                  // Arquivo com senhas, tópicos e usuário

// ======= Pinos =======
#define DHTPIN      4             // Pino do DHT11
#define DHTTYPE     DHT11         // Tipo do DHT
#define TRIG_PIN    22            // Trigger do sensor ultrassônico
#define ECHO_PIN    23            // Echo do sensor ultrassônico
#define LED_TREM    19            // LED que indica trem detectado
#define LDR_PIN     34            // Sensor de luminosidade (LDR)
#define LED_R       14            // LED RGB – vermelho
#define LED_G       26            // LED RGB – verde
#define LED_B       25            // LED RGB – azul

// ======= Objetos =======
WiFiClientSecure client;          // Cliente Wi-Fi seguro para MQTT TLS
PubSubClient mqtt(client);        // Objeto MQTT
DHT dht(DHTPIN, DHTTYPE);         // Objeto do sensor DHT11

// ======= Controle do trem =======
bool tremDetectadoAnterior = false;    // Guarda se o trem estava presente antes
unsigned long tempoInicioTrem = 0;     // Momento em que o trem foi detectado
bool mensagemParadoEnviada = false;    // Evita enviar alerta repetido

// ======= Controle do LDR =======
unsigned long ultimoEnvioLDR = 0;      // Último envio do LDR
const unsigned long intervaloLDR = 10000; // Envia leitura do LDR a cada 10 segundos

// ======= Controle do DHT11 =======
unsigned long ultimoEnvioTemp = 0;        // Controle do envio da temperatura
const unsigned long intervaloTemp = 10000; // A cada 10 segundos envia

// ======= Funções Auxiliares =======
void setColor(int r, int g, int b) {  // Liga/desliga cada cor do LED RGB
  digitalWrite(LED_R, r);
  digitalWrite(LED_G, g);
  digitalWrite(LED_B, b);
}

void piscarCor(int r, int g, int b, int tempo, int repeticoes) {
  for (int i = 0; i < repeticoes; i++) {
    setColor(r, g, b);          // Liga a cor
    delay(tempo);               // Espera
    setColor(0, 0, 0);          // Desliga
    delay(tempo);
  }
}

float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);    // Garante início em LOW
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);   // Pulso de 10 us
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000); // Lê duração do eco
  if (duracao == 0) return -1;   // Se não recebeu nada, retorna -1

  return duracao * 0.034 / 2.0;  // Fórmula para calcular distância em cm
}

// ======= FUNÇÃO COM LÓGICA DE ALERTA =======
void enviarTemperaturaUmidade() {
  float temperatura = dht.readTemperature();  // Lê temperatura
  float umidade = dht.readHumidity();         // Lê umidade

  if (!isnan(temperatura) && !isnan(umidade)) {  // Verifica se leitura é válida

    char tempStr[8];
    char umidStr[8];

    dtostrf(temperatura, 4, 1, tempStr);      // Converte temperatura para string
    dtostrf(umidade, 4, 1, umidStr);          // Converte umidade para string

    mqtt.publish(TOPIC_TEMP, tempStr);        // Envia temperatura
    mqtt.publish(TOPIC_UMID, umidStr);        // Envia umidade

    // ===== ALERTA DE TEMPERATURA =====
    if (temperatura >= 31.0) {                // Se temperatura alta → alerta
      mqtt.publish(TOPIC_TEMP, "ALERTA: Temperatura alta detectada!");
      Serial.printf("ALERTA! Temp: %.1f °C | Umidade: %.1f %%\n",
                    temperatura, umidade);
    } else {
      Serial.printf("Temp enviada: %.1f °C | Umidade enviada: %.1f %%\n",
                    temperatura, umidade);
    }
  }
}

void reconnectMQTT() {
  while (!mqtt.connected()) {                   // Fica tentando até conectar
    Serial.print("Tentando conectar ao broker... ");
    if (mqtt.connect("S1_ESP32", BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");
      piscarCor(0, 1, 0, 200, 3);               // Verde = conectado
    } else {
      Serial.print("Falha, rc=");
      Serial.println(mqtt.state());             // Código do erro
      piscarCor(1, 1, 0, 300, 1);               // Amarelo = erro
      delay(5000);                              // Tenta de novo em 5s
    }
  }
}

// ======= Setup =======
void setup() {
  Serial.begin(115200);              // Inicia Serial

  pinMode(TRIG_PIN, OUTPUT);         // Configura pinos do ultrassônico
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_TREM, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(LED_R, OUTPUT);            // LED RGB
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  dht.begin();                       // Inicia sensor DHT

  client.setInsecure();              // Aceita conexão MQTT sem certificado

  WiFi.begin(WIFI_SSID, WIFI_PASS);  // Conecta ao Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    piscarCor(1, 1, 0, 300, 1);       // LED amarelo até conectar
  }

  Serial.println("\nWiFi Conectado!");

  mqtt.setServer(BROKER_URL, BROKER_PORT); // Configura broker MQTT
  reconnectMQTT();                         // Conecta ao broker
}

// ======= Loop =======
void loop() {

  if (!mqtt.connected()) {  // Mantém MQTT conectado
    reconnectMQTT();
  }
  mqtt.loop();  // Processa mensagens MQTT

  // === Enviar temperatura/umidade ===
  if (millis() - ultimoEnvioTemp >= intervaloTemp) {
    enviarTemperaturaUmidade();
    ultimoEnvioTemp = millis();
  }

  // === LDR ===
  if (millis() - ultimoEnvioLDR >= intervaloLDR) {

    int luz = analogRead(LDR_PIN);           // Lê luminosidade

    char luzStr[8];
    itoa(luz, luzStr, 10);                   // Converte para string

    mqtt.publish(TOPIC_LUZ, luzStr);         // Envia valor bruto

    if (luz >= 2500) {                       // Se pouca luz → noite
      mqtt.publish(TOPIC_LUZ, "Noite detectada");
      setColor(1, 1, 1);                     // Liga LED RGB (branco)
    } else {                                 // Caso contrário → dia
      mqtt.publish(TOPIC_LUZ, "Dia detectado");
      setColor(0, 0, 0);                     // Desliga LED RGB
    }
  }

  // === Ultrassônico ===
  float distancia = medirDistancia();        // Mede distância
  bool tremAgora = (distancia > 0 && distancia < 15); // Trem detectado < 15cm

  if (tremAgora && !tremDetectadoAnterior) {
    mqtt.publish(TOPIC_TREM, "Trem detectado");
    digitalWrite(LED_TREM, HIGH);            // Liga LED
    tempoInicioTrem = millis();              // Marca tempo
    mensagemParadoEnviada = false;
  }

  if (tremAgora && tremDetectadoAnterior) {
    if (!mensagemParadoEnviada) {
      unsigned long tempoParado = millis() - tempoInicioTrem;

      if (tempoParado >= 5000) {             // 5 segundos parado
        mqtt.publish(TOPIC_TREM, "Trem parado no ponto B");
        mensagemParadoEnviada = true;
      }
    }
  }

  if (!tremAgora && tremDetectadoAnterior) {
    digitalWrite(LED_TREM, LOW);             // Apaga LED
    mensagemParadoEnviada = false;
  }

  tremDetectadoAnterior = tremAgora;         // Atualiza estado

  delay(200);                                // Pequeno delay
}
