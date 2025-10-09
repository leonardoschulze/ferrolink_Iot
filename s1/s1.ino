#include <Wifi.h>

const String SSID = "FIESC_IOT_EDU";
const String PASS = "8120gv08";

void setup() {
  Serial.begin(115200);
  Serial.begin("Conectando ao WIFI");
  Wifi.begin(SSID, PASS);
  while(Wifi.status() !=WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nConectado com Sucesso!");
}

void loop() {
 
}
