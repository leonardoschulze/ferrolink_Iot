#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

const byte TRIGGER_PIN = 5;
const byte ECHO_PIN = 18;
const byte TRIGGER_PIN2 = 19;
const byte ECHO_PIN2 = 21;

WiFiClientSecure client;
PubSubClient mqtt(client);

void setup() {
  Serial.begin(115200);
  client.setInsecure();
  Serial.println("Conectando ao WiFi"); //apresenta a mensagem na tela
  WiFi.begin(WIFI_SSID,WIFI_PASS); //tenta conectar na rede
  while(WiFi.status() != WL_CONNECTED){
    Serial.print("."); // mostra "....."
    delay(200);
  }
  Serial.println("\nConectado com Sucesso!");

  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BROKER_URL,BROKER_PORT);
  String BoardID = "s2";
  BoardID += String(random(0xffff),HEX);
  mqtt.connect(BoardID.c_str() , BROKER_USER , BROKER_PASS);
  while(!mqtt.connected()){
    Serial.print(".");
    delay(200);
  }

   Serial.begin(115200); //sensor1
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(115200); //sensor2
  pinMode(TRIGGER_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);
}

long lerDistancia() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH);
  long distancia = duracao * 0.034924 / 2;

  return distancia; //término sensor1

}

  long lerDistancia2() {
  digitalWrite(TRIGGER_PIN2, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN2, LOW);

  long duracao2 = pulseIn(ECHO_PIN2, HIGH);
  long distancia2 = duracao2 * 0.034924 / 2;

  return distancia2; //término sensor2

  mqtt.subscribe("TOPIC_ILUM");  //Recebe msg
  mqtt.setCallback(callback);  //Recebe msg
  Serial.println("\nConectado ao Broker!");
}

void loop() {

 long distancia = lerDistancia();

  Serial.print("Distância: ");
  Serial.print(distancia);
  Serial.println(" cm");

  if (distancia < 10) {
    Serial.println("Trem detectado!");
    mqtt.publish(TOPIC_PRESENCA1, "Trem detectado");
    delay(2000); 
  }

  delay(500);

   long distancia2 = lerDistancia2();

  Serial.print("Distância: ");
  Serial.print(distancia2);
  Serial.println(" cm");

  if (distancia2 < 10) {
    Serial.println("Trem detectado!");
    mqtt.publish(TOPIC_PRESENCA2, "Trem detectado");
    delay(2000); 
  }

  delay(500);

}

void callback(char* topic, byte* payload, unsigned int length){  //processa msg recebida
  String msg = "";
  for(int i = 0; i< length; i++){
    msg += (char) payload[i];
  }
  if(topic == "TOPIC_ILUM"){  //se TOPICO é Iluminação e msg Acender -> acende led
    digitalWrite(2,HIGH);
  }else if(topic == "TOPIC_ILUM" && msg == "Apagar"){  //se TOPICO é iluminação e msg é Apagar -> apaga led
    digitalWrite(2,LOW);  //processa msg recebida
  }
}

//teste