#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h>

#define PIN 16 // NeoPixel data pin

Adafruit_NeoPixel ring(12, PIN, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80);

String MacAddress;
int printed_welcome = 0;
int ota_started = 0;
uint8_t rgb_values[3];


void initWifi()
  {
  pinMode(LED_BUILTIN, OUTPUT);

  const char *ssid = SSID_NAME;
  const char *pass = SSID_PASS;

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println(". ");
  }
  Serial.println("Connected to the WiFi network");
  MacAddress = WiFi.macAddress();
  };

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return ring.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return ring.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return ring.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<ring.numPixels(); i++) {
    ring.setPixelColor(i, c);
    ring.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<ring.numPixels(); i++) {
      ring.setPixelColor(i, Wheel((i+j) & 255));
    }
    ring.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< ring.numPixels(); i++) {
      ring.setPixelColor(i, Wheel(((i * 256 / ring.numPixels()) + j) & 255));
    }
    ring.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < ring.numPixels(); i=i+3) {
        ring.setPixelColor(i+q, c);    //turn every third pixel on
      }
      ring.show();

      delay(wait);

      for (uint16_t i=0; i < ring.numPixels(); i=i+3) {
        ring.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < ring.numPixels(); i=i+3) {
        ring.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      ring.show();

      delay(wait);

      for (uint16_t i=0; i < ring.numPixels(); i=i+3) {
        ring.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}


void blink(int times = 3, int delay_ms = 150)
  {
  // blink LED 3 times to indicate that we are ready to receive OTA updates
  for (int i = 0; i < times; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delay_ms);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delay_ms);
  }
  };

void initOTA()
{
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(SENSOR_NAME);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]()
  {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Start updating " + type); 
    });
  
  ArduinoOTA.onEnd([]()
  {
    digitalWrite(LED_BUILTIN, LOW); 
    Serial.println("OTA End");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  { 
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("OTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("OTA Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("OTA Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("OTA End Failed");
  });
  
  ArduinoOTA.begin();
  ota_started = 1;
}

void initWebServer()
{
  server.on("/blink", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    blink(1, 50);
    request->send(200, "text/html", "<marquee>It blinked!</marquee>");
  });

  server.on("/colorWipe", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    colorWipe(ring.Color(255, 0, 0), 50);
    request->send(200, "text/html", "<marquee>It colorWiped!</marquee>");
  });

  server.on("/rainbow", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    rainbow(20);
    request->send(200, "text/html", "<marquee>It rainbowed!</marquee>");
  });

  server.on("/theaterChase", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    theaterChase(ring.Color(127, 127, 127), 50);
    request->send(200, "text/html", "<marquee>It theaterChased!</marquee>");
  });

  server.on("/theaterChaseGreen", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    theaterChase(ring.Color(0, 127, 0), 50);
    request->send(200, "text/html", "<marquee>It theaterChasedGreen!</marquee>");
  });

  server.on("/theaterChaseRainbow", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    theaterChaseRainbow(50);
    request->send(200, "text/html", "<marquee>It theaterChaseRainbowed!</marquee>");
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  initWifi();
  initOTA();
  initWebServer();
  ring.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  ring.setBrightness(50); // Set brightness to 50%
  ring.show(); // Initialize all pixels to 'off'
}

void printWelcome()
{
  if (printed_welcome == 0 && millis() > 8000)
  {
    blink(3);
    theaterChase(ring.Color(0, 127, 0), 50);
    colorWipe(ring.Color(0, 0, 0), 50);
    Serial.println("MAC            : " + MacAddress);
    Serial.println("IP             : " + WiFi.localIP().toString());
    Serial.println("MDNS           : " + String(SENSOR_NAME) + ".local");
    if (ota_started == 1) {
      Serial.println("OTA            : initialized");
    }
    printed_welcome = 1;
  }
}

void loop() {
  printWelcome();
  ArduinoOTA.handle();
}
