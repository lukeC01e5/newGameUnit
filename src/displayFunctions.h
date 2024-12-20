
#include <TFT_eSPI.h> // Include the TFT_eSPI library

TFT_eSPI tft; // Create an instance of the TFT class

void displayTrex()
{
  tft.fillScreen(TFT_BLACK); // Clear the screen

  int16_t x = tft.width() / 2;                   // Calculate the x coordinate of the center of the screen
  int16_t y = tft.height() / 2;                  // Calculate the y coordinate of the center of the screen
  tft.pushImage(x - 30, y - 50, 128, 128, Trex); // Draw the T-Rex image at the center of the screen

  delay(3000);               // Wait for 3 seconds
  tft.fillScreen(TFT_BLACK); // Clear the screen
  return;
}

void scan4challange()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  // tft.setCursor(0, tft.getCursorY() + 10); // Move the cursor down
  tft.setCursor(0, 0);
  tft.println("\nScan\nchallange\ncard.....");
}

void displayX()
{
  tft.fillScreen(TFT_BLACK); // Clear the screen

  tft.setTextDatum(MC_DATUM); // Set the datum to be the center of the screen
  tft.setTextSize(10);        // Set the text size to 5
  tft.setTextColor(TFT_RED);  // Set the text color to red

  int16_t x = tft.width() / 2;  // Calculate the x coordinate of the center of the screen
  int16_t y = tft.height() / 2; // Calculate the y coordinate of the center of the screen

  tft.drawString("X", x, y); // Draw the 'X' at the center of the screen

  delay(1000); // Wait for 3 seconds

  tft.fillScreen(TFT_BLACK); // Clear the screen
}

void displayCircle()
{
  tft.fillScreen(TFT_BLACK); // Clear the screen

  int16_t x = tft.width() / 2;  // Calculate the x coordinate of the center of the screen
  int16_t y = tft.height() / 2; // Calculate the y coordinate of the center of the screen
  int16_t radius = 50 / 2;      // Calculate the radius of the circle

  tft.fillCircle(x, y, radius, TFT_GREEN); // Draw the green circle

  delay(1000); // Wait for 3 seconds

  tft.fillScreen(TFT_BLACK); // Clear the screen
}

void animateEyes()
{
  int16_t x = tft.width() / 2;       // Calculate the x coordinate of the center of the screen
  int16_t y = tft.height() / 2 + 40; // Calculate the y coordinate of the center of the screen
  int16_t radius = 50 / 2;           // Calculate the radius of the circle
  int16_t pupilRadius = radius / 2;  // Calculate the radius of the pupil

  // Draw the white circles
  tft.fillCircle(x - radius - 10, y, radius, TFT_WHITE);
  tft.fillCircle(x + radius + 10, y, radius, TFT_WHITE);

  for (int i = -radius + pupilRadius + 2; i <= radius - pupilRadius - 2; i += 5)
  {
    // Clear the previous pupils
    tft.fillCircle(x - radius - 10 + i - 5, y, pupilRadius, TFT_WHITE);
    tft.fillCircle(x + radius + 10 + i - 5, y, pupilRadius, TFT_WHITE);

    // Draw the new pupils
    tft.fillCircle(x - radius - 10 + i, y, pupilRadius, TFT_BLACK);
    tft.fillCircle(x + radius + 10 + i, y, pupilRadius, TFT_BLACK);

    delay(200); // Wait for a short time
  }
}

void buttonReadText()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.println("\n      yes--->\n\n\n      no--->");
}

void displayErrorMessage(const std::string &message)
{
  tft.fillScreen(TFT_BLACK); // Clear the screen

  tft.setTextSize(3);
  tft.setTextColor(TFT_RED);
  tft.setCursor(0, 0);
  tft.println(("\nError:\n" + message).c_str());

  delay(1000); // Wait for 3 seconds
  return;
}