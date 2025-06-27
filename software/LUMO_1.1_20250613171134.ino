#include <Adafruit_NeoPixel.h>
#include <NTPClient.h>
#include <WiFi.h>
#include "BluetoothSerial.h"

// D32 = GPIO32
#define LED_PIN    32    // Data-Pin am LED-Streifen
#define LED_COUNT  50    // Anzahl der LEDs
#define BUTTON_PIN 25

BluetoothSerial SerialBT;

bool LED_STATUS = false;
//Hallo

const char *ssid = "andydelfino Gastzugang";
const char *password = "HomeCording";

const long utcOffsetInSeconds = 3600; // Zeitverschiebung

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
 // wlanVerbindung(); // WLAN Verbindung herstellen
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
    char empfangen = SerialBT.read(); // Eingehende Daten lesen
    Serial.print("Empfangen via Bluetooth: ");
    Serial.println(empfangen);

    // Beispiel: '1' schaltet Licht an, '0' aus
    if (empfangen == '1' || empfangen == '0') {
      
      lampSwitch(!LED_STATUS);

    }

  }

}

void zeitSteuerung(){

}

void lampSwitch(bool x) {
  LED_STATUS = x;

  if (x) {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(255, 255, 255)); // LED an
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
}

void einmalGruenLeuchten() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0)); // Grün
  }
  strip.show();
  delay(500);
}

