# Gabarits de sketchs

Chaque bloc `cpp` ci-dessous est un sketch complet qui compile (vérifié avec PlatformIO). Partir du gabarit le
plus proche du besoin, changer le nom d'appareil, le topic, les clés. Pour passer de l'Ethernet au WiFi,
remplacer la ligne `beginEthernet(...)` par :

```text
CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);
```
(et ajouter `WIFI_NOM` / `WIFI_MOT_DE_PASSE` dans la zone « À MODIFIER »).
Dans l'Arduino IDE, le type de carte pour une PoE est « OLIMEX ESP32-POE-ISO » ou « OLIMEX ESP32-POE ».

## 1. Envoyer des nombres en continu (Ethernet PoE)

```cpp
// Envoie r, g, b (0.0 à 1.0), 20 fois par seconde. Topic : saillance/globale/lumiere/bande1/couleur
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple01";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

FloatOut couleur;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);  // carte sans "ISO" : Board::Esp32Poe

  Topic topic = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande1", "couleur");
  couleur = CexiiMqtt.floatOut(topic, { "r", "g", "b" }, 20, SendMode::Continuous);
}

void loop() {
  CexiiMqtt.update();  // obligatoire, à chaque tour

  float t = millis() / 1000.0f;
  couleur.set("r", (sin(t) + 1) / 2);
  couleur.set("g", (sin(t + 2) + 1) / 2);
  couleur.set("b", (sin(t + 4) + 1) / 2);
}
```

## 2. Envoyer un capteur seulement quand il change (WiFi)

```cpp
// Potentiomètre sur la broche 34 -> saillance/regie/controle/potard1/valeur
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple02";
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
const int   PIN_CAPTEUR       = 34;
// ----------------------------------------------------------------------------

FloatOut potard;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);
  Topic topic = CexiiMqtt.topic(Zone::Regie, "controle", "potard1", "valeur");
  potard = CexiiMqtt.floatOut(topic, { "val" }, 30, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();
  float valeur = analogRead(PIN_CAPTEUR) / 4095.0f;
  valeur = round(valeur * 100) / 100.0f;  // 2 décimales : évite d'envoyer le bruit du capteur
  potard.set("val", valeur);
}
```

## 3. Envoyer du texte

```cpp
// Un compteur en texte chaque seconde -> saillance/ecrans/affichage/ecran1/texte
#include <CexiiMqtt.h>

const char* NOM_APPAREIL = "exemple03";

TextOut message;
unsigned long dernierEnvoi = 0;
int compteur = 0;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "affichage", "ecran1", "texte");
  message = CexiiMqtt.textOut(topic, 5, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();
  if (millis() - dernierEnvoi > 1000) {
    dernierEnvoi = millis();
    compteur++;
    message.set(String("compteur = ") + compteur);
  }
}
```

## 4. Envoyer une table

```cpp
// Table de 2 rangées dont certaines cases changent -> saillance/ecrans/tableau/liste/contenu
#include <CexiiMqtt.h>

const char* NOM_APPAREIL = "exemple04";

TableOut tableau;
unsigned long dernierChangement = 0;
bool etat = false;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "tableau", "liste", "contenu");
  tableau = CexiiMqtt.tableOut(topic, 5, SendMode::Event);
  tableau.addRow({ "a", "0.00", "off" });  // rangée 0
  tableau.addRow({ "b", "0.00", "off" });  // rangée 1
}

void loop() {
  CexiiMqtt.update();
  if (millis() - dernierChangement > 2000) {
    dernierChangement = millis();
    etat = !etat;
    tableau.set(0, 1, (sin(millis() / 1000.0f) + 1) / 2);  // un nombre devient du texte
    tableau.set(0, 2, etat ? "on" : "off");
    tableau.set(1, 2, etat ? "off" : "on");
  }
}
```

## 5. Recevoir les trois formats avec une fonction de rappel

```cpp
// Affiche tout ce qui arrive sur saillance/globale/lumiere/bande2/couleur
#include <CexiiMqtt.h>

const char* NOM_APPAREIL = "exemple05";

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);
  Topic topic = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande2", "couleur");

  CexiiMqtt.input(topic, [](const Message& m) {
    if (m.isTable) {
      for (size_t y = 0; y < m.rowCount(); y++) {
        for (size_t x = 0; x < m.colCount(y); x++) Serial.printf("[%s] ", m.cell(y, x));
        Serial.println();
      }
    } else if (m.isText) {
      Serial.printf("texte : %s\n", m.text);
    } else {
      for (uint8_t i = 0; i < m.count; i++) Serial.printf("%s = %.2f\n", m.key(i), m.value(i));
    }
  });
}

void loop() {
  CexiiMqtt.update();  // c'est update() qui appelle la fonction ci-dessus
}
```

## 6. Recevoir des nombres en lecture à la demande (LED)

```cpp
// La luminosité de la LED suit la clé "niv" reçue sur saillance/jam/lumiere/plafonnier/niveau
#include <CexiiMqtt.h>

const char* NOM_APPAREIL = "exemple06";
const int   PIN_LED      = 2;

Input niveau;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);
  Topic topic = CexiiMqtt.topic(Zone::Jam, "lumiere", "plafonnier", "niveau");
  niveau = CexiiMqtt.input(topic);
}

void loop() {
  CexiiMqtt.update();
  if (niveau.isNew()) Serial.printf("niv = %.2f\n", niveau.get("niv"));
  analogWrite(PIN_LED, (int)(niveau.get("niv") * 255));  // 0.0 tant que rien n'est arrivé
}
```

## 7. État de la connexion et erreur personnalisée

```cpp
#include <CexiiMqtt.h>

const char* NOM_APPAREIL = "exemple07";

unsigned long dernierAffichage = 0;
bool erreurEnvoyee = false;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.setAppName("exemple07_v1.0");  // avant begin : apparaît dans le champ "app" de infos
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);
}

void loop() {
  CexiiMqtt.update();
  if (millis() - dernierAffichage > 5000) {
    dernierAffichage = millis();
    if (CexiiMqtt.connected()) Serial.printf("Connecté en tant que %s\n", CexiiMqtt.username());
    else Serial.println("Pas encore connecté...");
  }
  if (!erreurEnvoyee && millis() > 15000) {
    erreurEnvoyee = true;
    CexiiMqtt.reportError("capteur", "lecture impossible (test)", Severity::Warning);
  }
}
```
