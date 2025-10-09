#include <WiFi.h>

const String SSID = "nome";
const String PASS = "senha";


void setup() {
  serial.begin(115200);
  serial.println("Conectando ao WIFI");
  WiFi.begin(SSID,PASS);

  while(WiFi.status () != WL_CONNECTED){
    serial.print(".");
    delay(100);
  }
  serial.println("\nConectado com Sucesso");
}

void loop() {
  // put your main code here, to run repeatedly:

}
