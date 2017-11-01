




#include "Arduino.h"
#include "SPI.h"

//-----------OLED 2 ----------
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS     10
#define TFT_RST    8  // you can also connect this to the Arduino reset
// in which case, set this #define pin to 0!
#define TFT_DC     9

// Option 1 (recommended): must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

//--------- Variablen ----------------

int rxByteNum;
byte rxData;
int tachWert; // U/min
int geschw; // Geschwindigkeit
int injTime; // Inejctor pulse time
float lph; // Liter pro Stunde
float lp100km; //  Liter pro 100km
unsigned long dispTime = 0, lastDispTime = 0, sensorTime = 0;


void setup() {
  // put your setup code here, to run once:

  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.setRotation(1);         // Darstellung des Displays
  tft.setTextWrap(true);    //automatischer Zeilenumbruch
  tft.fillScreen(ST7735_BLACK); //Display füllen
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.println("Init Serial... ");
  Serial.begin(9600);
  while (!Serial) {
    //Warten auf serielle Schnittstelle
  }
  tft.println("OK ");
  rxData = 0;
  byte txData = 0;
  bool conStatus = false;
  int rxByteNum = Serial.available();
  int versuch = 0;
  int i = 0;

  tft.println("available Bytes...  ");
  tft.print(rxByteNum, DEC);
  tft.println("Connect...  ");

  while (conStatus == false) {
    // ECU Initialisieren
    versuch++;
    tft.print(versuch, DEC);
    //------- Sende Init String an ECU ----
    txData = 0xFF;
    Serial.write(txData);
    Serial.write(txData);
    txData = 0xEF;
    Serial.write(txData);
    delay(250);
    rxByteNum = Serial.available();
    while (Serial.available() > 0) {
      rxData = Serial.read();
      // wenn ECU sich korrekt meldet
      if (rxData == 0x10) {
        conStatus = true;
        tft.println("Connected  ");
      }
    }
  }

  Serial.write( 0x5A );
  Serial.write( 0x00 );//MSB Tacho
  Serial.write( 0x5A );
  Serial.write( 0x01 );//LSB Tacho
  Serial.write( 0x5A );
  Serial.write( 0x0b );//Speed
  Serial.write( 0x5A );
  Serial.write( 0x14 );//MSB Injector pulse Time
  Serial.write( 0x5A );
  Serial.write( 0x15 );//LSB Injector pulse Time (WErt/100 (mS))
  // Beachten: im Empfangsstream muss "number of data bytes to follow" angepasst werden

  while (Serial.available() > 0) {
    rxData = Serial.read();
    tft.println("Daten vorhanden");
    // Antworten auslesen (jedoch nicht auswerten
  }

  rxData = 0;
  tachWert = 0; // U/min
  geschw = 0; // Geschwindigkeit
  injTime = 0; // Inejctor pulse time
  lph = 0; // Liter pro Stunde
  lp100km = 0; //  Liter pro 100km

  lastDispTime = millis();
  tft.setTextSize(2);
  Serial.flush();
  Serial.write( 0xF0 );// Go ahead termination
  // Jetzt streamen die Daten
}
//-------------------------------------------------------------------------------------------------------------------------

void loop() {
  // put your main code here, to run repeatedly:
  rxByteNum = Serial.available();

  // Antwort Format: 0xFF <- FrameStart; 0xXX <- Anzahl der folgenden Bytes; ...folgende Bytes

  //was soll hier passieren: wenn der Inputbuffer mehr als 7 Bytes enthält soll der Buffer ausgelesen werden
  // Rahmenanfang finden, prüfen ob 5 byte folgen, einmal berechnen

  if (rxByteNum > 7) {
    if (Serial.read() == 0xFF) {
      if (Serial.read() == 0x05) { // falls mehr Daten gestreamt werden hier "number of data bytes" anpassen (ordentlich programmieren)
        // -------------- Umdrehungen pro Min -------------------
        rxData = Serial.read(); // tachMSB
        tachWert = rxData << 8; // shift MSB
        rxData = Serial.read(); // Tach LSB
        tachWert = tachWert | (byte)rxData;
        tachWert = tachWert * 12.5;

        // ------------ Geschwindigkeit -------------------
        rxData = Serial.read(); // Geschwindigkeit
        geschw = rxData * 2; // in km/h

        // --------------Inejctor pulse time -----------------
        rxData = Serial.read(); // tachMSB
        injTime = rxData << 8; // shift MSB
        rxData = Serial.read(); // Tach LSB
        injTime = injTime | (byte)rxData;
        injTime = injTime / 100; // in ms
      }
    }
    Serial.flush();
    //Anzeigeupdate nach Zeit
    dispTime = millis();
    if ((dispTime - lastDispTime) > 500) {
      // --------------Verbrauch Berechnung -----------------
      lph = 4 * (((float)tachWert / 60000) * (float)injTime) * 16.2;

      tft.fillScreen(ST7735_BLACK); //Display füllen
      tft.setCursor(40, 40);
      //      tft.print("Um/min ");
      //tft.println(geschw);
      if (geschw > 0) {
        lp100km = ((lph / geschw) * 100);
        tft.print(lp100km);
        tft.setCursor(30, 80);
        tft.print(" l/100km");
      }
      else {
        tft.print(lph);
        tft.setCursor(30, 80);
        tft.print(" l/h");
      }

      lastDispTime = millis();
    }
  }
}

//-----------------------OLED Funktionen---------------------------------------
//void testdrawtext(char *text, uint16_t color) {
//  //tft.setCursor(0, 0);
//  tft.setTextColor(color);
//  tft.setTextWrap(true);
//  tft.print(text);
//}
