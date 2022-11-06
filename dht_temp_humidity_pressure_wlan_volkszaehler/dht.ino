/* This code contains functions from Conrad, when they used a similar
 * board to what I used for an Advent Calendar project. The code is still
 * available here: http://iot.fkainka.de/wp-content/uploads/2015/09/Day19_ThinkSpeak1.zip
 * though I have no idea whether it was ever licensed for redistribution etc.
 * 
 * For the sensor stuff, this page is very helpful:
 * https://support.sodaq.com/Boards/Autonomo/Examples/tph/
 * 
 * and you will see that some of the code is based on their example.
 *
 * You need to install the Sodaq BMP085 and SHT2x libraries.
 * Note however, that for my code, the BMP085 stuff isn't actually used.
 * 
 * As far as any code here that I have contributed is concerned, an MIT license applies.
 *
 * More details on this project: https://www.mattbirgit.de/diy-temp-hum.html
 *
 */

#include <Wire.h>
#include <Sodaq_BMP085.h>
#include <Sodaq_SHT2x.h>

#define SSID "PUT YOUR SSID HERE - USE 2.4GHz/802.11b/g"
#define PASSWORD "WLAN PASSWORD HERE"

#define SENSOR A0

#define LED_WLAN 13

#define DEBUG true

#include <SoftwareSerial.h>
SoftwareSerial esp8266(11, 12); // RX, TX

//TPH BMP sensor
Sodaq_BMP085 bmp;

void setup() {  
    Serial.begin(19200);
    Serial.println("Setting up sensors...");
    setupTPH();
    Serial.println("Setting up ESP8266");
    esp8266.begin(19200);
    
}

void loop() {
    Serial.println("Trying to connect to WLAN...");
    while (!espConfig()) {
      Serial.println("Could not connect to WLAN, waiting 20s then trying again...");
      delay(20000);
    }
    digitalWrite(LED_WLAN, HIGH);
    Serial.println("Taking reading...");
    int reading = getTPHTemp();  
    Serial.println(String(reading) + "°C");
    if (sendVolkszaehler("VZ SERVER HERE, e.g. 192.168.0.111", "CHANNEL UUID FOR TEMPERATURE HERE", reading)) debug("Sent Temp to VZ");
    reading = getTPHHum();  
    Serial.println(String(reading) + "%");
    if (sendVolkszaehler("VZ SERVER HERE, e.g. 192.168.0.111", "CHANNEL UUID FOR HUMIDITY HERE", reading)) debug("Sent Humidity to VZ, will now wait about 4min");
    debug("Disconnecting WLAN");
    if (disconnectStation()) {
        digitalWrite(LED_WLAN, LOW);
    }
    delay(120000);
}

void setupTPH(){  
    //Initialise the wire protocol for the TPH sensors  
    Wire.begin();  
    //Initialise the TPH BMP sensor  
    bmp.begin();
}
int getTPHTemp(){  
    return round(SHT2x.GetTemperature());
    // Start working with these lines if you also want the data from the BMP180:
    //data += String(bmp.readTemperature()) + ", ";  
    //data += String(bmp.readPressure() / 100)  + ", ";  
}
int getTPHHum(){  
    return round(SHT2x.GetHumidity());
}

//------------ VZ functions, derived from ThingsSpeak Functions from the Conrad code ---------

boolean sendVolkszaehler(String host, String channel, int value)
{
  boolean success = true;
  String msg = "{\"value\" : " + String(value) + "}";
  success &= sendCom("AT+CIPSTART=\"TCP\",\"" + host + "\",80", "OK");

  String getRequest = createVZGet(host, "/middleware/data/" + channel + ".json?operation=add&value=" + String(value));

  if (sendCom("AT+CIPSEND=" + String(getRequest.length()), ">"))
  {
    esp8266.print(getRequest);
    Serial.println(getRequest);
    esp8266.find("SEND OK");
    if (!esp8266.find("CLOSED")) success &= sendCom("AT+CIPCLOSE", "OK");
  }
  else
  {
    success = false;
  }
  return success;
}  

String createVZGet(String host, String path)
{
  //Don't close the connection because ESP routine does it for us from our side
  String xBuffer = "GET *PATH* HTTP/1.1\r\nHost: *HOST*\r\n\r\n";
  xBuffer.replace("*HOST*", host);
  xBuffer.replace("*PATH*", path);

  return xBuffer;
}

//-----------------------------------------Config ESP8266------------------------------------

boolean espConfig()
{
  boolean succes = true;
  esp8266.setTimeout(5000);
  succes &= sendCom("AT+RST", "ready");
  esp8266.setTimeout(1000);

  if (configStation(SSID, PASSWORD)) {
    succes &= true;
    debug("WLAN Connected");
    debug("My IP is:");
    debug(sendCom("AT+CIFSR"));
  }
  else
  {
    succes &= false;
  }
  //shorter Timeout for faster wrong UPD-Comands handling
  succes &= sendCom("AT+CIPMUX=0", "OK");
  succes &= sendCom("AT+CIPMODE=0", "OK");

  return succes;
}

boolean configTCPServer()
{
  boolean succes = true;

  succes &= (sendCom("AT+CIPMUX=1", "OK"));
  succes &= (sendCom("AT+CIPSERVER=1,80", "OK"));

  return succes;

}

boolean configTCPClient()
{
  boolean succes = true;

  succes &= (sendCom("AT+CIPMUX=0", "OK"));
  //succes &= (sendCom("AT+CIPSERVER=1,80", "OK"));

  return succes;

}


boolean configStation(String vSSID, String vPASSWORT)
{
  boolean succes = true;
  succes &= (sendCom("AT+CWMODE=1", "OK"));
  esp8266.setTimeout(20000);
  succes &= (sendCom("AT+CWJAP=\"" + String(vSSID) + "\",\"" + String(vPASSWORT) + "\"", "OK"));
  esp8266.setTimeout(1000);
  return succes;
}

boolean disconnectStation()
{
  boolean succes = true;
  succes &= (sendCom("AT+CWQAP", "OK"));
  esp8266.setTimeout(1000);
  return succes;
}


//-----------------------------------------------Controll ESP-----------------------------------------------------



boolean sendCom(String command, char respond[])
{
  esp8266.println(command);
  if (esp8266.findUntil(respond, "ERROR"))
  {
    return true;
  }
  else
  {
    debug("ESP SEND ERROR: " + command);
    return false;
  }
}

String sendCom(String command)
{
  esp8266.println(command);
  return esp8266.readString();
}



//-------------------------------------------------Debug Functions------------------------------------------------------
void serialDebug() {
  while (true)
  {
    if (esp8266.available())
      Serial.write(esp8266.read());
    if (Serial.available())
      esp8266.write(Serial.read());
  }
}

void debug(String Msg)
{
  if (DEBUG)
  {
    Serial.println(Msg);
  }
}
