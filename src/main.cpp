#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include "arduino_secrets.h" // Include the file with the WiFi credentials

TFT_eSPI tft = TFT_eSPI(); // Create TFT object
AsyncWebServer server(80); // Create AsyncWebServer object on port 80

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

String name = "";

void setup()
{
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_BLACK);

  // Connect to WiFi
  tft.setCursor(0, 0);
  tft.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    tft.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Connected to WiFi!");
    tft.println(WiFi.localIP());
  }
  else
  {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("No network");
    return;
  }

  // Serve HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", "<form action=\"/get\" method=\"GET\"><input type=\"text\" name=\"name\"><input type=\"submit\" value=\"Save\"></form>"); });

  // Handle form submission
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("name")) {
      name = request->getParam("name")->value();
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("Name:");
      tft.println(name);
    }
    request->send(200, "text/html", "<p>Name saved!</p><a href=\"/\">Go back</a>"); });

  server.begin();
}

void loop()
{
  // Nothing to do here
}