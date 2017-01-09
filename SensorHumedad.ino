#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* Parámetros de conexión red WiFi WiFi *********************************/
#define WLAN_SSID       "TELEPORTE.es 0D8_927750750"
#define WLAN_PASS       "RqnMiaPe@#76:01+"

/************************* Parámatros de conexión Servidor MQTT (e.g., Adafruit.io) *********************************/
#define AIO_SERVER      "192.168.2.106"
#define AIO_SERVERPORT  1883                   //8883 para SSL
//#define AIO_USERNAME    "bordiales"
//#define AIO_KEY         "f870d1bc5f844618a8f2a4c1b06bf991"

#define HumedadPin A0
#define relePin 12
//#define LED_GPIO 16

/************ Variables para cliente WiFi y cliente MQTT ******************/
// Crea un cliente ESP8266
WiFiClient client; //usar: WiFiClientSecure client; para cliente SSL

// Crea el cliente MQTT
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);

/****************************** Canales (Feeds) ***************************************/
// La ruta cambia según el servidor MQTT usado, para Adafruit.io: <username>/feeds/<feedname>


//SUSCRIPCIONES
Adafruit_MQTT_Subscribe canalhora = Adafruit_MQTT_Subscribe(&mqtt, "fecha/hora");
Adafruit_MQTT_Subscribe canalhumedad = Adafruit_MQTT_Subscribe(&mqtt, "tiempo/humedad");
Adafruit_MQTT_Subscribe canaltemperatura = Adafruit_MQTT_Subscribe(&mqtt, "tiempo/celsius");
Adafruit_MQTT_Subscribe canalriegomanual = Adafruit_MQTT_Subscribe(&mqtt, "jardin/riego/manual");



//PUBLIC
// Publicaremos en un canal el valor del sensor de humedad, el canal se llamara "canalhumedad"
Adafruit_MQTT_Publish canalsuelo = Adafruit_MQTT_Publish(&mqtt, "jardin/tiempo/humedad");

// Publicaremos en un canal el valor 1 cada vez que se riege, el canal se llamara "canalriego"
Adafruit_MQTT_Publish canalriego = Adafruit_MQTT_Publish(&mqtt, "jardin/riego/automatico");

// Variable para el calculo de humedad para su envío
unsigned int porcentajeHumedad;

//Variable para el control de errores de envío al publicar
unsigned int errorPublicacion;
// Función para conectar inicialmente y reconectar cuando se haya perdido la conexión al servidor MQTT

  uint16_t humedad;
  uint16_t temperatura;
  uint16_t hora;
  char riegomanual;

void MQTT_connect() {
  int8_t ret;

  if (!mqtt.connected()) {

    Serial.println("Conectando al servidor MQTT... ");

    uint8_t intentos = 3;
    while ((ret = mqtt.connect()) != 0) { // connect devuelve 0 cuando se ha conectado correctamente y el código de error correspondiente en caso contrario
      Serial.println(mqtt.connectErrorString(ret)); // (sólo interesante para debug)
      Serial.println("Reintentando dentro de 3 segundos...");
      mqtt.disconnect();
      delay(3000);  // esperar 3 segundos
      if (! intentos--) { //decrementamos el número de intentos hasta que sea cero
        while (1);  // El ESP8266 no soporta los while(1) sin código dentro, se resetea automáticamente, así que estamos forzando un reset
          //Si no quisieramos que se resetease, dentro del while(1) habría que incluir al menos una instrucción, por ejemplo delay(1); o yield();
      }
    }
    Serial.println("MQTT conectado");
  }
}

void WIFI_connect() {
  // Poner el módulo WiFi en modo station (este modo permite de serie el "light sleep" para consumir menos
  // y desconectar de cualquier red a la que pudiese estar previamente conectado
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);

  // Conectamos a la WiFi
  Serial.println("Conectando a la red WiFi");

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) { //Nos quedamos esperando hasta que conecte
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado.");
}

void regar(){
  digitalWrite(relePin,LOW);
  //digitalWrite(LED_GPIO, HIGH);
  Serial.println("Regando...\n");
  canalriego.publish("SI");
  delay(3000);
  digitalWrite(relePin,HIGH);
  canalriego.publish("NO");
  Serial.println("Fin riego\n");
  
}

void noregar(){
   digitalWrite(relePin,HIGH);
   //digitalWrite(LED_GPIO, LOW);
   Serial.println("Riego cerrado");
   canalriego.publish("NO");
}



void setup() {

   Serial.begin(115200);
  
   pinMode(HumedadPin,INPUT);
   pinMode(relePin,OUTPUT);
   //pinMode(LED_GPIO, OUTPUT);
   digitalWrite(relePin,HIGH); //creo que el rele funciona con lógica inversa, arrancamos con el rele apagado

   //Conectamos a la WiFi
  WIFI_connect();

  // Nos subscribimos a los canales
  mqtt.subscribe(&canalhumedad);
  mqtt.subscribe(&canaltemperatura);
  mqtt.subscribe(&canalhora);
  mqtt.subscribe(&canalriegomanual);
}

void loop() 
{

  
  MQTT_connect();

   //ESCUCHAMOS LOS CANALES A LOS QUE ESTAMOS SUSCRITOS
//hora= 10;
//temperatura = 20;
 Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(500))) {

      if(subscription == &canalhumedad){
         humedad = atoi((char *)canalhumedad.lastread); 
         Serial.print("Canal humedad = ");
         Serial.println((char *)canalhumedad.lastread);
      }
       if (subscription == &canalhora){
        hora = atoi((char *)canalhora.lastread);
        Serial.print("Canal hora = ");
        Serial.println((char *)canalhora.lastread);
      }
      
      if (subscription == &canaltemperatura){
        temperatura = atoi((char *)canaltemperatura.lastread);
        Serial.print("Canal temperatura = ");
        Serial.println((char *)canaltemperatura.lastread);
      }
      if(subscription == &canalriegomanual){
         riegomanual = atoi((char *)canalriegomanual.lastread); 
         Serial.print("Canal riego manual = ");
         Serial.println((char *)canalriegomanual.lastread);
      }

    }

  //PUBLICAMOS LA HUMEDAD LEIDA POR NUESTRO SENSOR

  porcentajeHumedad = map(analogRead(HumedadPin), 0, 1023, 100, 0);
  //porcentajeHumedad = analogRead(HumedadPin) * 100 / 1024; //Los valores leidos por el sensor van de 0 a 1024, los transformamos de 0 a 100%
  
  //Si no hay error de publicación, la función devuelve 0, sino el código de error correspondiente (sólo interesante para debug)
  if (! (errorPublicacion = canalsuelo.publish(porcentajeHumedad))) {
    Serial.print("Error de publicación (error ");
    Serial.print(errorPublicacion);
    Serial.println(")");
  }
   
  Serial.print("humedad suelo = ");
  Serial.print(porcentajeHumedad);
  Serial.print("% -- ");

  Serial.print("humedad tiempo = ");
  Serial.print(humedad);
  Serial.print("% -- ");

  Serial.print("temperatura = ");
  Serial.print(temperatura);
  Serial.print(" -- ");

  Serial.print("hora = ");
  Serial.println(hora);

  if (riegomanual==0){
    if(porcentajeHumedad < 70){ //tierra seca
      if (humedad < 80){ //poca humedad en el ambiente
        if (temperatura > 25){
          if (hora > 20 || hora < 8){
            //REGAMOS. Tiempo poco humedo, temperatura alta y buena hora
            Serial.println("Tiempo poco humedo, temperatura alta y buena hora - REGAMOS");
            regar();
          }
          else{ //horas de sol
            //NO REGAMOS
            Serial.println("Tiempo poco humedo, temperatura alta y horas de sol - NO REGAMOS");
            noregar();
          }
        }
        else{//temperatura baja
          //REGAMOS
          if (temperatura > 5){
          Serial.println("Tiempo poco humedo, temperatura baja - REGAMOS");      
          regar();
          }
          else{
            Serial.println("Temperatura demasido baja - NO REGAMOS");
            noregar();
          }
        }
      }
      else {
          //NO REGAMOS, TIEMPO HUMEDO
          Serial.println("Tiempo humedo - NO REGAMOS");
          noregar();
        }
    }
    else { //NO REGAMOS, SUELO HUMEDO
    Serial.println("Suelo humedo - NO REGAMOS");
    noregar();
  }    
  }  
  else{
    Serial.println("RIEGO MANUAL");
    regar();
  }

   delay(10000);
   //digitalWrite(bombaPin,LOW);
   
}


