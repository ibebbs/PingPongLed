#include <TimeLib.h> 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
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

// NTP Servers:
// IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
IPAddress timeServer(81, 21, 76, 27); // 0.pool.ntp.org

const int timeZone = 0;     // UTC
//const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)

const char mode_time = 't';
const char mode_custom = 'c';

// Define the array of leds
CRGB leds[NUM_LEDS];
char current_mode = mode_time;
bool ledsDirty = false;

const char* ssid = "OmniStage2";
const char* password = "************";

ESP8266WebServer server(80);

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

const int led = 13;


const int weekday_offset = 40;
const CRGB day_colours[7] = {
  CRGB(148, 0, 211), 
  CRGB(75, 0, 130), 
  CRGB(0, 0, 255), 
  CRGB(0, 255, 0), 
  CRGB(255, 255, 0), 
  CRGB(255, 127, 0), 
  CRGB(255, 0, 0) 
};

const CRGB zero_colour = CRGB(0, 0, 0);

const int rows = 6;
const int columns = 8;


time_t last_time;

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

  current_mode = mode_custom;
  ledsDirty = true;

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

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void setup(void) {
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
  
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);

  digitalClockDisplay();

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/leds", HTTP_POST, handleLeds);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void refreshLeds() {
  if (ledsDirty)
  {
    FastLED.show();
    ledsDirty = false;
  }
}

int indexOf(int x, int y) {
  int acty = rows - y;
  int row_type = acty % 2;

  switch (row_type) {
    case 0: return (acty * columns) + x;
    case 1: return ((acty + 1) * columns) - x;
  }  
}

void renderBinary(int bits, int value, int offset, bool forwards, CRGB colour) {
  for (int b = 0; b < bits; b += 1) {
    int mask = 1 << b;

    int led = offset;

    if (forwards) {
      led += b;
    } else {
      led -= b;
    }
    
    if (value & mask) {
      leds[led] = colour;
    } else {
      leds[led] = zero_colour;
    }
  }
}

void renderTime() {
  Serial.println("Rendering time...");
  
  time_t current = now();

  if (second(last_time) != second(current)) {
    int wd = weekday(current) - 1;
    leds[wd + weekday_offset] = day_colours[wd];

    int mon = month(current);
    renderBinary(4, mon, 39, false, day_colours[1]);  
  
    int d = day(current);
    renderBinary(5, d, 24, true, day_colours[2]);  
  
    int h = hour(current);
    renderBinary(6, h, 23, false, day_colours[3]);
  
    int m = minute(current);
    renderBinary(6, m, 8, true, day_colours[4]);

    int s = second(current);
    renderBinary(6, s, 7, false, day_colours[5]);  

    last_time = current;
    ledsDirty = true;
  }
}

void loop(void){
  server.handleClient();

  //Serial.println(current_mode);
  
  switch (current_mode) {
    case mode_time:
      renderTime();
  }

  refreshLeds();
}
