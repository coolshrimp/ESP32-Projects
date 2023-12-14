#include <Arduino.h>
#include "HomeSpan.h"

// Define global pins
const int redPin = 12;
const int greenPin = 13;
const int bluePin = 14;

struct RgbLED : Service::LightBulb {
    LedPin *redLedPin, *greenLedPin, *blueLedPin;
    SpanCharacteristic *power;
    SpanCharacteristic *H;
    SpanCharacteristic *S;
    SpanCharacteristic *V;
    bool invertBrightness;

    RgbLED(int red_pin, int green_pin, int blue_pin, bool invertBrightness) : Service::LightBulb() {
        power = new Characteristic::On();
        H = new Characteristic::Hue(0);
        S = new Characteristic::Saturation(0);
        V = new Characteristic::Brightness(100);
        V->setRange(5, 100, 1);
        redLedPin = new LedPin(red_pin);
        greenLedPin = new LedPin(green_pin);
        blueLedPin = new LedPin(blue_pin);
        this->invertBrightness = invertBrightness;

        Serial.println("Configuring RGB LED: Pins=(" + String(redLedPin->getPin()) + "," + String(greenLedPin->getPin()) + "," + String(blueLedPin->getPin()) + ")");
    }

    boolean update() {
        boolean p;
        float v, h, s, r, g, b;
        h = H->getVal<float>();
        s = S->getVal<float>();
        v = V->getVal<float>();
        p = power->getVal();
        Serial.println("Updating RGB LED: Pins=(" + String(redLedPin->getPin()) + "," + String(greenLedPin->getPin()) + "," + String(blueLedPin->getPin()) + "):");
        // Handle characteristic updates and print values
        if (power->updated()) {
            p = power->getNewVal();
            Serial.println("Power=" + String(power->getVal() ? "true" : "false") + "->" + String(p ? "true" : "false") + ", ");
        } else {Serial.println("Power=" + String(p ? "true" : "false") + ", ");}
        if (H->updated()) {
            h = H->getNewVal<float>();
            Serial.println("H=" + String(H->getVal<float>()) + "->" + String(h) + ", ");
        } else {Serial.println("H=" + String(h) + ", ");}
        if (S->updated()) {
            s = S->getNewVal<float>();
            Serial.println("S=" + String(S->getVal<float>()) + "->" + String(s) + ", ");
        } else {Serial.println("S=" + String(s) + ", ");}
        if (V->updated()) {
            v = V->getNewVal<float>();
            Serial.println("V=" + String(V->getVal<float>()) + "->" + String(v) + "  ");
        } else {Serial.println("V=" + String(v) + "  ");}
        LedPin::HSVtoRGB(h, s / 100.0, v / 100.0, &r, &g, &b);
        int R, G, B;
        if (invertBrightness) {
            R = 255 - p * r * 255; // Invert red
            G = 255 - p * g * 255; // Invert green
            B = 255 - p * b * 255; // Invert blue
        } else {
            R = p * r * 255;
            G = p * g * 255;
            B = p * b * 255;                    
        }
        Serial.println("RGB=(" + String(R) + "," + String(G) + "," + String(B) + ")");

          // Define the bounds
          const int minBrightness = 0;
          const int maxBrightness = 105;
          // User-provided R value (0 to 255)
          // Map the R value to fit within the specified bounds
          int mappedR = round(map(R, 0, 255, minBrightness, maxBrightness));
          int mappedG = round(map(G, 0, 255, minBrightness, maxBrightness));
          int mappedB = round(map(B, 0, 255, minBrightness, maxBrightness));

          // Ensure that values very close to 100 are rounded up to 100
          if (mappedR >= 70.5) mappedR = 255;
          if (mappedG >= 70.5) mappedG = 255;
          if (mappedB >= 70.5) mappedB = 255;

          // Set the redLedPin to the mapped brightness value
          redLedPin->set(mappedR);
          greenLedPin->set(mappedG);
          blueLedPin->set(mappedB);

        Serial.println("RGB=(" + String(mappedR) + "," + String(mappedG) + "," + String(mappedB) + ")");

        return (true);
    }
};

void setup() {
    Serial.begin(115200);
    homeSpan.begin(Category::Lighting, "LED Strip", "CoolshrimpModz");
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name("5V RGB LED Strip");
    new Characteristic::Manufacturer("Coolshrimp Modz");
    new Characteristic::Model("3-LED's");
    new RgbLED(redPin, greenPin, bluePin, true);
}

void loop() {
    homeSpan.poll();
}
