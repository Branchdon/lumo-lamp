#include <Adafruit_NeoPixel.h>     // Bibliothek für NeoPixel-LED-Steuerung
#include <NTPClient.h>             // Bibliothek für Netzwerk-Zeitsynchronisierung
#include <WiFi.h>                  // WLAN-Funktionen
#include "BluetoothSerial.h"       // Bluetooth-Funktionen (nur ESP32)

// Hardwaredefinitionen
#define LED_PIN    32              // GPIO-Pin für Datenleitung der LEDs
#define LED_COUNT  60              // Anzahl der LEDs im Streifen
#define BUTTON_PIN 25              // GPIO-Pin für den Taster

BluetoothSerial SerialBT;         // Bluetooth-Objekt erzeugen

bool LED_STATUS = false;          // Aktueller Schaltzustand der LEDs (an/aus)
int aktColor[3] = {255, 255, 255}; // Aktuelle Farbe in RGB (weiß)

// Einschalt- und Ausschaltzeit in [Stunde, Minute, Sekunde]
int zeitAn[3] = {18, 0, 0};
int zeitAus[3] = {0, 0, 0};

// WLAN-Zugangsdaten
const char *ssid = "iPhone von Jannik";
const char *password = "Hallo123";

// Zeitverschiebung in Sekunden (UTC+2)
const long utcOffsetInSeconds = 7200;

long zuletztGedrueckt = 0;         // Zeitstempel der letzten Taster-Interaktion
long zaehlerResetIntervall = 1000; // Zeitintervall für Reset der Erkennung

// Namen der Wochentage (nicht verwendet im Code)
char daysOfTheWeek[7][12] = {
  "Sonntag", "Montag", "Dienstag",
  "Mittwoch", "Donnerstag", "Freitag", "Samstag"
};

bool zeitSchaltungAktiv = true;   // Ist die Zeitautomatik aktiv?

// Initialisierung NTP (Zeit-Client)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// LED-Streifen-Objekt
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(19200);      // Serielle Ausgabe starten
  wlanVerbindung();         // Verbindung mit WLAN herstellen
  initialisieren();         // LEDs, Taster, Bluetooth starten
}

void wlanVerbindung() {
  WiFi.begin(ssid, password); // WLAN-Verbindung starten

  // Solange keine Verbindung besteht, alle Sekunde retry
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Ich verbinde mich mit dem Internet...");
  }

  Serial.println("Ich bin mit dem Internet verbunden!");

  // Start des Zeit-Clients
  timeClient.begin();
}

void initialisieren() {
  strip.begin();             // Initialisierung des LED-Streifens
  strip.show();              // Alle LEDs ausschalten
  strip.setBrightness(255);  // Maximale Helligkeit
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Taster als Eingang
  SerialBT.begin("Lumo-Lampe");      // Bluetooth starten
  Serial.println("Bluetooth gestartet. Gerät heißt: Lumo-Lampe");
  SerialBT.setTimeout(1);            // BT-Lese-Timeout auf 1ms setzen
}

void loop() {
  buttonSteuerung();                   // Auswertung des Hardware-Buttons
  bluetoothSteuerung();               // Auswertung der Bluetooth-Kommandos
  if (zeitSchaltungAktiv) zeitSteuerung(); // Zeitsteuerung nur wenn aktiv
}

// Buttonstatus-Tracking
bool letzterButtonStatus = true;
int wechselZaehler = 0;

void buttonSteuerung() {
  bool aktuellerStatus = digitalRead(BUTTON_PIN); // Aktuellen Status lesen

  // Zähler zurücksetzen, wenn zu lange kein Wechsel kam
  if ((millis() - zuletztGedrueckt) > zaehlerResetIntervall) wechselZaehler = 0;

  // Reagiere nur, wenn sich der Button-Zustand geändert hat
  if (aktuellerStatus != letzterButtonStatus) {
    Serial.println("Zustand geändert");

    lampSwitch(!LED_STATUS);           // Licht an/aus schalten

    zuletztGedrueckt = millis();       // Zeit merken
    wechselZaehler++;                  // Wechsel zählen
    Serial.print("Wechsel erkannt. Zähler: ");
    Serial.println(wechselZaehler);

    // Wenn 3 schnelle Wechsel erkannt wurden → Zeitautomatik umschalten
    if (wechselZaehler >= 3) {
      zeitSchaltungAktiv = !zeitSchaltungAktiv;

      if (zeitSchaltungAktiv) einmalGruenLeuchten(); // Visual Feedback
      else einmalRotLeuchten();

      lampSwitch(!LED_STATUS); // Lichtzustand umkehren

      Serial.print("Zeitschaltung ist jetzt: ");
      Serial.println(zeitSchaltungAktiv ? "Aktiv" : "Inaktiv");

      wechselZaehler = 0; // Reset
    }

    letzterButtonStatus = aktuellerStatus; // Update Status
    delay(50); // Entprellen
  }
}

void bluetoothSteuerung() {
  if (SerialBT.available()) {
    String empfangenColor = SerialBT.readString(); // Eingehende Daten lesen
    empfangenColor.trim(); // Leerzeichen entfernen

    Serial.print("Empfangen via Bluetooth: ");
    Serial.println(empfangenColor);

    // Licht ein/aus mit "1" oder "0"
    if ((empfangenColor == "1" || empfangenColor == "0")) {
      lampSwitch(!LED_STATUS);
    }

    // Zeit zum Einschalten setzen (z.B. "ein180000")
    else if (empfangenColor.substring(0, 3) == "ein" && empfangenColor.length() > 3) {
      zeitAn[0] = empfangenColor.substring(4, 6).toInt();
      zeitAn[1] = empfangenColor.substring(6, 8).toInt();
      zeitAn[2] = empfangenColor.substring(8, 10).toInt();
      Serial.println(String(zeitAn[0]) + " " + String(zeitAn[1]) + " " + String(zeitAn[2]));
    }

    // Zeit zum Ausschalten setzen (z.B. "aus230000")
    else if (empfangenColor.substring(0, 3) == "aus" && empfangenColor.length() > 3) {
      zeitAus[0] = empfangenColor.substring(4, 6).toInt();
      zeitAus[1] = empfangenColor.substring(6, 8).toInt();
      zeitAus[2] = empfangenColor.substring(8, 10).toInt();
      Serial.println(String(zeitAus[0]) + " " + String(zeitAus[1]) + " " + String(zeitAus[2]));
    }

    // Sprachbefehle
    else if (empfangenColor == "Licht ein") {
      lampSwitch(true);
    } else if (empfangenColor == "Licht aus") {
      lampSwitch(false);
    } else if (empfangenColor == "Zeitschaltung an") {
      zeitSchaltungAktiv = true;
      einmalGruenLeuchten();
      lampSwitch(LED_STATUS);
    } else if (empfangenColor == "Zeitschaltung aus") {
      zeitSchaltungAktiv = false;
      einmalRotLeuchten();
      lampSwitch(LED_STATUS);
    }

    // Farbeingabe (z.B. "255125000")
    else if (empfangenColor.length() == 9) {
      int rot = empfangenColor.substring(0, 3).toInt();
      int gruen = empfangenColor.substring(3, 6).toInt();
      int blau = empfangenColor.substring(6).toInt();

      aktColor[0] = rot;
      aktColor[1] = gruen;
      aktColor[2] = blau;

      lampSwitch(true); // Neue Farbe sofort anwenden
    }
  }
}

void zeitSteuerung() {
  timeClient.update(); // Aktuelle Zeit abrufen

  int stunde = timeClient.getHours();
  int minute = timeClient.getMinutes();
  int sekunde = timeClient.getSeconds();

  Serial.print("Uhrzeit: ");
  Serial.println(timeClient.getFormattedTime());

  // Lampe einschalten zur definierten Einschaltzeit
  if (stunde == zeitAn[0] && minute == zeitAn[1] && sekunde == zeitAn[2] && !LED_STATUS) {
    lampSwitch(true);
  }

  // Lampe ausschalten zur definierten Ausschaltzeit
  if (stunde == zeitAus[0] && minute == zeitAus[1] && sekunde == zeitAus[2] && LED_STATUS) {
    lampSwitch(false);
  }
}

void lampSwitch(bool x) {
  LED_STATUS = x;

  if (x) {
    // Alle LEDs mit aktueller Farbe einschalten
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(aktColor[0], aktColor[1], aktColor[2]));
    }
  } else {
    // Alle LEDs ausschalten
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }

  strip.show(); // Änderungen übernehmen
}

// Kurzes rotes Blinksignal – z. B. zum Anzeigen von "Zeitsteuerung aus"
void einmalRotLeuchten() {
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(255, 0, 0));
  strip.show();
  delay(500);
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 0, 0));
  strip.show();
  delay(500);
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(255, 0, 0));
  strip.show();
  delay(500);
}

// Kurzes grünes Blinksignal – z. B. zum Anzeigen von "Zeitsteuerung an"
void einmalGruenLeuchten() {
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 255, 0));
  strip.show();
  delay(500);
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 0, 0));
  strip.show();
  delay(500);
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 255, 0));
  strip.show();
  delay(500);
}
