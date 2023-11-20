/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>


const uint16_t PixelCount = 150; // make sure to set this to the number of pixels in your strip
const uint16_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266
const uint16_t AnimCount = PixelCount / 5 * 2 + 1; // we only need enough animations for the tail and one extra

const uint16_t PixelFadeDuration = 300; // third of a second
// one second divide by the number of pixels = loop once a second
const uint16_t NextPixelMoveDuration = 1000 / PixelCount; // how fast we move through the pixels

NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma

// NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
// For Esp8266, the Pin is ignored and it uses GPIO3.  
// There are other Esp8266 alternative methods that provide more pin options, but also have
// other side effects.
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);
// NeoEsp8266Uart800KbpsMethod also ignores the pin parameter and uses GPI02
//NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(PixelCount, PixelPin);
// NeoEsp8266Uart800KbpsMethod will work with all but pin 16, but is not stable with WiFi 
// being active

// what is stored for state is specific to the need, in this case, the colors and
// the pixel to animate;
// basically what ever you need inside the animation update function



struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};

NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object
MyAnimationState animationState[AnimCount];
uint16_t frontPixel = 0;  // the front of the loop
RgbColor frontColor;  // the color at the front of the loop


/* Set these to your desired credentials. */
const char *ssid = "ESPap";
const char *password = "thereisnospoon";
int start = 0;
ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
  start++;
   if (start == 4) {
     start = 0;
   }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
    strip.Begin();
    strip.Show();
    SetRandomSeed();
    // we use the index 0 animation to time how often we move to the next
    // pixel in the strip
    animations.StartAnimation(0, NextPixelMoveDuration, LoopAnimUpdate);
}

void loop() {
  server.handleClient();

  if (start > 0) { 
    animations.UpdateAnimations();
    strip.Show();
  }
}



void SetRandomSeed()
{
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    // Serial.println(seed);
    randomSeed(seed);
}

void FadeOutAnimUpdate(const AnimationParam& param)
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

void LoopAnimUpdate(const AnimationParam& param)
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
        int color = 0;
        if (start == 1) {
          color = 100;
        }
        if (start == 2) {
          color = 200;
        }
        if (start == 3) {
          color = 300;
        }
        
        if (frontPixel == 0)
        {
            // we looped, lets pick a new front color
            // frontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
            frontColor = HslColor(color / 360.0f, 1.0f, 0.25f);
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

