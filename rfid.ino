

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


/************************* WiFi Access Point *********************************/


//bağlanılacak wifi adı ve şifresi

#define WLAN_SSID       xxxxxx//wifi afı
#define WLAN_PASS       xxxxxx//wifi şifresi

/************************* Adafruit.io Setup *********************************/

//adafruit hesabının keyi ve username
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  xxxxxx // use 8883 for SSL
#define AIO_USERNAME  xxxxxx
#define AIO_KEY       xxxxxx
/************ Global State (you don't need to change this!) ******************/


WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY); //bağlantı için gerekli

Adafruit_MQTT_Publish mesafedegiskeni = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/mesafe"); //hangi feedde veri yollanacağı seçilir

Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");



#define SS_PIN D4  
#define RST_PIN D3
const int trigPin = D2;
const int echoPin = D8;

#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;
float distanceInch;


#include <SPI.h>
#include <MFRC522.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);  
int statuss = 0;
int out = 0;
int geriSayac = 5;

void MQTT_connect(); //haberleşmesi için gerekn protokol

void setup() 
{
  Serial.begin(9600);   
  SPI.begin();      
  mfrc522.PCD_Init();   
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); 
  pinMode(2,OUTPUT);

  delay(10);

  //adafruit için baglantı kısmı
  Serial.println(F("Adafruit MQTT demo"));
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());


  mqtt.subscribe(&onoffbutton);


}


void deger()
{
  digitalWrite(trigPin, LOW);
  digitalWrite(trigPin, HIGH);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);

  distanceCm = duration * SOUND_VELOCITY/2;
}

void loop() 
{
  deger();


  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  Serial.println();
  Serial.print(" UID numarası :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  Serial.println();
  if (content.substring(1) == "16 9E AC 6F") 
  {

    digitalWrite(2,HIGH);
    Serial.println("Düzce Üniversitesi Kartı");
    delay(1000);
    Serial.println();
    statuss = 1;
    Serial.print("Mesafe (cm): ");
    Serial.println(distanceCm);
    delay(2000);
        while(distanceCm>0)
    {
      deger();
      Serial.print("Mesafe (cm): ");
      Serial.println(distanceCm);
      delay(2000);
      if(geriSayac > 0){
        if(distanceCm>70)
        {
          geriSayac--;
        }else{
          geriSayac = 5;
        }
      }
      else{
        geriSayac = 5;
        Serial.println("Kartı tekrardan okutunuz");
        digitalWrite(2,LOW);
        break;
      }

    }

  }



  else if(content.substring(1) == "8A 37 5D 0E")
  {
    digitalWrite(2,HIGH);
    Serial.println("RFID Kartı");
    delay(1000);
    Serial.println();
    statuss = 1;
    Serial.print("Mesafe (cm): ");
    Serial.println(distanceCm);
    delay(2000);
    while(distanceCm>0)
    {
      deger();
      Serial.print("Mesafe (cm): ");
      Serial.println(distanceCm);
      delay(2000);
      if(geriSayac > 0){
        if(distanceCm>70)
        {
          geriSayac--;
        }else{
          geriSayac = 5;
        }
      }
      else{
        geriSayac = 5;
        Serial.println("Kartı tekrardan okutunuz");
        digitalWrite(2,LOW);
        break;
      }

    }
  }
  else   {
    Serial.println("Farklı Bir Kart");
    delay(3000);
  }

      MQTT_connect();

 Adafruit_MQTT_Subscribe *subscription;  //bağlantı için gerekli
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
    }
  }

  
  Serial.print(F("\nMesafe degiskeni degeri gonderiliyor")); //adafruite gönderme kısmı
  Serial.print(distanceCm);
  if (! mesafedegiskeni.publish(distanceCm)) {
    Serial.println(F("Cm değeri yollanamadi"));
  } else {
    Serial.println(F("Cm degeri yollandi"));
  }

} 


void MQTT_connect() { //bağlantı için gerekli
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(500);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}




