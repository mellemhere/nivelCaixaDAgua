#include <Arduino.h>
#include <Wire.h>
#include "WiFi.h"
#include "VL53L0X.h"
#include "LiquidCrystal.h"
#include "HTTPClient.h"

const char *ssid = "Wireless13 - 2.4GHz";
const char *password = "Happy13family";
const char *serverName = "http://mellemhere.com/caixadeagua.php";

unsigned long timerDelay = 5000;
unsigned long lastTime = 0;


#define D4 32
#define D5 33
#define D6 25
#define D7 26
#define RS 27
#define EN 14

#define BTN 15
#define BL  2

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
VL53L0X sensor;

/*
 *  Barras de progresso
 */
byte bar1[8] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
byte bar2[8] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
byte bar3[8] = {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C};
byte bar4[8] = {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E};
byte bar5[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

void (*resetFunc)() = nullptr;

void setup() {

    pinMode(BL, OUTPUT);
    pinMode(BTN, INPUT);
    digitalWrite(BL, HIGH);

    Serial.begin(115200);

    lcd.createChar(1, bar1);
    lcd.createChar(2, bar2);
    lcd.createChar(3, bar3);
    lcd.createChar(4, bar4);
    lcd.createChar(5, bar5);
    lcd.begin(20, 2);
    lcd.clear();

    Wire.begin();

    sensor.setTimeout(500);
    if (!sensor.init()) {
        lcd.print("Sem sensor.");
        Serial.println("Sem sensor");
        lcd.setCursor(0, 1);

        delay(2000);
        resetFunc();
    }

    sensor.startContinuous();

    WiFi.setSleep(false);
    WiFi.enableSTA(true);
    delay(1000);

    WiFi.begin(ssid, password);

    while (WiFi.status() != 3) {
        delay(500);
        Serial.println("Connecting to WiFi..");
        Serial.println(WiFi.status());
    }

    Serial.println("Connected to the WiFi network");
}

void LCD_progress_bar(int row, int var, int minVal, int maxVal) {
    int block = map(var, minVal, maxVal, 0,
                    18);   // Block represent the current LCD space (modify the map setting to fit your LCD)
    int line = map(var, minVal, maxVal, 0, 100);     // Line represent the theoretical lines that should be printed
    int bar = (line - (block * 5));                             // Bar represent the actual lines that will be printed

    lcd.setCursor(0, row);
    lcd.write("E");
    for (int x = 1; x < block + 1; x++)                        // Print all the filled blocks
    {
        lcd.setCursor(x, row);
        lcd.write(1023);
    }

    lcd.setCursor(block,
                  row);                            // Set the cursor at the current block and print the numbers of line needed
    if (bar != 0) lcd.write(bar);
    if (block == 0 && line == 0) lcd.write(1022);   // Unless there is nothing to print, in this case show blank

    for (int x = 20; x > block; x--)                       // Print all the blank blocks
    {
        lcd.setCursor(x, row);
        lcd.write(1022);
    }

    lcd.setCursor(19, row);
    lcd.write("F");
}

void loop() {

    int nivel = sensor.readRangeContinuousMillimeters();
    lcd.clear();

    if (sensor.timeoutOccurred()) {
        Serial.print("TIMEOUT COM O SENSOR");
    } else {
        lcd.setCursor(0, 0);
        lcd.print("QUANTIDADE:");
        lcd.print(nivel);

        if (nivel > 3000) {
            lcd.setCursor(0, 1);
            lcd.print("Fora do nivel. > 3000");
        } else {
            LCD_progress_bar(1, 400 - nivel, 0, 400);
        }
    }

    if ((millis() - lastTime) > timerDelay) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;

            http.begin(serverName);

            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            String httpRequestData = "nivel=" + String(nivel);
            http.POST(httpRequestData);

            http.end();
        } else {
            resetFunc();
        }

        lastTime = millis();
    }

    delay(1000);
}