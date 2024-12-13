#include <WiFi.h>
#include "WiFiSetup.h"

void setupWiFi(const char *ssid, const char *pass)
{
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
}