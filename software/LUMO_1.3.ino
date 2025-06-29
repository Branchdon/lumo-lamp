#include <Adafruit_NeoPixel.h>
#include <NTPClient.h>
#include <WiFi.h>
#include "BluetoothSerial.h"

// D32 = GPIO32
#define LED_PIN    32    // Data-Pin am LED-Streifen
#define LED_COUNT  50    // Anzahl der LEDs
#define BUTTON_PIN 25    // Pin für den Hardware Knopf

BluetoothSerial SerialBT;

bool LED_STATUS = false;
int aktColor[3] = {255, 255, 255}; 

// Zeitpunkte wann Lampe an / aus {Stunde, Minute, Sekunde}
int zeitAn[3] = {18, 0, 0}
int zeitAus[3] = {0, 0, 0}

const char *ssid = "iPhone von Jannik";
const char *password = "Hallo123";

const long utcOffsetInSeconds = 7200; // Zeitverschiebung

long zuletztGedrueckt = 0;
long zaehlerResetIntervall = 1000;

// Wochentage als String-Array
char daysOfTheWeek[7][12] = {
  "Sonntag", "Montag", "Dienstag",
  "Mittwoch", "Donnerstag", "Freitag", "Samstag"
};

bool zeitSchaltungAktiv = true; 

// Initialisierung des Zeit-Clients
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(19200);
  wlanVerbindung(); // WLAN Verbindung herstellen
  initialisieren();
}

void wlanVerbindung() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Ich verbinde mich mit dem Internet...");
  }
  Serial.println("Ich bin mit dem Internet verbunden!");

  // NTP-Client starten
  timeClient.begin();
} 

void initialisieren() {
  strip.begin();             // Initialisiert den LED-Streifen
  strip.show();              // Alle LEDs ausschalten
  strip.setBrightness(255);  // Helligkeit (0-255)
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button-Pin initialisieren
  SerialBT.begin("Lumo-Lampe"); // Bluetooth starten
  Serial.println("Bluetooth gestartet. Gerät heißt: Lumo-Lampe");
  SerialBT.setTimeout(1); // Timeout Zeit auf 1ms
}

void loop() {
  buttonSteuerung(); 
  bluetoothSteuerung();
  if(zeitSchaltungAktiv) zeitSteuerung();

}


bool letzterButtonStatus = true;
int wechselZaehler = 0;

void buttonSteuerung() {
  bool aktuellerStatus = digitalRead(BUTTON_PIN);
  //Zeitschaltungscounter nur weiterzählen, wenn 3 mal schnell gedrückt wird
  if ( (millis() - zuletztGedrueckt) > zaehlerResetIntervall) wechselZaehler = 0;

  // Nur reagieren, wenn sich der Zustand geändert hat
  if (aktuellerStatus != letzterButtonStatus) {
    Serial.println("Zustand geändert");

    lampSwitch(!LED_STATUS);

    zuletztGedrueckt = millis();
    wechselZaehler++;
    Serial.print("Wechsel erkannt. Zähler: ");
    Serial.println(wechselZaehler);

    // Wenn 3 Wechsel erkannt wurden
    if (wechselZaehler >= 3) {


      zeitSchaltungAktiv = !zeitSchaltungAktiv;


      if(zeitSchaltungAktiv) einmalGruenLeuchten();
      if(!zeitSchaltungAktiv) einmalRotLeuchten();

      lampSwitch(!LED_STATUS); 

      Serial.print("Zeitschaltung ist jetzt: ");
      Serial.println(zeitSchaltungAktiv ? "Aktiv" : "Inaktiv");

      wechselZaehler = 0; 
    }

    letzterButtonStatus = aktuellerStatus;
    delay(50);
  }
}

void bluetoothSteuerung(){

    if (SerialBT.available()) {

    String empfangenColor = SerialBT.readString(); // Eingehende Daten lesen
    empfangenColor.trim();
    
    Serial.print("Empfangen via Bluetooth: ");
    Serial.println(empfangenColor);
    //Serial.println(empfangenStatus);

    // Beispiel: '1' schaltet Licht an, '0' aus
    if ((empfangenColor == "1" || empfangenColor == "0")) {
      
      lampSwitch(!LED_STATUS);

    } else if(empfangenColor.length() == 9) {

        int rot = empfangenColor.substring(0,3).toInt();
        int gruen = empfangenColor.substring(3,6).toInt();
        int blau = empfangenColor.substring(6).toInt();

        aktColor[0] = rot;
        aktColor[1] = gruen;
        aktColor[2] = blau;

      lampSwitch(true);

    }

  }

}

void zeitSteuerung() {
  timeClient.update();

  int stunde = timeClient.getHours();
  int minute = timeClient.getMinutes();
  int sekunde = timeClient.getSeconds();

  Serial.print("Uhrzeit: ");
  Serial.println(timeClient.getFormattedTime());

  // Schalte um 18:00:00 EIN
  if (stunde == 17 && minute == 19 && sekunde == 0 && !LED_STATUS) {
    lampSwitch(true);
    Serial.println("Lampe automatisch eingeschaltet (18:00 Uhr)");
  }

  // Schalte um 00:00:00 AUS
  if (stunde == 17 && minute == 19 && sekunde == 30 && LED_STATUS) {
    lampSwitch(false);
    Serial.println("Lampe automatisch ausgeschaltet (0:00 Uhr)");
  }
}



void lampSwitch(bool x) {
  LED_STATUS = x;

  if (x) {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(aktColor[0], aktColor[1], aktColor[2])); // LED an
    }
    strip.show();
  } else {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0)); // LED aus
    }
    strip.show();
  }
}

void einmalRotLeuchten() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0)); // Rot
  }
  strip.show();
  delay(500);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0)); // aus
  }
  strip.show();
  delay(500);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0)); // Rot
  }
  strip.show();
  delay(500);
}

void einmalGruenLeuchten() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0)); // Rot
  }
  strip.show();
  delay(500);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0)); // aus
  }
  strip.show();
  delay(500);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0)); // Rot
  }
  strip.show();
  delay(500);
 
}

