#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 50 

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806, define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 4
#define CLOCK_PIN 11

// Define the array of leds
CRGB leds[NUM_LEDS];

const char* ssid = "OmniStage2";
const char* password = "B1gB@ngTheory";

ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(led, 0);
}

void handleLeds() {
  String value = server.arg("value");

  //Serial.print(value);

  //String response = "";
  int count = 0;
  char * pch;

  pch = strtok (&value[0],"-");
  while (pch != NULL && count < 50)
  {
    long number = (long) strtol( &pch[0], NULL, 16);
    int r = number >> 16;
    int g = number >> 8 & 0xFF;
    int b = number & 0xFF;

    leds[count++] = CRGB(r,g,b);

    //response = response + "[" + String(count) + "] = [" + String(r) + "," + String(g) + "," + String(b) + "]\n";

    pch = strtok (NULL, "-");
  }
  
  FastLED.show(); 

  //Serial.print(response);

  server.send(204, "text/plain", "Done");
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void){

  digitalWrite(led, 0);
  Serial.begin(115200);
  
  LEDS.addLeds<WS2812,DATA_PIN,RGB>(leds,NUM_LEDS);
  LEDS.setBrightness(84);
  
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/leds", HTTP_POST, handleLeds);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
