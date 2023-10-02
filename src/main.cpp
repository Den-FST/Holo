#include <Arduino.h>

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "SPIFFS.h"

#include <TFT_eSPI.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <sys/time.h>
#include <Timezone.h>
#include <WebSerial.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <AsyncTCP.h>
#endif

// needed for library
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>         //https://github.com/bblanchon/ArduinoJson

#include <ArduinoOTA.h>

#include <Free_Fonts.h>
#include "digital_760pt7b.h"          // Include the header file attached to this sketch
#include "Open_24_Display_St50pt7b.h" // Include the header file attached to this sketch
// Include the header file that contains the font definition
#include <secret.h>


#define GFXFF 1
// #define FF18 &FreeMono9pt7b

unsigned long previousTime = 0;

TFT_eSPI tft; // Initialize TFT display object
TFT_eSprite img = TFT_eSprite(&tft);

AsyncWebServer server(80);
WiFiClient client;
DNSServer dns;

const char *ntpServer = "pool.ntp.org"; // Time server
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

TimeChangeRule myDST = {"DST", Last, Sun, Mar, 2, 60}; // Daylight Saving Time
TimeChangeRule mySTD = {"STD", Last, Sun, Oct, 3, 0};  // Standard Time

Timezone myTZ(myDST, mySTD);

int xPos = 240;
int tPos = 0;

bool wifimanager = false;

// const char *ssid = "";
// const char *password = "";

void time_sync()
{
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
    delay(500);
  }
}

void set_time()
{

  time_sync();
  // Get the current UNIX timestamp from the time client
  unsigned long epochTime = timeClient.getEpochTime();

  // Set the system time using the UNIX timestamp
  struct timeval tv;
  tv.tv_sec = epochTime;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  WebSerial.println("TimeSet Ok");
}

String sendTime()
{
  String timeString = "";
  // Get the current datetime
  time_t t = time(nullptr);

  // Convert to local time
  TimeChangeRule *tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  int currentSecond = timeinfo->tm_sec;

  // Format the time string
  if (currentHour < 10)
  {
    timeString += "0";
  }
  timeString += String(currentHour);

  timeString += ":";

  if (currentMinute < 10)
  {
    timeString += "0";
  }
  timeString += String(currentMinute);

  timeString += ":";

  if (currentSecond < 10)
  {
    timeString += "0";
  }
  timeString += String(currentSecond);

  return timeString;
}

void printDateTimeTFT()
{
  String scnd;
  String hour;
  String minutes;
  // Get the current datetime
  time_t t = time(nullptr);

  // Convert to local time
  TimeChangeRule *tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  if (timeinfo->tm_hour < 10)
  {
    hour = "0" + String(timeinfo->tm_hour);
  }
  else
  {
    hour = String(timeinfo->tm_hour);
  }

  if (timeinfo->tm_min < 10)
  {
    minutes = "0" + String(timeinfo->tm_min);
  }
  else
  {
    minutes = String(timeinfo->tm_min);
  }

  String HourMins = hour + ":" + minutes;

  if (timeinfo->tm_sec < 10)
  {
    scnd = "0" + String(timeinfo->tm_sec);
  }
  else
  {
    scnd = String(timeinfo->tm_sec);
  }

  img.fillSprite(TFT_BLACK);
  img.setFreeFont(&digital_760pt7b);
  img.setTextSize(1);
  img.setTextDatum(4);

  img.setTextColor(TFT_CYAN, TFT_BLACK);
  // img.drawNumber(hour.toInt(), 120, img.fontHeight(FONT7) - 10);
  img.drawString(HourMins, 120, img.fontHeight(FONT7) );
  // img.setTextDatum(4);
  img.setTextColor(TFT_MAGENTA, TFT_BLACK);
  img.drawString(scnd, 120, img.fontHeight(FONT7) + 100 );

  // img.drawNumber(scnd.toInt(), 120, img.fontHeight(FONT7) + 100);
  img.pushSprite(0, 0);

  // // img.fillSprite(TFT_BLACK);
  // img.setFreeFont(&digital_760pt7b);
  // img.setTextSize(1);

  // // img.setFreeFont(&FreeSansBold12pt7b);
  // // img.setTextSize(3);
  // img.setTextColor(  TFT_CYAN, TFT_BLACK);
  // img.drawString(HourMins, 35, 5 );

  // img.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  // img.setTextSize(2);
  // img.drawString(scnd, 50, 100,7);

  // img.pushSprite(0, 0, TFT_TRANSPARENT);

  // // printTFT(0, (TFT_HEIGHT / 2.2), HourMins, FSS9, TFT_CYAN, 5, 1);
  // printTFT(TFT_WIDTH / 3.8, (TFT_HEIGHT / 1.2), scnd, FSS9, TFT_GREENYELLOW, 5, 1);
  delay(20);
}

void printDateTime()
{
  // Get the current datetime
  time_t t = time(nullptr);

  // Convert to local time
  TimeChangeRule *tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  // Print the datetime
  WebSerial.print("Current datetime: ");
  WebSerial.print(timeinfo->tm_year + 1900);
  WebSerial.print("-");
  WebSerial.print(timeinfo->tm_mon + 1);
  WebSerial.print("-");
  WebSerial.print(timeinfo->tm_mday);
  WebSerial.print(" ");
  WebSerial.print(timeinfo->tm_hour);
  // printTFT(20, 128, String(timeinfo->tm_hour), FF18, TFT_WHITE, 6, 1);
  WebSerial.print(":");
  // printTFT(50, 128, ":", FF18, TFT_WHITE, 6, 1);
  WebSerial.print(timeinfo->tm_min);
  // printTFT(60, 128, String(timeinfo->tm_min), FF18, TFT_WHITE, 6, 1);
  WebSerial.print(":");
  WebSerial.print(timeinfo->tm_sec);

  int dayOfWeek = timeinfo->tm_wday;
  WebSerial.print("Day of Week: ");
  WebSerial.println(dayOfWeek);
  switch (dayOfWeek)
  {
  case 0:
    WebSerial.println(" - Sunday");
    break;
  case 1:
    WebSerial.println(" - Monday");
    break;
  case 2:
    WebSerial.println(" - Tuesday");
    break;
  case 3:
    WebSerial.println(" - Wednesday");
    break;
  case 4:
    WebSerial.println(" - Thursday");
    break;
  case 5:
    WebSerial.println(" - Friday");
    break;
  case 6:
    WebSerial.println(" - Saturday");
    break;
  default:
    WebSerial.println(" - Unknown day of the week");
    break;
  }
}

void recvMsg(uint8_t *data, size_t len)
{
  // WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  WebSerial.println(d);

  if (d == "gettime")
  {
    time_sync();
    WebSerial.println(timeClient.getFormattedTime());
  }
  else if (d == "settime")
  {
    set_time();
  }
  else if (d == "systime")
  {
    printDateTime();
  }
  else if (d == "time")
  {
    String t = timeClient.getFormattedTime();
    WebSerial.println(t);
  }
  else if (d == "cmd")
  {

    WebSerial.print("-- Get time from NTP server : ");
    WebSerial.println("gettime");
    WebSerial.print("-- Get time from NTP and sync with local time : ");
    WebSerial.println("settime");
    WebSerial.print("-- Get system time : ");
    WebSerial.println("systime");
    WebSerial.print("-- Show the time from server : ");
    WebSerial.println("time");
  }
}

// flag for saving data
bool shouldSaveConfig = false;

// callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  tft.begin();
  // img.createSprite(239, 239);
  // tft.writecommand(TFT_MADCTL);
  // tft.writedata(TFT_MAD_MX);

  tft.setRotation(4); // Special Flip Command in ST7789_Rotation.h  writedata( TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
  tft.fillScreen(TFT_BLACK);

  // tft.fillScreen(TFT_BLACK); // Clear the screen
  // tft.setSwapBytes(true);



  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 50);
  tft.println("Started");

  if (SPIFFS.begin())
  {
    // Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      // file exists, reading and loading
      Serial.println("reading config file");
      tft.println("reading config file");
      tft.println(" ");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("\nparsed json");

          // strcpy(fx_server, json["fx_server"]);
          // strcpy(fx_port, json["fx_port"]);
          // strcpy(fx_server2, json["fx_server2"]);
          // strcpy(fx_port2, json["fx_port2"]);

          // tft.setTextColor(TFT_YELLOW);
          // tft.print("Server: ");
          // tft.println(fx_server);
          // tft.print("Port: ");
          // tft.println(fx_port);
          // tft.println(" ");
        }
        else
        {
          Serial.println("failed to load json config");
          tft.println("failed to load json config");
        }
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
    tft.println("failed to mount FS");
    SPIFFS.format(); // Format SPIFFS and all data

    tft.println("Open FX_AP with pass: Future2050");
  }
  if (wifimanager == true)
  {
    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    // AsyncWiFiManagerParameter custom_fx_server("server", "fx server", fx_server, 40);
    // AsyncWiFiManagerParameter custom_fx_port("port", "fx port", fx_port, 5);
    // AsyncWiFiManagerParameter custom_fx_server2("server2", "fx2 server", fx_server2, 40);
    // AsyncWiFiManagerParameter custom_fx_port2("port2", "fx2 port", fx_port2, 5);

    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    AsyncWiFiManager wifiManager(&server, &dns);

    // set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    // add all your parameters here
    // wifiManager.addParameter(&custom_fx_server);
    // wifiManager.addParameter(&custom_fx_port);

    // wifiManager.addParameter(&custom_fx_server2);
    // wifiManager.addParameter(&custom_fx_port);

    // reset settings - for testing
    //  wifiManager.resetSettings();

    // sets timeout until configuration portal gets turned off
    // useful to make it all retry or go to sleep
    // in seconds
    //  wifiManager.setTimeout(120);

    // tft.println("Open FX_AP with pass: Future2050");
    // tft.println(wifiManager.getConfigPortalSSID());

    // fetches ssid and pass and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"

    // and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("FX_AP", "Future2050"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
  else
  {

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  tft.setTextColor(TFT_GREEN);
  tft.println("Connected");
  tft.setTextColor(TFT_WHITE);
  // read updated parameters
  // strcpy(fx_server, custom_fx_server.getValue());
  // strcpy(fx_port, custom_fx_port.getValue());
  // strcpy(fx_server2, custom_fx_server2.getValue());
  // strcpy(fx_port2, custom_fx_port2.getValue());

  // save the custom parameters to FS
  if (shouldSaveConfig)
  {
    Serial.println("saving config");
    tft.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    // json["fx_server"] = fx_server;
    // json["fx_port"] = fx_port;
    // json["fx_server2"] = fx_server2;
    // json["fx_port2"] = fx_port2;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("failed to open config file for writing");
      tft.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    // end save
  }

  // tft.println(wifiManager.getConfiguredSTASSID());

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());

  tft.print("Local ip: ");
  tft.setTextColor(TFT_BLUE);
  tft.println(WiFi.localIP());
  tft.setTextColor(TFT_WHITE);
  tft.println(" ");
  // Serial.print("FX Server 1: ");
  // Serial.println(fx_server);
  // Serial.print("FX port 1: ");
  // Serial.println(fx_port);
  // Serial.print("FX Server 2: ");
  // Serial.println(fx_server2);
  // Serial.print("FX port 2: ");
  // Serial.println(fx_port2);

  // strcpy(fx_server_con, fx_server);
  // strcpy(fx_port_con, fx_port);

  delay(500);

  ArduinoOTA.setHostname("FX-ota");
  ArduinoOTA.begin();

  WebSerial.begin(&server);
  /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);
  server.begin();
  delay(1000);
  timeClient.begin();
  delay(100);
  set_time();
  delay(100);

    img.createSprite(235, 235);
  img.setSwapBytes(true);
  img.setTextWrap(true);
  img.fillSprite(TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
}

void loop()
{

  String HourMins;

  unsigned long currentTime = millis(); // Get the current time

  if (currentTime - previousTime >= 1000)
  {

    printDateTimeTFT();
    previousTime = currentTime; // Update the previous time
  }

  // String text = "Privet Volodya!";
  // int textWidth = tft.textWidth(text);
  // // tft.fillScreen(TFT_BLACK); // Clear the screen
  // tft.setTextSize(3);
  // tft.fillRect(0, (TFT_HEIGHT / 1.3), TFT_HEIGHT, 40, TFT_BLACK);
  // tft.setCursor(xPos - textWidth , (TFT_HEIGHT / 1.3)); //- tft.fontHeight())
  // tft.setTextColor(TFT_WHITE);
  // tft.print(text);

  // xPos--; // Decrement the X position for scrolling effect

  // Serial.print("Xpos ");
  // Serial.println(xPos);

  // if (xPos <  -50) {
  //   xPos = 240; // Reset the X position
  // }

  // delay(20); // Adjust the delay as needed to control the scrolling speed

  // tft.setFreeFont(FSS9);
  // tft.fillRect(0, (TFT_HEIGHT / 1.9), 20, 40, TFT_BLACK);
  // tft.setTextColor(TFT_WHITE);
  // tft.setTextSize(5);
  // tft.setCursor(0, (TFT_HEIGHT / 1.9));
  // tft.print(HourMins);
}
