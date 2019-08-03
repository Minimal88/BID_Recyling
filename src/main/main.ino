/**
 *
 * HX711 library for Arduino - example file
 * https://github.com/bogde/HX711
 *
 * MIT License
 * (c) 2018 Bogdan Necula
 *
**/
#include <math.h>
#include "HX711.h"
#include <ESP8266WiFi.h> 
#include <ArduinoOTA.h>

#include <PubSubClient.h>
#include <WiFiUdp.h>


#include<stdio.h>
#include<stdlib.h>

#include "Config.h"

// HX711 circuit wiring
const int HX711_DATA = 14;
const int HX711_CLK = 13;

const int WAKE_PIN = 1;

#define DEBUG true
#define ENABLE_OTA false
#define Serial if(DEBUG)Serial
#define DATA_WINDOW_SIZE 3
#define BATT_CUTOFF 2280
#define BATT_FULL 2812
#define RTC_MAGIC 0x75a78fc5
//Set Container ID
#define ContainerID "1" //Change this to be unique for each container!
#define mqttContainer "tinkermax/f/smartcontainer" ContainerID
#define mqttBatt "tinkermax/f/smartcontainerbatt" ContainerID
#define randName "Container" ContainerID

// Set the ADCC to read supply voltage.
ADC_MODE(ADC_VCC);

// RTC memory structure
typedef struct {
  uint32 magic;
  uint64 calibration;
  float weight;
  boolean battwarningsent;
  boolean weightwarningsent;
} RTC;

RTC RTCvar;

// Define the scale sensor variable 
HX711 scale;

WiFiClient wclient;
PubSubClient client(wclient);


float sensorData[DATA_WINDOW_SIZE+1];
float current_weight;
float last_weight;
float battlevel;
boolean battwarningsent, weightwarningsent;
boolean mqtt_server_con = false;
boolean waiforOTA = false;

char weight_str[50], battlevel_str[50];
char buffer[100]; //for the sprintf outputs
int keeptrying; 

// Define needed constants
const float scale_calib = 208.91;
const float containerFullWeight = 2015; //Weight of the contents only (excluding container weight)
const float containerAlertWeight = 300; //Alert when contents reach this weight (excluding container weight)

int wakeup_val;


void setup() { 
  // Sets the wakepu pin as input
  pinMode(WAKE_PIN, INPUT);    

  // Setup console
  Serial.begin(500000);
  Serial.println("-->IOT RECYCLE TRACKER<--");

  Serial.println("Initializing the scale...");
  // Initialize library with data output pin, clock input pin and gain factor 128.  
  scale.begin(HX711_DATA, HX711_CLK);

  //Read saved parameters
  system_rtc_mem_read(64, &RTCvar, sizeof(RTCvar));

  // Se calibration scale (manually adjust constant, See README)
  scale.set_scale(scale_calib); 

  // First time initialisation (after replacing battery and/or discharging caps to clear RTC memory)
  if (RTCvar.magic != RTC_MAGIC) {
    // Setup Scale
    scale.read();
    yield();
    scale.tare();               // reset the scale to 0 (with nothing on it)
    yield();
    last_weight = 0;
    
    RTCvar.magic = RTC_MAGIC;
    RTCvar.calibration = scale.get_offset();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    //Wi-Fi must be present on initialisation start up as a failure probably indicates incorrect credentials
    //In that case we do not want to go into deep sleep
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      Serial.print(".");
      delay(20000);
      ESP.restart();
    }
    
    Serial.println("");

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (ENABLE_OTA == true){
      //Initialise OTA in case there is a software upgrade
      ArduinoOTA.setHostname("Container");
      ArduinoOTA.onStart([]() {
        Serial.println("Start");
      });
      ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
      });
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      });
      ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });
        
      ArduinoOTA.begin();

      //wait 10 secs and check if button pressed, which will trigger a wait for the OTA reflash
      Serial.println("Waiting for any OTA updates");
      keeptrying = 20;
      while (keeptrying-- > 0 || waiforOTA == true) {
        if (digitalRead(0) == 0) {
          waiforOTA = true;
          Serial.print ("OTA");
        }
        Serial.print (".");
        ArduinoOTA.handle();
        delay(500);
      }      
    }

    Serial.println("");
   
    Serial.println("rtc memory init...");
  }

  // If waking up from deep sleep, lookup saved values
  else {
    scale.set_offset(RTCvar.calibration);
    last_weight = RTCvar.weight;
    battwarningsent = RTCvar.battwarningsent;
    //weightwarningsent = RTCvar.weightwarningsent;
  }

  // Specify MQTT server
//  client.setServer(mqtt_server, 1883);

  // Read Sensor Load value
  Serial.print("one reading:\t");
  Serial.println(scale.get_units(), 1);

  current_weight = scale.get_units(10);
  Serial.print("Weight:\t");
  Serial.println(current_weight);


  //Read supply voltage value
  //Need to recharge at BATT_CUTOFF (represents 3.6V before diode)
  battlevel= ESP.getVcc();
  Serial.print("battlevel: ");
  Serial.println(battlevel);

  // reset battery warning if not low (allowing for hysteresis)
  if (battlevel > BATT_CUTOFF + 50) {
    battwarningsent = false;
  }

  // Reconnect to MQTT broker
  /*client.loop();
  if (!client.connected()) {
    Serial.println("Reconnecting to MQTT Broker");
    mqtt_server_con = reconnect();
  }*/

  // if successfully connected to broker, proceed to send message
      if (mqtt_server_con == true) {
        Serial.println("Sending mqtt updates"); 
         
        // Calculates  current_weight in terms of %full
        dtostrf(100.0 * (current_weight  / containerFullWeight),1,0,weight_str);
        // Calculates  batt in terms of %full
        dtostrf(100.0 * (battlevel - BATT_CUTOFF) / (BATT_FULL - BATT_CUTOFF),1,0,battlevel_str);
        client.publish(mqttContainer, weight_str);
        client.publish(mqttBatt, battlevel_str);
        sprintf(buffer, "%s,%s", weight_str, battlevel_str);
        Serial.println(buffer);
        Serial.println("\najajajaj");
        
        //Only send one battery warning
        /*
        if (battlevel <= BATT_CUTOFF) {
          sprintf(buffer, "Greek yoghurt battery - %s%% charge", battlevel_str);
          if (sendNotification(buffer) > 0) {
            battwarningsent = true;
          }
        }*/
  
        //Only send one replenishment warning
        /*
        if (current_weight <= (containerAlertWeight + containerEmptyWeight) && weightwarningsent == false) {
          sprintf(buffer, "Greek yoghurt - %s%% remaining", weight_str);
          if (sendNotification(buffer) > 0) {
            weightwarningsent = true;
          }
        }*/
  
        last_weight = current_weight;
      }

  // put the ADC in sleep mode  
  scale.power_down();             
  delay(1000);
  scale.power_up();  

  RTCvar.weight = last_weight;
  RTCvar.battwarningsent = battwarningsent;
  RTCvar.weightwarningsent = weightwarningsent;
  system_rtc_mem_write(64, &RTCvar, sizeof(RTCvar));
  delay(1);
  Serial.print("Going to sleep");
  
  //Sleep for 60 minutes
  //ESP.deepSleep(60 * 60 * 1000000, WAKE_RF_DEFAULT);
  ESP.deepSleep(1000000, WAKE_RF_DEFAULT); // 10 seconds
  delay(500);
}



boolean reconnect() {
  // Loop until we're reconnected to MQTT Broker - but try a maximum of 5 times to avoid draining battery
  int keeptrying = 5;
  
  while (!client.connected() && keeptrying-- > 0) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(randName, mqttuser, mqttpwd)) {
      Serial.println("connected");
      return true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
  return false;
}

void loop() {

  Serial.println(digitalRead(WAKE_PIN));
  delay(1000);
  ESP.wake();
    
  
}
