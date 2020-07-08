/*
Nate Damen
June 7, 2020
nate.damen@gmail.com

MIT License

Copyright (c) 2020 ND

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#define ssid        "wifi"
#define password    "password"


#include <HTTPClient.h>
#include <HardwareSerial.h>

HardwareSerial MySerial(2);

#include "Adafruit_Soundboard.h"
// Connect to the RST pin on the Sound Board
#define SFX_RST 21
Adafruit_Soundboard sfx = Adafruit_Soundboard(&MySerial, NULL, SFX_RST);

char pasnum = 'A';
uint8_t wificount = 0;
int dist = 0;

int sitTimerStart = 0;
int sitTimer = 0;
int goneTimer = 0;
int goneTimerStart=0;
int sleepTimer = 0;
int sleepTimerStart=0;

bool sleepButton = false;

void wifi_reconnection(){
  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    }

  while (WiFi.status() != WL_CONNECTED) {
      delay(250);
      if(wificount>15){
          WiFi.disconnect();
          wificount = 0;
      }
      ++wificount;
    }
  }
}

void music_reconnect(){
  if(!sfx.reset()) {
    Serial.println("Not found");
    delay(500);
    MySerial.end();
    delay(250);
    MySerial.begin(9600);
  }
}


struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button1 = {14, 0, false};

void IRAM_ATTR isr() {
  static unsigned long last_interrupt_time = 0;
   unsigned long interrupt_time = millis();
 // If interrupts come faster than 200ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 350)
 {
  button1.numberKeyPresses += 1;
  button1.pressed = true;
 }
 
 last_interrupt_time = interrupt_time;
}



void setup() {

  pinMode(button1.PIN, INPUT_PULLUP);
  attachInterrupt(button1.PIN, isr, FALLING);

  Serial.begin(115200);
  //start the Adafruit Sensor Board
  MySerial.begin(9600);
  music_reconnect();
  Serial.println("SFX board found");
  wifi_reconnection();
}


void loop() {
  wifi_reconnection();
  dist = wifi_dist_read();
  Serial.println(dist);

  flushInput();
  
  char cmd;

  if(dist<=700 && sitTimer>4000 && sitTimer<300000 && sleepButton==false){
    sfx.playTrack((uint8_t)1);
    sitTimer = millis()-sitTimerStart;
    goneTimerStart=millis();
    goneTimer=0;
    cmd = ' ';
  }
  else if(dist<=700 && sitTimer>4000 && sitTimer<300000 && sleepButton==false){
    sfx.playTrack((uint8_t)2);
    goneTimerStart=millis();
    goneTimer=0;
    cmd = ' ';
  }
  else if(dist<=700 && sitTimer<4000){
    sitTimer = millis()-sitTimerStart; 
  }
  else if(dist>700 && goneTimer>4000){
    sitTimerStart = millis();
    sitTimer = 0;
    cmd = '+';
  }
  else if(dist>700 && goneTimer<4000){
    goneTimer = millis()-goneTimerStart;
  }

  if(!sleepButton){
    sleepTimerStart = millis();
    sleepTimer=0;
  }
  else if(sleepButton){
    sleepTimer= millis()-sleepTimerStart;
    if(sleepTimer > 600000){
      sleepButton = false;
    }
  }
  

  Serial.printf("times: GTS = %d ,",goneTimerStart);
  Serial.printf("GT = %d ,",goneTimer);
  Serial.printf("STS = %d ,",sitTimerStart);
  Serial.printf("ST = %d ,",sitTimer);
  Serial.printf("SLS = %d ,",sleepTimerStart);
  Serial.printf("SL = %d ,",sleepTimer);
  Serial.printf("SleepingTrigger = %d ,",sleepButton);
  Serial.printf("CMD is currently : %d ,",cmd);
  Serial.println("");
 
  
  flushInput();
  
  switch (cmd) {
    case 'r': {
      if (!sfx.reset()) {
        Serial.println("Reset failed");
      }
      break; 
    }
    
    case 'L': {
      uint8_t files = sfx.listFiles();
    
      Serial.println("File Listing");
      Serial.println("========================");
      Serial.println();
      Serial.print("Found "); Serial.print(files); Serial.println(" Files");
      for (uint8_t f=0; f<files; f++) {
        Serial.print(f); 
        Serial.print("\tname: "); Serial.print(sfx.fileName(f));
        Serial.print("\tsize: "); Serial.println(sfx.fileSize(f));
      }
      Serial.println("========================");
      break; 
    }
    
    case '#': {
      Serial.print("Enter track #");
      uint8_t n = readnumber();

      Serial.print("\nPlaying track #"); Serial.println(n);
      if (! sfx.playTrack((uint8_t)n) ) {
        Serial.println("Failed to play track?");
      }
      break;
    }
    
    case 'P': {
      Serial.print("Enter track name (full 12 character name!) >");
      char name[20];
      readline(name, 20);

      Serial.print("\nPlaying track \""); Serial.print(name); Serial.print("\"");
      if (! sfx.playTrack(name) ) {
        Serial.println("Failed to play track?");
      }
      break;
   }

   case '+': {
      Serial.println("Vol up...");
      uint16_t v;
      if (! (v = sfx.volUp()) ) {
        Serial.println("Failed to adjust");
      } else {
        Serial.print("Volume: "); Serial.println(v);
      }
      break;
   }

   case '-': {
      Serial.println("Vol down...");
      uint16_t v;
      if (! (v=sfx.volDown()) ) {
        Serial.println("Failed to adjust");
      } else { 
        Serial.print("Volume: "); 
        Serial.println(v);
      }
      break;
   }
   
   case '=': {
      Serial.println("Pausing...");
      if (! sfx.pause() ) Serial.println("Failed to pause");
      break;
   }
   
   case '>': {
      Serial.println("Unpausing...");
      if (! sfx.unpause() ) Serial.println("Failed to unpause");
      break;
   }
   
   case 'q': {
      Serial.println("Stopping...");
      if (! sfx.stop() ) Serial.println("Failed to stop");
      break;
   }  

   case 't': {
      Serial.print("Track time: ");
      uint32_t current, total;
      if (! sfx.trackTime(&current, &total) ) Serial.println("Failed to query");
      Serial.print(current); Serial.println(" seconds");
      break;
   }  

   case 's': {
      Serial.print("Track size (bytes remaining/total): ");
      uint32_t remain, total;
      if (! sfx.trackSize(&remain, &total) ) 
        Serial.println("Failed to query");
      Serial.print(remain); Serial.print("/"); Serial.println(total); 
      break;
    }
  }

  if (button1.pressed) {
    sleepButton=true;
    Serial.println("Sleep mode initiated");
    button1.pressed = false;
  }
}






/************************ MENU HELPERS ***************************/

void flushInput() {
  // Read all available serial input to flush pending data.
  uint16_t timeoutloop = 0;
  while (timeoutloop++ < 40) {
    while(MySerial.available()) {
      MySerial.read();
      timeoutloop = 0;  // If char was received reset the timer
    }
    delay(1);
  }
}

char readBlocking() {
  while (!Serial.available());
  return Serial.read();
}

uint16_t readnumber() {
  uint16_t x = 0;
  char c;
  while (! isdigit(c = readBlocking())) {
    //Serial.print(c);
  }
  Serial.print(c);
  x = c - '0';
  while (isdigit(c = readBlocking())) {
    Serial.print(c);
    x *= 10;
    x += c - '0';
  }
  return x;
}

uint8_t readline(char *buff, uint8_t maxbuff) {
  uint16_t buffidx = 0;
  
  while (true) {
    if (buffidx > maxbuff) {
      break;
    }

    if (Serial.available()) {
      char c =  Serial.read();
      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0) {  // the first 0x0A is ignored
          continue;
        }
        buff[buffidx] = 0;  // null term
        return buffidx;
      }
      buff[buffidx] = c;
      buffidx++;
    }
  }
  buff[buffidx] = 0;  // null term
  return buffidx;
}
/************************ MENU HELPERS ***************************/


int wifi_dist_read(){
   if(WiFi.status() == WL_CONNECTED) {

        HTTPClient http;

        //USE_SERIAL.print("[HTTP] begin...\n");
        /// configure traged server and url
        //http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
        //http.begin("https://www.natedamen.com/_functions/readInteractions"); //HTTP
        http.begin("http://192.168.1.97");
        
        //USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCde will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
           //USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                //Serial.println(payload);
                return payload.toInt();
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
}
