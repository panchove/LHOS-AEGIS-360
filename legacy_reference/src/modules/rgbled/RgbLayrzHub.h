#pragma once

#ifndef __RGBLAYRZHUB_H__
#define __RGBLAYRZHUB_H__

#include <modules/global_objects/GlobalObjectsLayrzHub.h>

class MorseCode {
public:
  MorseCode(int pin, int numPixels)
      : rgbLed(numPixels, pin, NEO_GRB + NEO_KHZ800), colorIndex(0) {
    rgbLed.begin();
    rgbLed.setBrightness(rgbBrightness);
    rgbLed.show();
  }
  ~MorseCode() {
    // Destructor
  }

  void display(int pixelAddress, const String &text, uint32_t color1 = 0xFFFF00,
               uint32_t color2 = 0x0000FF, uint32_t color3 = 0xFF0000,
               uint32_t color4 = 0xFFFFFF) {
    int shortDelay = 100;          // Short delay (dot duration) in milliseconds
    int longDelay = 300;           // Long delay (dash duration) in milliseconds
    int interElementDelay = 100;   // Delay between elements of a character
    int interCharacterDelay = 300; // Delay between characters

    uint32_t colors[4] = {color1, color2, color3, color4};

    for (int i = 0; i < text.length(); i++) {
      char c = text.charAt(i);
      c = toupper(c); // Convert to uppercase

      const char *morse = getMorseCode(c);

      while (*morse) {
        rgbLed.setPixelColor(pixelAddress, colors[colorIndex]);
        rgbLed.show();

        if (*morse == '.') {
          vTaskDelay(shortDelay / portTICK_PERIOD_MS);
        } else if (*morse == '-') {
          vTaskDelay(longDelay / portTICK_PERIOD_MS);
        }

        rgbLed.setPixelColor(0, 0); // Turn off the LED
        rgbLed.show();
        vTaskDelay(interElementDelay / portTICK_PERIOD_MS);

        morse++;
      }

      // Delay between characters
      vTaskDelay(interCharacterDelay / portTICK_PERIOD_MS);

      // Move to the next color in sequence
      colorIndex = (colorIndex + 1) % 4;
    }
  }

private:
  Adafruit_NeoPixel rgbLed;
  int colorIndex;

  // Morse code definitions for [A-Z, 0-9]
  const char *morseCode[36] = {
    ".-",    "-...",  "-.-.",  "-..",   ".",     "..-.",  "--.",
    "....",  "..",    ".---",  "-.-",   ".-..",  "--",    "-.",
    "---",   ".--.",  "--.-",  ".-.",   "...",   "-",     "..-",
    "...-",  ".--",   "-..-",  "-.--",  "--..", // A-Z
    "-----", ".----", "..---", "...--", "....-", ".....", "-....",
    "--...", "---..",
    "----." // 0-9
  };

  const char *getMorseCode(char c) {
    if (c >= 'A' && c <= 'Z') {
      return morseCode[c - 'A'];
    } else if (c >= '0' && c <= '9') {
      return morseCode[c - '0' + 26];
    }
    return "";
  }
};

#endif