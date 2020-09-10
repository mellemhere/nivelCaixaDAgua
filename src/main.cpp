#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-static-accessed-through-instance"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <Arduino.h>
#include <Wire.h>
#include "WiFi.h"
#include "HCSR04.h"
#include "LiquidCrystal.h"
#include "HTTPClient.h"
#include "string.h"

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

#define ECHO 16
#define TRIGGER 17
#define TEMP_AMB 24
#define MAX_DIST 300

#define FULL 24 // 24 cm == 1000 L
#define EMPTY 85

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

HCSR04 ultrasonicSensor(TRIGGER, ECHO, TEMP_AMB, MAX_DIST);

/*
 *  Barras de progresso
 */
byte bar1[8] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
byte bar2[8] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
byte bar3[8] = {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C};
byte bar4[8] = {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E};
byte bar5[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

float current_distance = 0;
double current_level = 0;

void (*resetFunc)() = nullptr;

void setup() {
    Serial.begin(115200);

    /*
     *  LCD
     */
    pinMode(BL, OUTPUT);
    pinMode(BTN, INPUT);
    digitalWrite(BL, LOW);
    lcd.createChar(1, bar1);
    lcd.createChar(2, bar2);
    lcd.createChar(3, bar3);
    lcd.createChar(4, bar4);
    lcd.createChar(5, bar5);
    lcd.begin(20, 2);
    lcd.clear();
    digitalWrite(BL, HIGH);

    /*
     *  Sensor
     */
    ultrasonicSensor.begin();

    /*
     *  Wifi
     */
    WiFi.setSleep(false);
    WiFi.enableSTA(true);
    delay(1000);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
        if (WiFi.status() == WL_CONNECT_FAILED) {
            resetFunc();
        }
    }

    Serial.println("Done");
}

void draw_bar() {
    int block = map(current_level, 0, 100, 0,
                    20);   // Block represent the current LCD space (modify the map setting to fit your LCD)
    int line = map(current_level, 0, 100, 0, 100);     // Line represent the theoretical lines that should be printed
    int bar = (line - (block * 5));                             // Bar represent the actual lines that will be printed

    /* LCD Progress Bar Characters, create your custom bars */

    for (int x = 0; x < block; x++)                        // Print all the filled blocks
    {
        lcd.setCursor(x, 1);
        lcd.write(1023);
    }

    lcd.setCursor(block,
                  1);                            // Set the cursor at the current block and print the numbers of line needed
    if (bar != 0) lcd.write(bar);
    if (block == 0 && line == 0) lcd.write(1022);   // Unless there is nothing to print, in this case show blank

    for (int x = 16; x > block; x--)                       // Print all the blank blocks
    {
        lcd.setCursor(x, 1);
        lcd.write(1022);
    }
}

void transmit() {
    if ((millis() - lastTime) > timerDelay) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;

            http.begin(serverName);

            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            String httpRequestData = "nivel=" + String(current_level);
            int respCode = http.POST(httpRequestData);

            Serial.print("Transmitindo. Resp: ");
            Serial.println(respCode);
            http.end();
        } else {
            resetFunc();
        }

        lastTime = millis();
    }
}

void loop() {

    current_distance = ultrasonicSensor.getMedianFilterDistance(); //pass 3 measurements through median filter, better result on moving obstacles

    if (current_distance != HCSR04_OUT_OF_RANGE) {
        Serial.print(current_distance, 1);
        Serial.println(F(" cm"));

        current_level = 100.0 - ((100.0 / (EMPTY - FULL)) * (current_distance - FULL));
        if (current_level < 0) current_level = 0;
        if (current_level > 100) current_level = 100;

        Serial.print(current_level);
        Serial.println(F(" %"));

        lcd.setCursor(0, 0);
        lcd.print("Nivel:");

        lcd.setCursor(13, 0);
        lcd.print(current_level, 2);
        lcd.print("%");


        transmit();
        draw_bar();
    }

    delay(1000);

}

#pragma clang diagnostic pop