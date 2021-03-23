#include "FastLED.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define tempData 25 // Pin for temperature sensor
#define LED_PIN 27 // Pin for LED strip
#define NUM_LEDS 10 // Number of LED on strip
#define soundPin 13 // Analog input for the sound sensor
#define buttonPin 32 // Pin for state change button
#define clkPin 26 // CLK pin on TM1637 display
#define dioPin 14 // DIO pin on TM1637 display
#define PATH "https://damp-bayou-68931.herokuapp.com/temperature" // Heroku endpoint for temperature data

const char* ssid = "";
const char* password = "";

const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n" \
"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n" \
"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n" \
"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n" \
"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n" \
"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n" \
"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n" \
"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n" \
"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n" \
"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n" \
"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n" \
"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n" \
"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n" \
"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n" \
"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n" \
"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n" \
"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n" \
"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n" \
"+OkuE6N36B9K\n" \
"-----END CERTIFICATE-----\n";

// Initialize variables
int buttonPushCounter = 0;
int lastButtonState = 0;
long lastDebounceTime = 0;
long debounceDelay = 150;
volatile int buttonState = 0;
int brightness = 0;
int fadeAmount = 2;
int temp;
int r;
int g;
int b;

// LED 
CRGB leds[NUM_LEDS];
// Temperature sensor 
OneWire oneWire(tempData);
DallasTemperature sensors(&oneWire);
// Display 
TM1637Display display(clkPin, dioPin);

// create degree fahrenheit symbol
const uint8_t fTemp[] = {
    SEG_A | SEG_B | SEG_F | SEG_G, // CIRCLE
    SEG_A | SEG_F | SEG_G | SEG_E // F
};

HTTPClient http; // init http

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  display.clear();
  display.setBrightness(75);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected successfully boss");

  //  init button as input
  pinMode(buttonPin, INPUT);
  //  init temperature sensor
  sensors.begin();
  //  init LED strip and clear it
  FastLED.addLeds<WS2812B, LED_PIN>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.clear();
//   Attach interrupt 
  attachInterrupt(digitalPinToInterrupt(buttonPin), pin_ISR, CHANGE);
  delay(500);
}

ICACHE_RAM_ATTR void pin_ISR() {
  http.end();

  buttonState = digitalRead(buttonPin);

  if ( (millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState == 1) {
      Serial.println("Button Pressed");
      if (buttonPushCounter >= 3) {
        buttonPushCounter = 0;
      } else {
        buttonPushCounter++;
      }
    
    }
  
  }
  lastDebounceTime = millis();
}

void getHomeTemp() {
  

  http.begin(PATH, root_ca);
  delay(50);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error with request");
  }

  http.end();
}

void soundSensor() {
  
  int value = analogRead(soundPin);
  Serial.println(value);
  int brightness = ((value * 100) / 255); // Brightness will be a percentage of the total 
  if (value > 1000) {
    brightness = value / 40;
  }

  FastLED.setBrightness(brightness);
  
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay(175);

}

void randomRGB() {
  delay(100);
  r = random(255);
  g = random(255);
  b = random(255);

  fill_solid(leds, NUM_LEDS, CRGB(g, r, b));
  FastLED.show();
  delay(50);
  
}

void tempSensor() {
  Serial.println("Checking the Temperature now:");

  sensors.requestTemperatures();

  Serial.print("Temperature: ");
  temp = sensors.getTempFByIndex(0);
  Serial.println(temp);
  display.showNumberDec(temp, false, 2, 0);
  display.setSegments(fTemp, 2, 2);
   if (temp >= 100) {
    r = 126;
    g = 0;
    b = 10;
  } else if (temp >= 95 && temp <= 99) {
    r = 154;
    g = 0;
    b = 15;
  } else if (temp >= 90 && temp <= 94) {
    r = 216;
    g = 1;
    b = 25;
  } else if (temp >= 85 && temp <= 89) {
    r = 254;
    g = 1;
    b = 32;
  } else if (temp >= 80 && temp <= 84) {
    r = 244;
    g = 101;
    b = 35;
  } else if (temp >= 75 && temp <= 79) {
    r = 255;
    g = 158;
    b = 36;
  } else if (temp >= 70 && temp <= 74) {
    r = 255;
    g = 215;
    b = 40;
  } else if (temp >= 65 && temp <= 69) {
    r = 253;
    g = 254;
    b = 43;
  } else if (temp >= 60 && temp <= 64) {
    r = 204;
    g = 255;
    b = 37;
  } else if (temp >= 55 && temp <= 59) {
    r = 128;
    g = 255;
    b = 30;
  } else if (temp >= 50 && temp <= 54) {
    r = 58;
    g = 181;
    b = 74;
  } else if (temp >= 45 && temp <= 49) {
    r = 6;
    g = 247;
    b = 248;
  } else if (temp >= 40 && temp <= 44) {
    
    r = 1;
    g = 204;
    b = 253;
    
  } else if (temp >= 35 && temp <= 39) {
    r = 1;
    g = 126;
    b = 252;
  } else if (temp >= 30 && temp <= 34) {
    r = 34;
    g = 17;
    b = 252;
  } else if (temp >= 25 && temp <= 29) {
    r = 27;
    g = 20;
    b = 100;
  } else if (temp >= 20 && temp <= 24) {
    r = 102;
    g = 46;
    b = 147;
  } else if (temp >= 15 && temp <= 19) {
    r = 147;
    g = 38;
    b = 143;
  } else if (temp >= 10 && temp <= 14) {
    r = 255;
    g = 0;
    b = 254;
  } else if (temp >= 5 && temp <= 9) {
    r = 255;
    g = 123;
    b = 172;
  } else if (temp >= 0 && temp <= 4) {
    r = 254;
    g = 156;
    b = 195;
  } else {
    
    r= 255;
    g = 255;
    b = 255;
  }

  fill_solid(leds, NUM_LEDS, CRGB(g, r, b));
  FastLED.setBrightness(brightness);
  FastLED.show();
  brightness = brightness + fadeAmount;
  if (brightness <= 0 || brightness >= 100) {
    fadeAmount = -fadeAmount;
    
  }

  delay(10);
}

void loop() {
  // put your main code here, to run repeatedly:
      
    
    Serial.print("Number of button pushes: ");
    Serial.println(buttonPushCounter);
    
    if (buttonPushCounter == 0) {
      delay(10);
      Serial.println("Temperature Sensor on");
      tempSensor();
      
    } 
    else if (buttonPushCounter == 1) {
      display.clear();
      delay(10);
      Serial.println("Sound Sensor on");
      soundSensor();
      
    }
    else if (buttonPushCounter == 2) {
      display.clear();
      delay(10);
      Serial.println("Random RGB");
      randomRGB();
      
    } else if (buttonPushCounter == 3) {
      delay(10);
      Serial.println("Getting CLT Temperature");
      getHomeTemp();
    } else {
      buttonPushCounter = 0;
      return;
    }
    
  }
