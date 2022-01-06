
//Please note that at the moment, the SEN0203 sensor does not work as it's meant to when combined with all the additional functinality within this sketch (MQTT and IR temperature)

#define heartratePin    A0

#include <ESP8266WiFi.h> 
#include "Adafruit_MQTT.h" 
#include "Adafruit_MQTT_Client.h" 
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <DFRobot_Heartrate.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
DFRobot_Heartrate heartrate(ANALOG_MODE);

#define WLAN_SSID       "eeeiot" 
#define WLAN_PASS       "status01" 
#define MQTT_SERVER     "192.168.0.162"  // give static address
#define MQTT_PORT         1883                    
#define MQTT_USERNAME    "" 
#define MQTT_PASSWORD         "" 

#define MAC_ADDRESS     "a4:91:b1:ce:c4:59"

//Example taken from: https://www.hackster.io/ruchir1674/raspberry-pi-talking-to-esp8266-using-mqtt-ed9037

WiFiClient client; //Declaring the WiFiclient object

Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD); 

Adafruit_MQTT_Publish pi_pub1 = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/publish1/pi"); //Publishing Field
Adafruit_MQTT_Publish pi_pub2 = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/publish2/pi");

Adafruit_MQTT_Subscribe esp8266_sub = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/subscribe/esp8266"); //Subscribing to changes

IPAddress staticIP(192,168,0,88); //Setting a static IP for this board
IPAddress gateway(192,168,0,1);  //Home WiFi IP address
IPAddress subnet(255,255,255,0);

void MQTT_connect(); 

float Previous_Time = 0;
float Ambient_Temperature;
float Object_Temperature;

void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);
    delay(10);
    Serial.println(F("RPi-ESP-MQTT")); Serial.println(); 
    Serial.print("Connecting to: ");
    Serial.println(WLAN_SSID);
    WiFi.enableInsecureWEP(true); //Added this hoping it would work
    WiFi.disconnect(true);
    WiFi.setAutoConnect(false);
    WiFi.setPhyMode(WIFI_PHY_MODE_11G); //Added these last 3 lines to get it to work
    WiFi.begin(WLAN_SSID, WLAN_PASS, 1); //The last variable is the wifi channel we connect to
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500); 
        Serial.print("."); 
    }
    Serial.println("WiFi connected");
    mqtt.subscribe(&esp8266_sub);    //Setting up the MQTT subscription feed
    mlx.begin();
}

void loop() {
  MQTT_connect(); //Automated MQTT server connection/reconnection
  Adafruit_MQTT_Subscribe *subscription;  //Read the subscription feed

  Object_Temperature = float(mlx.readObjectTempC());
  uint8_t rateValue;
  heartrate.getValue(heartratePin); ///< A1 foot sampled values
  rateValue = heartrate.getRate(); ///< Get heart rate value
  
  if(rateValue)  {
     pi_pub1.publish(rateValue);
     Serial.print("Heart Rate: ");Serial.println(rateValue);
     Serial.print("Time: "); Serial.println(millis());   
  } 
  
  if (! pi_pub2.publish(Object_Temperature)) {
      Serial.println(F("Failed"));
  } 
  
  else {
    //Serial.println(F("OK!"));
  }
  
  while ((subscription = mqtt.readSubscription())) {    //This part of the code waits for incoming subscription packages
      if (subscription == &esp8266_sub) { 
         Serial.print(F("Got: ")); 
         Serial.println((char *)esp8266_sub.lastread); 
      }      
  }

  Previous_Time = millis();
  delay(20);

}

void MQTT_connect() {  //This function is meant to automate the connection/reconnection to the MQTT server
   int8_t ret; 
   // Stop if already connected. 
   if (mqtt.connected()) { 
     return; 
   } 
   uint8_t retries = 3; 
   while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected 
        Serial.println(mqtt.connectErrorString(ret)); 
        Serial.println("Retrying MQTT connection in 5 seconds..."); 
        mqtt.disconnect(); 
        delay(5000);  // wait 5 seconds 
        retries--; 
        if (retries == 0) { 
          // basically die and wait for WDT to reset me 
          while (1); 
        } 
   } 
   Serial.println("MQTT Connected!"); 
}  
