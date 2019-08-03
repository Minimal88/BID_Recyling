#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

const char *ssid = "Kolbi Martinez"; 
const char *password = "familiamartinez";  
const char* host = "https://script.google.com/macros/s/AKfycbx-Gv6XNQ7SOnOGiTmKwBytNmEeKbVjIOjHuV1J291OAAniobs/exec"; 
const char* fingerprint = "89 ff f4 0f 4d 98 09 ed e3 ef 95 f2 8a af af 22 89 08 ac 03";
String url;

void setup() 
{
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Netmask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
}

void loop() 
{
  Serial.print("connecting to ");
  Serial.println(host);
 
  WiFiClientSecure client;

  const int httpPort = 443;
  if (!client.connect(host, httpPort)) 
  {
    Serial.println("connection failed");
    return;
  }

  float t = analogRead(A0);
  
  url = "/macros/s/AKfycbyvGjcryd7c5uNeX6dkleZhmjDKyTrs1L9Lf3kWlPhTZDn9JPrH/exec?func=addData&val="+ String(t);
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(500);
  String section="header";
  while(client.available())
  {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  Serial.println("closing connection");
  delay(6000);
}
