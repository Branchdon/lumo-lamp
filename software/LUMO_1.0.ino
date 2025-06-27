#include <Adafruit_NeoPixel.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// D32 = GPIO32
#define LED_PIN    32    // Data-Pin am LED-Streifen
#define LED_COUNT  50    // Anzahl der LEDs
#define BUTTON_PIN 25

bool LED_STATUS = false;

const char *ssid = "Wlan name";
const char *password = "passwort";

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
  Serial.println("setup");
}

/*void wlanVerbindung() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Ich verbinde mich mit dem Internet...");
  }
  Serial.println("Ich bin mit dem Internet verbunden!");

  // NTP-Client starten
  timeClient.begin();
} */

void initialisieren() {
  strip.begin();             // Initialisiert den LED-Streifen
  strip.show();              // Alle LEDs ausschalten
  strip.setBrightness(255);  // Helligkeit (0-255)
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button-Pin initialisieren
}

void loop() {
  Serial.println("Fortntitiititititit");

  buttonSteuerung(); 
  
  Serial.println("FOrnttie");

}


bool letzterButtonStatus = true;
int wechselZaehler = 0;

void buttonSteuerung() {
  bool aktuellerStatus = digitalRead(BUTTON_PIN);
  Serial.println("Zustand gleich");
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

