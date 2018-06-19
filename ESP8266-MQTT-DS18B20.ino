#include <ESP8266WiFi.h>                                           // Bibliothek zum ESP8266 einbinden
#include <PubSubClient.h>                                          // Bobliothek zu MQTT einbinden
#include <OneWire.h>                                               // Bibliothek für 1-Wire-Bus einbinden
#include <DallasTemperature.h>                                     // Bibliothek für den DS18B20 einbinden

#define ONE_WIRE_BUS 5                                             // Datenpin für den DS18B20
#define LED 16                                                     // Pin für die LED

OneWire oneWire(ONE_WIRE_BUS);                                     // 1-Wire-Instanz erstellen
DallasTemperature sensors(&oneWire);                               // Dallas-Temperatursensor aktivieren

const char* ssid = "Charon";                                       // SSID des WLANS
const char* password = "Kerberos";                                 // Passwort des WLANS
const char* mqtt_server = "192.168.4.1";                           // MQTT-Server-Adresse
const char* topic_LED = "kueche/LED";                              // Topic der LED
const char* topic_TMP = "kueche/temperatur";                       // Topic des DS18B20

WiFiClient espClient3;                                             // Erstellung einer WLAN-Client-Instanz
PubSubClient client(espClient3);                                   // Erstellung einer MQTT-Client-Instanz

long lastMsg = 0;                                                  // Initialisierung der Variablen zur Zeitfeststellung
float temp = 0;                                                    // Initialisierung der Variablen zum Temperaturwert                                               

void setup_wifi() {
  delay(10);                                                       // We start by connecting to a WiFi network
  Serial.println();                                                // Serial Monitor Zeilenumbruch
  Serial.print("Connecting to ");                                  // Serial Monitor Ausgabe
  Serial.println(ssid);                                            // Serial Monitor Ausgabe der SSID
  WiFi.mode(WIFI_STA);                                             // WLAN-Modus einstellen
  WiFi.begin(ssid, password);                                      // WLAN-Verbindung mit SSID und Passwort herstellen

  while (WiFi.status() != WL_CONNECTED) {                          // Falls keine WLAN-Verbindung besteht, Punkte ausgeben
    delay(500);                                                    // Verzögerungszeit zwischen zwei Punkten
    Serial.print(".");                                             // Serial Monitor Ausgabe der Punkte
  }
  
  Serial.println("");                                              // Serial Monitor Zeilenumbruch
  Serial.println("WiFi connected");                                // Serial Monitor Ausgabe WLAN hergestellt
  Serial.println("IP address: ");                                  // Serial Monitor Ausgabe IP-Adresse
  Serial.println(WiFi.localIP());                                  // Serial Monitor Ausgabe der erhaltenen IP-Adresse
}

void reconnect() {                                                 // Schleife bis MQTT verbunden ist
  while (!client.connected()) {                                    // Solage nicht verbinden, Verbindung herstellen
    Serial.print("Attempting MQTT connection...");                 // Serial Monitor Ausgabe zum Verbindungsaufbau
    if (client.connect("espClient3")) {                            // Wenn verbunden
      Serial.println("connected");                                 // Serial Monitor Ausgabe, dass Verbindung vorhanden
    } else {                                                       // Ansonsten
      Serial.print("failed, rc=");                                 // Serial Monitor Ausgabe, für den Fehlercode
      Serial.print(client.state());                                // Serial Monitor Ausgabe des Client-Status
      Serial.println(" try again in 5 seconds");                   // Serial Monitor Ausgabe, dass es in 5 Sek. weiter geht
      delay(5000);                                                 // 5 Sek. warten
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {   // Abfrage nach LED-Schaltung
  Serial.print("Command from MQTT broker is: ");                   // Serial Monitor Ausgabe zur MQTT-Nachricht
  Serial.print(topic);                                             // Ausgabe des Topic-Namen
  int p =(char)payload[0]-'0';                                     // Speichern der Payload in einer Variablen
  if(p==0) {                                                       // Wenn eine 0 empfangen wurde
    digitalWrite(LED, LOW);                                        // LED ausschalten 
    Serial.println(" Turn Off LED! " );                            // Serial Monitor Ausgabe zum Ausschalten der LED
  }
  if(p==1) {                                                       // Wenn eine 1 empfangen wurde
    digitalWrite(LED, HIGH);                                       // LED einschalten
    Serial.println(" Turn On LED! " );                             // Serial Monitor Ausgabe zum Einschalten der LED
  }
  Serial.println();                                                // Serial Monitor Zeilenumbruch
}

void setup() {                                                     // Setup-Methode
  Serial.begin(115200);                                            // Start des Serial Monitor mit 115200 Baud
  setup_wifi();                                                    // Start des WLAN
  client.setServer(mqtt_server, 1883);                             // Herstellung der MQTT-Broker-Verbindung
  pinMode(ONE_WIRE_BUS, INPUT);                                    // Pin des DS18B20 als Input einrichten
  pinMode(LED, OUTPUT);                                            // Pin der LED als Output einrichten
  sensors.begin();                                                 // Start der Sensoren
  client.setCallback(callback);                                    // Initiales einholen der LED-MQTT-Nachricht
}

void loop() {                                                      // Loop-Methode
  if (!client.connected()) {                                       // Wenn keine Verbindung besteht
    reconnect();                                                   // Aufruf der Methode zum Verbindungsaufbau
  }
  client.loop();                                                   // MQTT-Loop
  client.subscribe(topic_LED);                                     // Subscriben des Themas zur LED
  long now = millis();                                             // Anzahl der Millisekunden zählen und an now übergeben
  if (now - lastMsg > 600) {                                       // Wenn mehr als 600 Millisekunden vergangen sind
    lastMsg = now;                                                 // Zeit übergeben
    sensors.setResolution(12);                                     // Sensor-Auflösung einstellen
    sensors.requestTemperatures();                                 // Temperatur ermitteln
    temp = sensors.getTempCByIndex(0);                             // Temperaturwert übergeben
    Serial.println(temp);                                          // Serial Monitor Temperaturwert ausgeben
    if((temp > -20) && (temp <60)) {                               // Wenn die Temperatur zwischen -20 und +60 Grad ist
      client.publish(topic_TMP, String(temp).c_str(),1);           // Publish an den MQTT-Broker mit Temperatur-Topic
    }
  }
}
