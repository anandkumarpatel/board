// /* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

const char *ssid = "Fire Alarm";
const char *password = "connectme";

const uint16_t PixelCount = 200; // make sure to set this to the number of pixels in your strip
const uint16_t PixelPin = 2;     // make sure to set this to the correct pin, ignored for Esp8266

NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);

int red = 0;
int green = 0;
int blue = 0;

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot()
{
  server.send(200, "text/html", "<h1>I work</h1>");
}
void handleChangeRed()
{
  server.send(200, "text/html", "<h1>light changed</h1>");
  red = red + 10;
  if (red > 255)
  {
    red = 0;
  }
  strip.ClearTo(RgbColor(red, green, blue));
  strip.Show();
}

void handleChangeGreen()
{
  server.send(200, "text/html", "<h1>light changed</h1>");
  green = green + 10;
  if (green > 255)
  {
    green = 0;
  }
  strip.ClearTo(RgbColor(red, green, blue));
  strip.Show();
}

void handleChangeBlue()
{
  server.send(200, "text/html", "<h1>light changed</h1>");
  blue = blue + 10;
  if (blue > 255)
  {
    blue = 0;
  }
  strip.ClearTo(RgbColor(red, green, blue));
  strip.Show();
}

void handleOff()
{
  server.send(200, "text/html", "<h1>lights off</h1>");
  red = 0;
  blue = 0;
  green = 0;
  strip.ClearTo(RgbColor(red, green, blue));
  strip.Show();
}

void handleLight()
{
  server.send(200, "text/html", "<h1>full blast</h1>");
  red = 255;
  blue = 255;
  green = 255;
  strip.ClearTo(RgbColor(red, green, blue));
  strip.Show();
}

void handleDim()
{
  server.send(200, "text/html", "<h1>dimming</h1>");
  red = red - 10;
  blue = blue - 10;
  green = green - 10;

  if (blue < 0)
  {
    blue = 0;
  }

  if (red < 0)
  {
    red = 0;
  }

  if (green < 0)
  {
    green = 0;
  }
  Serial.println("dim");
}

void handleBright()
{
  server.send(200, "text/html", "<h1>brighter</h1>");
  red = red + 10;
  blue = blue + 10;
  green = green + 10;

  if (blue > 255)
  {
    blue = 255;
  }

  if (red > 255)
  {
    red = 255;
  }

  if (green > 255)
  {
    green = 255;
  }
  Serial.println("bright");
  strip.ClearTo(RgbColor(red, green, blue));
  strip.Show();
}
void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ; // wait for serial attach
  Serial.println();
  Serial.println("Connecting to wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(500000);
    ESP.restart();
  }
  ArduinoOTA.setHostname("lightstrip");

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.print("IP address: ");

  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/red", handleChangeRed);
  server.on("/green", handleChangeGreen);
  server.on("/blue", handleChangeBlue);
  server.on("/dim", handleDim);
  server.on("/bright", handleBright);
  server.on("/off", handleOff);
  server.on("/on", handleLight);
  server.begin();
  Serial.println("HTTP server started");
  strip.Begin();
  strip.Show();
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
}
