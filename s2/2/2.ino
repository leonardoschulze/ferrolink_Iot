#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

const byte TRIGGER_PIN  = 5;
const byte ECHO_PIN     = 18;
const byte TRIGGER_PIN2 = 19;
const byte ECHO_PIN2    = 21;

const int LED_R = 13;
const int LED_G = 12;
const int LED_B = 14;

WiFiClientSecure client;
PubSubClient mqtt(client);

void acenderBranco() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}

void apagarRGB() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
}

void conectarAoBroker(){
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  String BoardID = "S2";
  mqtt.connect(BoardID.c_str(), BROKER_USER, BROKER_PASS);
  
  while(!mqtt.connected()){
    BoardID += String(random(0xffff), HEX);
    Serial.print(".");
    delay(200);
  }

  mqtt.subscribe(TOPIC_ILUM);
  mqtt.setCallback(callback);
  Serial.println("\nConectado ao Broker!");
}

void setup() {
  Serial.begin(115200);
  client.setInsecure();

  Serial.println("Conectando ao WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nConectado com Sucesso!");

  Serial.println("Conectando ao Broker...");
  conectarAoBroker();

  // Sensores
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(TRIGGER_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  // LED RGB
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  apagarRGB(); // começa apagado
}

long lerDistancia() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH);
  long distancia = duracao * 0.034924 / 2;

  return distancia;
}

long lerDistancia2() {
  digitalWrite(TRIGGER_PIN2, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN2, LOW);

  long duracao2 = pulseIn(ECHO_PIN2, HIGH);
  long distancia2 = duracao2 * 0.034924 / 2;

  return distancia2;
}

void loop() {
  if(!mqtt.connected()){
    Serial.println("Desconectado do broker, tentando reconexão...");
    conectarAoBroker();
  }

  long distancia = lerDistancia();
  Serial.print("Distância 1: ");
  Serial.print(distancia);
  Serial.println(" cm");

  if(distancia < 10){
    Serial.println("Trem detectado!");
    mqtt.publish(TOPIC_PRESENCA1, "Trem detectado");
    delay(2000);
  }

  delay(500);

  long distancia2 = lerDistancia2();
  Serial.print("Distância 2: ");
  Serial.print(distancia2);
  Serial.println(" cm");

  if(distancia2 < 10){
    Serial.println("Trem detectado!");
    mqtt.publish(TOPIC_PRESENCA2, "Trem detectado");
    delay(2000);
  }

  delay(500);
  mqtt.loop();
}

void callback(char* topic, byte* payload, unsigned int length){
  String msg = "";
  for(int i = 0; i < length; i++){
    msg += (char) payload[i];
  }

  Serial.println("Recebido: " + msg);

  if(strcmp(topic, TOPIC_ILUM) == 0){

    if(msg == "Noite detectada"){
      acenderBranco();   // acende LED RGB em branco
    }

    else if(msg == "Dia detectado"){
      apagarRGB();       // desliga LED RGB
    }
  }
}
