#include <Adafruit_NeoPixel.h> // Bibliothek für Steuerung von NeoPixel-LEDs
#include <NTPClient.h>         // Bibliothek für Netzwerk-Zeitabfrage per NTP
#include <WiFi.h>              // Bibliothek für WLAN-Verbindung
#include "BluetoothSerial.h"   // Bibliothek für Bluetooth-Kommunikation (nur auf ESP32 verfügbar)

// D32 = GPIO32
#define LED_PIN    32    // Daten-Pin für den LED-Streifen
#define LED_COUNT  60    // Anzahl der LEDs im Streifen
#define BUTTON_PIN 25    // Pin für den physischen Taster

BluetoothSerial SerialBT; // Bluetooth-Objekt

bool LED_STATUS = false;          // Aktueller Status der LEDs (an/aus)
int aktColor[3] = {255, 255, 255}; // Standardfarbe: Weiß (RGB)

// Einschalt- und Ausschaltzeit [Stunde, Minute, Sekunde]
int zeitAn[3] = {18, 0, 0};
int zeitAus[3] = {0, 0, 0};

// WLAN-Zugangsdaten
const char *ssid = "iPhone von Jannik";
const char *password = "Hallo123";

const long utcOffsetInSeconds = 7200; // Zeitzonenoffset (z.B. +2 Stunden für MESZ)

long zuletztGedrueckt = 0;           // Zeitpunkt der letzten Button-Interaktion
long zaehlerResetIntervall = 1000;   // Zeitintervall für Reset des Wechselzählers (1 Sekunde)

// Namen der Wochentage für spätere Nutzung (optional)
char daysOfTheWeek[7][12] = {
  "Sonntag", "Montag", "Dienstag",
  "Mittwoch", "Donnerstag", "Freitag", "Samstag"
};

bool zeitSchaltungAktiv = true; // Ob die Zeitsteuerung aktiv ist
bool wlanAktiv = true;          // Ob WLAN aktiv ist

// Initialisiert den NTP-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Objekt für LED-Streifen
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(19200);      // Serielle Kommunikation starten
  wlanVerbindung();         // Versuche WLAN-Verbindung herzustellen
  initialisieren();         // Initialisiere LEDs, Bluetooth und Button
}

void wlanVerbindung() {
  WiFi.begin(ssid, password); // WLAN-Verbindung starten

  Serial.println("Versuche, eine WLAN-Verbindung herzustellen...");
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 5000; // Timeout: 5 Sekunden

  // Warten auf Verbindung oder Timeout
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nMit dem WLAN verbunden!");
    timeClient.begin(); // Starte Zeitclient
  } else {
    Serial.println("\nKeine WLAN-Verbindung. Zeitsteuerung deaktiviert.");
    zeitSchaltungAktiv = false;
    wlanAktiv = false; 
  }
}

void initialisieren() {
  strip.begin();             // LED-Streifen initialisieren
  strip.show();              // Alle LEDs ausschalten
  strip.setBrightness(255);  // Helligkeit auf Maximum
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button als Eingang mit Pullup-Widerstand
  SerialBT.begin("Lumo-Lampe");      // Bluetooth starten mit Gerätename
  Serial.println("Bluetooth gestartet. Gerät heißt: Lumo-Lampe");
  SerialBT.setTimeout(1);            // Bluetooth-Timeout setzen
}

void loop() {
  buttonSteuerung();                  // Auswertung der Button-Eingabe
  bluetoothSteuerung();              // Auswertung eingehender Bluetooth-Daten
  if (zeitSchaltungAktiv) zeitSteuerung(); // Zeitsteuerung prüfen
}

bool letzterButtonStatus = true; // Letzter bekannter Status des Buttons
int wechselZaehler = 0;          // Zähler für schnelle Wechsel zur Umschaltung

void buttonSteuerung() {
  bool aktuellerStatus = digitalRead(BUTTON_PIN); // Aktuellen Button-Zustand lesen

  // Zähler zurücksetzen, wenn zu viel Zeit vergangen ist
  if ((millis() - zuletztGedrueckt) > zaehlerResetIntervall) wechselZaehler = 0;

  // Nur reagieren, wenn sich der Button-Zustand geändert hat
  if (aktuellerStatus != letzterButtonStatus) {
    Serial.println("Zustand geändert");

    lampSwitch(!LED_STATUS); // Lampe umschalten

    zuletztGedrueckt = millis();
    if (wlanAktiv) wechselZaehler++; // Nur wenn WLAN da ist, Zeitumschaltung zählen

    Serial.print("Wechsel erkannt. Zähler: ");
    Serial.println(wechselZaehler);

    // Wenn 3 schnelle Wechsel erkannt wurden → Zeitsteuerung umschalten
    if (wechselZaehler >= 3) {
      zeitSchaltungAktiv = !zeitSchaltungAktiv; // Umschalten

      if (zeitSchaltungAktiv) einmalGruenLeuchten(); // Grün: Aktiv
      else einmalRotLeuchten();                     // Rot: Inaktiv

      lampSwitch(!LED_STATUS); // Lampe wieder umschalten
      Serial.print("Zeitschaltung ist jetzt: ");
      Serial.println(zeitSchaltungAktiv ? "Aktiv" : "Inaktiv");

      wechselZaehler = 0; // Zähler zurücksetzen
    }

    letzterButtonStatus = aktuellerStatus; // Zustand aktualisieren
    delay(50); // Entprellung
  }
}

void bluetoothSteuerung() {
  if (SerialBT.available()) {
    String empfangenColor = SerialBT.readString(); // Eingehende Daten lesen
    empfangenColor.trim();

    Serial.print("Empfangen via Bluetooth: ");
    Serial.println(empfangenColor);

    // Ein/Aus-Schalten per '1' oder '0'
    if ((empfangenColor == "1" || empfangenColor == "0")) {
      lampSwitch(!LED_STATUS);
    }

    // Startzeit einstellen z. B. "ein180000"
    else if (empfangenColor.substring(0, 3) == "ein" && empfangenColor.length() > 3) {
      zeitAn[0] = empfangenColor.substring(4, 6).toInt();
      zeitAn[1] = empfangenColor.substring(6, 8).toInt();
      zeitAn[2] = empfangenColor.substring(8, 10).toInt();
      Serial.println(String(zeitAn[0]) + " " + String(zeitAn[1]) + " " + String(zeitAn[2]));
    }

    // Ausschaltzeit einstellen z. B. "aus000000"
    else if (empfangenColor.substring(0, 3) == "aus" && empfangenColor.length() > 3) {
      zeitAus[0] = empfangenColor.substring(4, 6).toInt();
      zeitAus[1] = empfangenColor.substring(6, 8).toInt();
      zeitAus[2] = empfangenColor.substring(8, 10).toInt();
      Serial.println(String(zeitAus[0]) + " " + String(zeitAus[1]) + " " + String(zeitAus[2]));
    }

    // Sprachkommandos
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

    // Farbeingabe als 9-stelliger String z. B. "255000125" für RGB
    else if (empfangenColor.length() == 9) {
      int rot = empfangenColor.substring(0, 3).toInt();
      int gruen = empfangenColor.substring(3, 6).toInt();
      int blau = empfangenColor.substring(6).toInt();

      aktColor[0] = rot;
      aktColor[1] = gruen;
      aktColor[2] = blau;

      lampSwitch(true); // Lampe mit neuer Farbe anschalten
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

  // Ein- oder Ausschalten zum programmierten Zeitpunkt
  if (stunde == zeitAn[0] && minute == zeitAn[1] && sekunde == zeitAn[2] && !LED_STATUS) {
    lampSwitch(true);
  }

  if (stunde == zeitAus[0] && minute == zeitAus[1] && sekunde == zeitAus[2] && LED_STATUS) {
    lampSwitch(false);
  }
}

// Schaltet alle LEDs an oder aus, je nach x (true = an, false = aus)
void lampSwitch(bool x) {
  LED_STATUS = x;

  if (x) {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(aktColor[0], aktColor[1], aktColor[2]));
    }
  } else {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  strip.show();
}

// LEDs blinken rot als visuelles Feedback (z. B. für „aus“)
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

// LEDs blinken grün als visuelles Feedback (z. B. für „ein“)
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
