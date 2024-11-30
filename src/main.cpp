#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>

#include "arduino_secrets.h" // Include the file with the WiFi credentials
#include "object.h"          // Include the Player class

TFT_eSPI tft = TFT_eSPI(); // Create TFT object
AsyncWebServer server(80); // Create AsyncWebServer object on port 80

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

String name = "";
String characterType = "";

void listSPIFFSFiles()
{
  fs::File root = SPIFFS.open("/");
  fs::File file = root.openNextFile();
  while (file)
  {
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

void clearDispaly()
{
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
}

void setup()
{
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("Initializing...");

  delay(1000);
  clearDispaly();

  Serial.println("Initializing SPIFFS...");
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    tft.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  tft.println("SPIFFS mounted");

  listSPIFFSFiles(); // List files in SPIFFS to verify upload

  delay(1000);
  clearDispaly();

  Serial.println("Connecting to WiFi...");
  tft.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    tft.print(".");
    wifi_attempts++;
    if (wifi_attempts > 20)
    { // Timeout after 20 seconds
      Serial.println("Failed to connect to WiFi");
      tft.println("Failed to connect to WiFi");
      return;
    }
  }
  Serial.println("Connected to WiFi");
  tft.println("Connected to WiFi");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Serving /index.html");
    request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/submit", HTTP_POST, [&](AsyncWebServerRequest *request)
            {
    if (request->hasParam("name", true) && request->hasParam("characterType", true)) {
      name = request->getParam("name", true)->value();
      characterType = request->getParam("characterType", true)->value();
      
      std::vector<std::string> items = {"Sword", "Shield"};
      std::vector<std::string> creatures = {"Dragon", "Phoenix"};
      Player player(name.c_str(), "MainCreature", characterType.c_str(), items, creatures);

      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println(player.toString().c_str());

      Serial.println("Player created:");
      Serial.println(player.toString().c_str());
    } else {
      Serial.println("Missing parameters");
      tft.println("Missing parameters");
    }
    request->send(200, "text/plain", "Player created"); });

  server.begin();
  Serial.println("HTTP server started");
  tft.println("HTTP server started");
}

void loop()
{
  // Nothing to do here
}