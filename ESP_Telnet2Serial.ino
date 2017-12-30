/*--------------------------------------------------
Telnet to Serial AccessPoint for ESP8266 
for ESP8266 adapted Arduino IDE

by Stefan Thesen 08/2015 - free for anyone
http://blog.thesen.eu

Creates an accesspoint or network client which can 
be connected by telnet; e.g. telnet 192.168.4.1
Telnet input is sent to serial and vice versa.

Serial output can e.g. be used to steer an attached
Arduino or other serial interfaces.
Please take care for levels of the serial lines.

Code inspired by a post of ghost on github:
https://github.com/esp8266/Arduino/issues/307
--------------------------------------------------*/

#include <ESP8266WiFi.h>

////////////////////////////////////
// settings for Telnet2Serial Bridge
////////////////////////////////////

// max number of clients that can connect
#define MAX_NO_CLIENTS 1
const WiFiMode wifi_mode = WIFI_AP;     // set WIFI_AP for access-point or WIFI_STA for WIFI client
const char* ssid = "schopl-door";   // Name of AP (for WIFI_AP) or name of Wifi to connect to (for WIFI_STA)
const char* password = "sesamoeffnedich";      // set to "" for open access point w/o password
const int iSerialSpeed = 57600;          // speed of the serial connection
const bool bSuppressLocalEcho = false;   // shall local echo in telnet be suppressed (usually yes)



// Create an instance of the server on Port 23
WiFiServer server(23);
WiFiClient pClientList[MAX_NO_CLIENTS]; 

void setup() 
{
  // start serial
  Serial.begin(iSerialSpeed, SERIAL_8N1);

  if (wifi_mode == WIFI_AP)
  {
    // AP mode
    WiFi.mode(wifi_mode);
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.begin();
    server.setNoDelay(false);
  }
  else
  {
    // network cient - inital connect
    WiFi.mode(WIFI_STA);
    WiFiStart();
  }
}


void WiFiStart()
{ 
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
  }
  
  // Start the server
  server.begin();
  server.setNoDelay(true);
}


void loop() 
{ 
  int ii;

  ////////////////////////////////////////////////
  // if network client: check if WLAN is connected
  ////////////////////////////////////////////////
  if ((wifi_mode == WIFI_STA) && (WiFi.status() != WL_CONNECTED))
  {
    WiFiStart();
  }

  /////////////////////
  // handle new clients
  /////////////////////
  if (server.hasClient())
  {
    bool bFoundPlace=false;
    
    // search a free spot
    for(ii = 0; ii < MAX_NO_CLIENTS; ii++)
    {
      if (!pClientList[ii] || !pClientList[ii].connected())
      {
        // remove old connections
        if(pClientList[ii]) 
        {
          pClientList[ii].stop();
        }

        // new client
        pClientList[ii] = server.available();
        if (bSuppressLocalEcho) 
        { 
          pClientList[ii].write("\xFF\xFB\x01", 3); 
        }
        pClientList[ii].write("Welcome to Telnet2Serial Adapter\r\n");
        pClientList[ii].write("S. Thesen 08/2015 - https://blog.thesen.eu\r\n"); 
        bFoundPlace=true;
        break;
      }
    }

    //no free spot --> sorry
    if (!bFoundPlace)
    {
      WiFiClient client = server.available();
      client.stop();
    }
  }

  /////////////////////
  // Telnet --> Serial
  /////////////////////

  bool lineDone=false;
  bool serialSend=false;
  
  for(ii = 0; ii < MAX_NO_CLIENTS; ii++)
  {
    if (pClientList[ii] && pClientList[ii].connected())
    {
        if(pClientList[ii].available()) 
        {
          char cc = pClientList[ii].read();
          Serial.write(cc);
          serialSend=true;
          //yield();
          // after NL process serial responses first
          if (cc == 13) {
            lineDone=true;
            //break;
          }
        }
    }
  }

  /////////////////////
  // Serial --> Telnet
  /////////////////////
  if (serialSend || Serial.available()) {
    serialSend=false;
    unsigned long lastSerial = millis();
    uint8_t waitForSilence = (lineDone ? 50 : 2);
    lineDone=false;
    // continue with reading telnet earliest after 50 ms silence on Serial
    uint8_t sbuf[256];
    size_t fill=0;
    while (millis() - lastSerial <= waitForSilence) {      
      while(Serial.available())
      {
        size_t len = Serial.available();
        if (len+fill > 256) len=256-fill;
        Serial.readBytes(&sbuf[fill], len);
        fill+=len;
        lastSerial = millis();
        if (fill==256) {
          for(ii = 0; ii < MAX_NO_CLIENTS; ii++)
          {
            if (pClientList[ii] && pClientList[ii].connected())
            {
              pClientList[ii].write(&sbuf[0], fill);
              delay(1);
            }
          }
          fill=0;
        }
      }
      yield(); // busy wait for silence
    }
    if (fill>0) {
      for(ii = 0; ii < MAX_NO_CLIENTS; ii++)
      {
        if (pClientList[ii] && pClientList[ii].connected())
        {
          pClientList[ii].write(&sbuf[0], fill);
          delay(1);
        }
      }
      fill=0;
    }
    
  }

}
