#include <WiFi.h>
#include <PubSubClient.h>
#include "env.h"

WiFiClient client;
PubSubClient mqtt(client);

void setup() {
  Serial.begin(115200);
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
  mqtt.subscribe("TOPIC_ILUM");  //Recebe msg
  mqtt.setCallback(callback);  //Recebe msg
  Serial.println("\nConectado ao Broker!");
}

void loop() {
  mqtt.publish("TOPIC_ILUM" , "Acender");  //Envia mensagem
  mqtt.loop();
  delay(1000);

}

void callback(char* topic, byte* payload, unsigned int length){  //processa msg recebida
  String msg = "";
  for(int i = 0; i< legth; i++){
    msg += (char) payboard[i];
  }
  if(topic == "TOPIC_ILUM"){  //se TOPICO é Iluminação e msg Acender -> acende led
    digitalWrite(2,HIGH);
  }else if(topic == "TOPIC_ILUM" && msg == "Apagar"){  //se TOPICO é iluminação e msg é Apagar -> apaga led
    digitalWrite(2,LOW);  //processa msg recebida
  }
}

//teste