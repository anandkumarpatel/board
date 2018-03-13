// /* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

const char *ssid = "MuleIoT_2-4GHz";
const char *password = "Mulesoft123";

const uint16_t PixelCount = 150;                   // make sure to set this to the number of pixels in your strip
const uint16_t PixelPin = 2;                       // make sure to set this to the correct pin, ignored for Esp8266
const uint16_t AnimCount = PixelCount / 5 * 2 + 1; // we only need enough animations for the tail and one extra

const uint16_t PixelFadeDuration = 1000; // third of a second
// one second divide by the number of pixels = loop once a second
const uint16_t NextPixelMoveDuration = 5000 / PixelCount; // how fast we move through the pixels

NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);

struct MyAnimationState
{
  RgbColor StartingColor;
  RgbColor EndingColor;
  uint16_t IndexPixel; // which pixel this animation is effecting
};

NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object
MyAnimationState animationState[AnimCount];
uint16_t frontPixel = 0; // the front of the loop
RgbColor frontColor;     // the color at the front of the loop

int start = 1;
ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot()
{
  server.send(200, "text/html", "<h1>I work</h1>");
}
void handleChange()
{
  server.send(200, "text/html", "<h1>light changed</h1>");
  start++;
}
void handleOff()
{
  server.send(200, "text/html", "<h1>lights off</h1>");
  start = 0;
  strip.ClearTo(RgbColor(0, 0, 0));
  strip.Show();
}

void handleLight()
{
  server.send(200, "text/html", "<h1>stright wight</h1>");
  start = 0;
  strip.ClearTo(RgbColor(255, 255, 255));
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
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/change", handleChange);
  server.on("/off", handleOff);
  server.on("/light", handleLight);
  server.begin();
  Serial.println("HTTP server started");
  strip.Begin();
  strip.Show();
  // we use the index 0 animation to time how often we move to the next
  // pixel in the strip
  animations.StartAnimation(0, NextPixelMoveDuration, LoopAnimUpdate);
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
  if (start > 0)
  {
    animations.UpdateAnimations();
    strip.Show();
  }
}

void FadeOutAnimUpdate(const AnimationParam &param)
{
  // this gets called for each animation on every time step
  // progress will start at 0.0 and end at 1.0
  // we use the blend function on the RgbColor to mix
  // color based on the progress given to us in the animation
  RgbColor updatedColor = RgbColor::LinearBlend(
      animationState[param.index].StartingColor,
      animationState[param.index].EndingColor,
      param.progress);
  // apply the color to the strip
  strip.SetPixelColor(animationState[param.index].IndexPixel,
                      colorGamma.Correct(updatedColor));
}

void LoopAnimUpdate(const AnimationParam &param)
{
  // wait for this animation to complete,
  // we are using it as a timer of sorts
  if (param.state == AnimationState_Completed)
  {
    // done, time to restart this position tracking animation/timer
    animations.RestartAnimation(param.index);

    // pick the next pixel inline to start animating
    //
    frontPixel = (frontPixel + 1) % PixelCount; // increment and wrap
    int color = start * 10;
    if (start > 70)
    {
      start = 1;
      color = 0;
    }

    if (frontPixel == 0)
    {
      Serial.println("LOOP DONE");
      Serial.println(start);
      start++;
      frontColor = HslColor(color / 360.0f, 1.0f, 0.5f);
    }

    uint16_t indexAnim;
    // do we have an animation available to use to animate the next front pixel?
    // if you see skipping, then either you are going to fast or need to increase
    // the number of animation channels
    if (animations.NextAvailableAnimation(&indexAnim, 1))
    {
      animationState[indexAnim].StartingColor = frontColor;
      animationState[indexAnim].EndingColor = RgbColor(0, 0, 0);
      animationState[indexAnim].IndexPixel = frontPixel;

      animations.StartAnimation(indexAnim, PixelFadeDuration, FadeOutAnimUpdate);
    }
  }
}
