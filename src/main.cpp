#include <Arduino.h>
#include <TFT_eSPI.h>       // Include the TFT library
#include <SPI.h>            // Include the SPI library
#include <HardwareSerial.h> // Include the HardwareSerial library
#include <string>
// #include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "arduino_secrets.h" // Include the file with the WiFi credentials
#include "displayFunctions.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

HardwareSerial mySerial(1); // Use the second hardware serial port

// volatile bool buttonPressed = false;
volatile int buttonValue = -1; // Global variable to hold the button value

const char *serverName = "https://gameapi-2e9bb6e38339.herokuapp.com/api/v1/resources";

void whatAnimal();
void savePlayerData();

void getPlayerData();

void yesButtonPressed()
{
  buttonValue = 1;
}

void noButtonPressed()
{
  buttonValue = 0;
}

const int yesButtonPin = 35; // Button1 on the TTGO T-Display
const int noButtonPin = 0;   // Button2 on the TTGO T-Display

void clearScreen()
{
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
}

void connectToNetwork()
{
  WiFi.begin(ssid, pass); // Connect to the network
  delay(1000);            // Wait for 1 second

  while (WiFi.status() != WL_CONNECTED)
  {
    clearScreen();
    tft.println("Connecting to WiFi...");
    // delay(1000); // Wait for 1 second
  }
  clearScreen();
  tft.println("Connected to WiFi!");
  delay(500); // Wait for 1 second
  getPlayerData();
  delay(2000); // Wait for 10 seconds before next request
}

std::pair<std::string, int> extractWordAndNumberString(const std::string &str)
{
  std::string startDelimiter = "11--";
  std::string endDelimiter = "--11";

  // Find the start and end positions of the word
  size_t startPos = str.find(startDelimiter);
  size_t endPos = str.find(endDelimiter);

  // If the start or end delimiter was not found, return an empty string and 0
  if (startPos == std::string::npos || endPos == std::string::npos)
  {
    return {"", 0};
  }

  // Adjust the start position to be after the start delimiter
  startPos += startDelimiter.length();

  // Calculate the length of the word
  size_t length = endPos - startPos;

  // Extract the word
  std::string wordAndNumber = str.substr(startPos, length);

  // Split the word and number at the full stop
  size_t dotPos = wordAndNumber.find('.');
  if (dotPos == std::string::npos)
  {
    return {"", 0}; // Return an empty string and 0 if the full stop was not found
  }

  std::string word = wordAndNumber.substr(0, dotPos);
  int number = std::stoi(wordAndNumber.substr(dotPos + 1));

  return {word, number};
}

int buttonConfirm()
{
  attachInterrupt(digitalPinToInterrupt(yesButtonPin), yesButtonPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(noButtonPin), noButtonPressed, FALLING);

  // Wait for a button press
  while (buttonValue == -1)
  {
    delay(100); // Optional delay to prevent the loop from running too fast
  }

  int returnValue = buttonValue; // Save the button value

  // Reset the button value
  buttonValue = -1;

  detachInterrupt(digitalPinToInterrupt(yesButtonPin));
  detachInterrupt(digitalPinToInterrupt(noButtonPin));

  return returnValue; // Return the button value
}

void setup()
{
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, 27, 26); // Initialize serial communication on pins 27 (RX) and 26 (TX)
  tft.init();                               // Initialize the TFT display
  clearScreen();
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);

  // Initialize the button pins as input
  pinMode(yesButtonPin, INPUT_PULLUP);
  pinMode(noButtonPin, INPUT_PULLUP);

  String word = "";
  scan4challange();

  // client.setInsecure(); // Use this to disable certificate checking
}

void loop()
{
  if (mySerial.available())
  {

    String barcode = mySerial.readString();
    if (!barcode.isEmpty())
    {

      std::string extractWord(const std::string &str);

      tft.setCursor(0, 0);

      std::string myText = barcode.c_str(); // Convert Arduino String to std::string

      std::pair<std::string, int> result = extractWordAndNumberString(myText);

      std::string word = result.first;
      int number = result.second;

      if (word == "challenge")
      {
        displayCircle();
        tft.setCursor(0, 0);
        tft.println("challenge \n card loaded \n" + number);
      }
      else if (word == "creatureCapture")
      {
        clearScreen();
        tft.println("Seek Creature\n to be your");
        whatAnimal();
      }
      else
      {
        displayX();
        const std::string message = "Invalid\nscan: " + word;
        displayErrorMessage(message);
        delay(1000); // Wait for 2 seconds
        scan4challange();
      }
    }
    else
    {
      Serial.println("Empty barcode");
      const std::string message = "Empty barcode: + barcode.c_str()";
      displayErrorMessage(message);
    }
  }
}

void whatAnimal()
{
  tft.println(" champion");
  delay(1000); // Wait for 1 seconds

  String input = ""; // Declare the variable "input"

  while (!mySerial.available())
  {
    // wait for data to be available
    delay(100); // optional delay to prevent the loop from running too fast
    animateEyes();
  }

  // now read from Serial
  input = mySerial.readString();

  String word = "";
  clearScreen();

  std::string myText = input.c_str(); // Convert Arduino String to std::string

  std::pair<std::string, int> result = extractWordAndNumberString(myText);

  std::string animal = result.first;
  int number = result.second;

  if (animal == "babyTrex")
  {
    displayTrex();
    tft.setCursor(0, 0);
    tft.println("Keep T-rex\nas your\nchampion");
    delay(2000); // Wait for 2 seconds
    buttonReadText();
    buttonConfirm();

    if (buttonConfirm() == 1)
    {
      clearScreen();
      tft.println("Return to\ntavern to\nkeep\ncreature");
      delay(1000); // Wait for 2 seconds
      connectToNetwork();
    }
    else
    {
      clearScreen();
      tft.println("\nPick another\ncreature");
      delay(1000); // Wait for 2 seconds
      return;
    }
  }

  else
  {
    tft.println("Invalid input: " + input);
    scan4challange();
    return;
  }
}

void getPlayerData()
{
  if (WiFi.status() == WL_CONNECTED)
  { // Check WiFi connection status

    WiFiClientSecure client;

    String serverNameString = String(serverName); // Convert serverName to String type

    if (!client.connect(serverNameString.c_str(), 443))
    {
      Serial.println("Connection to server failed");
      return;
    }

    client.println("GET /api/v1/resources HTTP/1.1");
    client.println("Host: " + serverNameString); // Use serverNameString instead of String(serverName)
    client.println("Connection: close");
    client.println();

    while (client.connected())
    {
      String line = client.readStringUntil('\n');
      if (line == "\r")
      {
        break;
      }
    }

    String httpResponse = client.readStringUntil('\n');
    int httpResponseCode = httpResponse.toInt();

    if (httpResponseCode >= 200 && httpResponseCode < 300)
    {
      String response = client.readStringUntil('\r');
      clearScreen();
      tft.println(httpResponseCode);
      tft.println(response);
    }
    else
    {
      clearScreen();
      tft.print("Error on sending GET: ");
      tft.println(httpResponseCode);
    }
  }
  else
  {
    Serial.println("Not connected to WiFi");
  }

  delay(5000); // Wait for 5 seconds before next loop
}

/*
void savePlayerData()
{
  WiFiClientSecure client;
  if (WiFi.status() == WL_CONNECTED)
  {

    WiFiClient client;
    HttpClient http(client, serverName);

    String serverPath = "/api/v1/resources";

    DynamicJsonDocument doc(200);
    doc["player_name"] = "Player1";
    doc["creature"] = "Dragon";
    doc["powerLevel"] = 9000;

    String requestBody;
    serializeJson(doc, requestBody);

    http.beginRequest();
    http.post(serverPath);
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Content-Length", requestBody.length());
    http.beginBody();
    http.print(requestBody);
    http.endRequest();

    int httpResponseCode = http.responseStatusCode();

    if (httpResponseCode >= 200 && httpResponseCode < 300)
    {
      String response = http.responseBody();
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println(httpResponseCode);
      tft.println(response);
    }
    else
    {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.print("Error on sending POST: ");
      tft.println(httpResponseCode);
    }
  }

  delay(5000); // Wait for 10 seconds before next loop
}
*/