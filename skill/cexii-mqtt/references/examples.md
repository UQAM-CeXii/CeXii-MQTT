# Gabarits de sketchs

Ces 11 sketchs sont **les exemples de la bibliothèque** (`examples/`), recopiés tels quels : ils compilent tous
(vérifié avec PlatformIO). Partir de celui qui est le plus proche du besoin, puis changer le nom d'appareil, le
topic, les clés.

Le style à garder quand on écrit un sketch pour quelqu'un :
- un bloc d'en-tête qui dit ce que fait le sketch, le matériel, comment le tester dans MQTTX ;
- une zone « À MODIFIER » en haut (nom d'appareil, et WiFi seulement pour l'exemple WiFi) ;
- le code découpé en **étapes numérotées** en commentaire : `// 1) Démarrer…`, `// 2) Choisir où envoyer…`,
  `// 3) Créer l'envoi…`, puis `// 4) …` dans `loop()` ;
- `CexiiMqtt.update();` en premier dans `loop()`, avec le commentaire « OBLIGATOIRE ».

Par défaut, utiliser l'**Ethernet** (`beginEthernet(Board::Esp32PoeIso, …)`, carte « OLIMEX ESP32-POE-ISO »
dans l'Arduino IDE ; `Board::Esp32Poe` pour la carte sans « ISO »). Le WiFi n'est utilisé que si la personne a une
ESP32 classique ou le demande : voir l'exemple 11.

| Besoin | Exemple |
|---|---|
| envoyer des nombres en continu | 01 |
| envoyer un capteur, seulement quand il change | 02 |
| envoyer du texte | 03 |
| envoyer une table | 04 |
| recevoir des nombres (lecture à la demande) | 05 |
| recevoir du texte (fonction automatique) | 06 |
| recevoir une table | 07 |
| envoyer et recevoir les trois formats | 08 |
| envoyer l'état d'un bouton | 09 |
| état de la connexion, erreur personnalisée | 10 |
| WiFi (ESP32 classique) | 11 |


## Exemple 01 : ENVOYER DES NOMBRES (ESP32 PoE branchée en Ethernet)

```cpp
// ============================================================================
//  Exemple 01 : ENVOYER DES NOMBRES (ESP32 PoE branchée en Ethernet)
//
//  Envoie 3 valeurs (r, g, b) qui changent, 20 fois par seconde.
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/globale/lumiere/bande1/couleur
//
//  Pas de câble ? Remplacez la ligne "beginEthernet" par la version WiFi :
//      CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, "NomDuWifi", "MotDePasse");
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple01";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

FloatOut couleur;  // notre "tuyau" d'envoi de nombres

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  //    Le username sera "esp32poeiso_exemple01". Carte sans "ISO" : Board::Esp32Poe
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande1", "couleur");

  // 3) Créer l'envoi : le topic, le nom de chaque valeur,
  //    20 envois par seconde au maximum, en continu
  couleur = CexiiMqtt.floatOut(topic, { "r", "g", "b" }, 20, SendMode::Continuous);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) Donner les valeurs (toujours entre 0.0 et 1.0)
  float t = millis() / 1000.0f;
  couleur.set("r", (sin(t) + 1) / 2);
  couleur.set("g", (sin(t + 2) + 1) / 2);
  couleur.set("b", (sin(t + 4) + 1) / 2);
}
```

## Exemple 02 : ENVOYER LA VALEUR D'UN CAPTEUR (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 02 : ENVOYER LA VALEUR D'UN CAPTEUR (ESP32 PoE en Ethernet)
//
//  Lit un potentiomètre (ou tout capteur analogique) et envoie sa valeur de
//  0.0 à 1.0, SEULEMENT quand elle change (mode "Event").
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//  Branchement : milieu du potentiomètre -> broche analogique, côtés -> 3V3 et GND.
//
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/regie/controle/potard1/valeur
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple02";  // lettres et chiffres, 24 max
const int   PIN_CAPTEUR  = 35;           // broche analogique libre de votre carte (voir son schéma)
// ----------------------------------------------------------------------------

FloatOut potard;  // notre "tuyau" d'envoi de nombres

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Regie, "controle", "potard1", "valeur");

  // 3) Créer l'envoi : le topic, le nom de la valeur, 30 envois par seconde au maximum,
  //    en mode Event = on n'envoie que si la valeur change
  potard = CexiiMqtt.floatOut(topic, { "val" }, 30, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) Lire le capteur : analogRead donne 0 à 4095 -> on le ramène entre 0.0 et 1.0
  float valeur = analogRead(PIN_CAPTEUR) / 4095.0f;

  // 5) Arrondir à 2 décimales : sans ça, le petit "bruit" du capteur
  //    ferait envoyer des messages en permanence
  valeur = round(valeur * 100) / 100.0f;

  // 6) Donner la valeur
  potard.set("val", valeur);
}
```

## Exemple 03 : ENVOYER DU TEXTE (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 03 : ENVOYER DU TEXTE (ESP32 PoE en Ethernet)
//
//  Envoie un nouveau texte chaque seconde ("compteur = 1", "compteur = 2"...).
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/ecrans/affichage/ecran1/texte
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple03";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

TextOut message;  // notre "tuyau" d'envoi de texte
unsigned long dernierEnvoi = 0;
int compteur = 0;

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "affichage", "ecran1", "texte");

  // 3) Créer l'envoi : pas de liste de clés pour du texte.
  //    5 envois par seconde au maximum, seulement quand le texte change
  message = CexiiMqtt.textOut(topic, 5, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) Donner un nouveau texte une fois par seconde (128 caractères max)
  if (millis() - dernierEnvoi > 1000) {
    dernierEnvoi = millis();
    compteur++;
    message.set(String("compteur = ") + compteur);
  }
}
```

## Exemple 04 : ENVOYER UNE TABLE (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 04 : ENVOYER UNE TABLE (ESP32 PoE en Ethernet)
//
//  Une table = une liste de rangées, chaque rangée une liste de TEXTES
//  (même les nombres). Maximum 64 rangées x 16 colonnes, 8 Ko.
//
//      colonne 0   colonne 1   colonne 2
//      "a"         "0.00"      "off"        <- rangée 0
//      "b"         "0.00"      "off"        <- rangée 1
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/ecrans/tableau/liste/contenu
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple04";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

TableOut tableau;  // notre "tuyau" d'envoi de table
unsigned long dernierChangement = 0;
bool etat = false;

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "tableau", "liste", "contenu");

  // 3) Créer l'envoi : 5 envois par seconde au maximum, seulement quand une case change
  tableau = CexiiMqtt.tableOut(topic, 5, SendMode::Event);

  // 4) Remplir la table de départ : addRow ajoute une rangée à la fin
  tableau.addRow({ "a", "0.00", "off" });  // rangée 0
  tableau.addRow({ "b", "0.00", "off" });  // rangée 1
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 5) Changer des cases toutes les 2 secondes : set(rangée, colonne, valeur)
  if (millis() - dernierChangement > 2000) {
    dernierChangement = millis();
    etat = !etat;

    tableau.set(0, 1, (sin(millis() / 1000.0f) + 1) / 2);  // un nombre devient du texte
    tableau.set(0, 2, etat ? "on" : "off");
    tableau.set(1, 2, etat ? "off" : "on");

    // Autres outils :  tableau.clear();   vide toute la table
    //                  tableau.trigger(); renvoie la table même sans changement
  }
}
```

## Exemple 05 : RECEVOIR DES NOMBRES (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 05 : RECEVOIR DES NOMBRES (ESP32 PoE en Ethernet)
//
//  Allume une LED selon une valeur reçue (0.0 = éteinte, 1.0 = pleine).
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau
//  + une LED (avec une résistance) sur une broche libre.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour tester, avec MQTTX publiez sur : saillance/jam/lumiere/plafonnier/niveau
//
//      {"v":1,"ts":0,"seq":0,"val":[0.7],"f":"float","label":["niv"]}
//
//  "niv" est le nom de la valeur (la clé). Il doit être le même chez
//  celui qui envoie et chez celui qui reçoit.
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple05";  // lettres et chiffres, 24 max
const int   PIN_LED      = 4;            // broche de la LED (vérifiez le schéma de votre carte)
// ----------------------------------------------------------------------------

Input niveau;  // notre "tuyau" de réception

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où recevoir : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Jam, "lumiere", "plafonnier", "niveau");

  // 3) S'abonner à ce topic : tout ce qui y arrive sera gardé dans "niveau"
  niveau = CexiiMqtt.input(topic);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) isNew() est vrai une seule fois pour chaque nouveau message
  if (niveau.isNew()) {
    Serial.printf("niv = %.2f\n", niveau.get("niv"));
  }

  // 5) get("niv") donne la DERNIÈRE valeur reçue (0.0 tant que rien n'est arrivé)
  analogWrite(PIN_LED, (int)(niveau.get("niv") * 255));
}
```

## Exemple 06 : RECEVOIR DU TEXTE (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 06 : RECEVOIR DU TEXTE (ESP32 PoE en Ethernet)
//
//  Affiche chaque texte reçu, avec une fonction appelée automatiquement.
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour tester, avec MQTTX publiez sur : saillance/ecrans/affichage/ecran1/texte
//
//      {"v":1,"ts":0,"seq":0,"val":"bonjour","f":"string"}
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple06";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où recevoir : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "affichage", "ecran1", "texte");

  // 3) S'abonner, avec une fonction qui sera appelée toute seule à chaque message.
  //    Tout ce qui est entre { } s'exécute à chaque message reçu sur ce topic.
  //    "m" est le message : m.isText dit si c'est du texte, m.text est le texte.
  CexiiMqtt.input(topic, [](const Message& m) {
    if (!m.isText) {
      Serial.println("Ce message n'est pas du texte");
      return;
    }
    Serial.printf("Texte reçu : %s\n", m.text);
  });
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE : c'est update() qui appelle la fonction ci-dessus
}
```

## Exemple 07 : RECEVOIR UNE TABLE (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 07 : RECEVOIR UNE TABLE (ESP32 PoE en Ethernet)
//
//  Affiche chaque table reçue, rangée par rangée.
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour tester, avec MQTTX publiez sur : saillance/ecrans/tableau/liste/contenu
//
//      {"v":1,"ts":0,"seq":0,"val":[["a","0.1","on"],["b","0.5","off"]],"f":"table"}
//
//  La case de la rangée y, colonne x : tableau.cell(y, x)   (du texte)
//  Pour lire une case comme un nombre : tableau.cellFloat(y, x)
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple07";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

Input tableau;  // notre "tuyau" de réception

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où recevoir : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "tableau", "liste", "contenu");

  // 3) S'abonner à ce topic : la dernière table reçue sera gardée dans "tableau"
  tableau = CexiiMqtt.input(topic);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) Quand une nouvelle table arrive, l'afficher
  if (tableau.isNew() && tableau.isTable()) {
    Serial.printf("--- table reçue : %u rangées ---\n", (unsigned)tableau.rowCount());

    for (size_t y = 0; y < tableau.rowCount(); y++) {     // chaque rangée
      for (size_t x = 0; x < tableau.colCount(y); x++) {  // chaque colonne de la rangée
        Serial.printf("[%s] ", tableau.cell(y, x));
      }
      Serial.println();
    }

    // 5) Exemple : la case rangée 0, colonne 1 lue comme un nombre
    Serial.printf("rangée 0, colonne 1 = %.2f\n", tableau.cellFloat(0, 1));
  }
}
```

## Exemple 08 : TOUS LES FORMATS (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 08 : TOUS LES FORMATS (ESP32 PoE en Ethernet)
//
//  Envoie les 3 formats (un topic par format) ET reçoit les 3 formats.
//
//  ENVOI :
//      saillance/globale/test/float/couleur   (nombres, en continu)
//      saillance/globale/test/texte/message   (texte, chaque seconde)
//      saillance/globale/test/table/liste     (table, toutes les 2 secondes)
//  Dans MQTTX, abonnez-vous à : saillance/globale/test/#
//
//  RECEPTION : tout ce qui arrive sur saillance/globale/lumiere/bande2/couleur
//  est affiché, que ce soit des nombres, du texte ou une table.
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple08";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

FloatOut couleur;  // format 1 : nombres
TextOut  message;  // format 2 : texte
TableOut tableau;  // format 3 : table

unsigned long dernierTexte = 0;
unsigned long dernierTableau = 0;
int compteur = 0;
bool etat = false;

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir les topics : un par format à envoyer, un pour recevoir
  Topic topicFloat = CexiiMqtt.topic(Zone::Globale, "test", "float", "couleur");
  Topic topicTexte = CexiiMqtt.topic(Zone::Globale, "test", "texte", "message");
  Topic topicTable = CexiiMqtt.topic(Zone::Globale, "test", "table", "liste");
  Topic topicReception = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande2", "couleur");

  // 3) Créer les envois
  couleur = CexiiMqtt.floatOut(topicFloat, { "r", "g", "b" }, 20, SendMode::Continuous);
  message = CexiiMqtt.textOut(topicTexte, 5, SendMode::Event);
  tableau = CexiiMqtt.tableOut(topicTable, 5, SendMode::Event);
  tableau.addRow({ "a", "0.00", "off" });
  tableau.addRow({ "b", "0.00", "off" });
  tableau.addRow({ "c", "0", "off" });

  // 4) Créer la réception : la fonction traite les trois formats possibles
  CexiiMqtt.input(topicReception, [](const Message& m) {
    if (m.isTable) {
      Serial.printf("table : %u rangées\n", (unsigned)m.rowCount());
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
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()
  float t = millis() / 1000.0f;

  // 5) Nombres : trois ondes (toujours entre 0.0 et 1.0)
  couleur.set("r", (sin(t) + 1) / 2);
  couleur.set("g", (sin(t + 2) + 1) / 2);
  couleur.set("b", (sin(t + 4) + 1) / 2);

  // 6) Texte : un nouveau message chaque seconde
  if (millis() - dernierTexte > 1000) {
    dernierTexte = millis();
    compteur++;
    message.set(String("compteur = ") + compteur);
  }

  // 7) Table : des cases qui changent toutes les 2 secondes
  if (millis() - dernierTableau > 2000) {
    dernierTableau = millis();
    etat = !etat;
    tableau.set(0, 1, (sin(t) + 1) / 2);
    tableau.set(1, 1, (cos(t) + 1) / 2);
    tableau.set(2, 1, compteur);
    tableau.set(0, 2, etat ? "on" : "off");
    tableau.set(1, 2, etat ? "off" : "on");
    tableau.set(2, 2, etat ? "on" : "off");
  }
}
```

## Exemple 09 : UN BOUTON (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 09 : UN BOUTON (ESP32 PoE en Ethernet)
//
//  Le bouton de la carte envoie 1.0 quand on l'enfonce et 0.0 quand on le relâche.
//  Mode "Event" : un message n'est envoyé que lorsque l'état du bouton change.
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//  Avec le username "esp32poeiso_controlbouton" ci-dessous.
//
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/regie/interrupteur/bouton1/etat
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "controlbouton";  // lettres et chiffres, 24 max
const int   PIN_BOUTON   = 34;               // bouton "BUT1" de la carte Olimex
// ----------------------------------------------------------------------------

FloatOut bouton;  // notre "tuyau" d'envoi de nombres

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOUTON, INPUT);

  // 1) Démarrer : type de carte et nom de l'appareil (carte sans "ISO" : Board::Esp32Poe)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Regie, "interrupteur", "bouton1", "etat");

  // 3) Créer l'envoi : une seule valeur "on", 30 envois par seconde au maximum,
  //    seulement quand elle change
  bouton = CexiiMqtt.floatOut(topic, { "on" }, 30, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) Donner l'état du bouton. Sur cette carte, il lit LOW (0) quand il est enfoncé.
  bouton.set("on", digitalRead(PIN_BOUTON) == LOW ? 1.0f : 0.0f);
}
```

## Exemple 10 : ÉTAT DE LA CONNEXION ET ERREURS (ESP32 PoE en Ethernet)

```cpp
// ============================================================================
//  Exemple 10 : ÉTAT DE LA CONNEXION ET ERREURS (ESP32 PoE en Ethernet)
//
//  - CexiiMqtt.connected()   : la carte est-elle connectée au serveur ?
//  - CexiiMqtt.username()    : le nom utilisé sur le serveur
//  - CexiiMqtt.setAppName()  : le nom de votre programme (apparaît dans "infos")
//  - CexiiMqtt.reportError() : envoyer VOTRE propre erreur
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Les erreurs vont sur : saillance/sys/<username>/error
//  Dans MQTTX, abonnez-vous à : saillance/sys/#
//  (les messages "infos" et "ping" y arrivent aussi, tout seuls)
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple10";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

unsigned long dernierAffichage = 0;
bool erreurEnvoyee = false;

void setup() {
  Serial.begin(115200);

  // 1) Donner un nom à votre programme (AVANT begin). Il apparaît dans le champ "app".
  CexiiMqtt.setAppName("exemple10_v1.0");

  // 2) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 3) Toutes les 5 secondes : dire où on en est
  if (millis() - dernierAffichage > 5000) {
    dernierAffichage = millis();
    if (CexiiMqtt.connected()) {
      Serial.printf("Connecté au serveur en tant que %s\n", CexiiMqtt.username());
    } else {
      Serial.println("Pas encore connecté...");
    }
  }

  // 4) Après 15 secondes, envoyer une erreur de test (une seule fois).
  //    Arguments : qui parle, le message, la gravité (Info, Warning, Error ou Critical)
  if (!erreurEnvoyee && millis() > 15000) {
    erreurEnvoyee = true;
    CexiiMqtt.reportError("capteur", "lecture impossible (test)", Severity::Warning);
  }
}
```

## Exemple 11 : EN WIFI (ESP32 classique, sans câble réseau)

```cpp
// ============================================================================
//  Exemple 11 : EN WIFI (ESP32 classique, sans câble réseau)
//
//  Le seul exemple en WiFi. Il fait la même chose que l'exemple 01 :
//  envoie 3 valeurs (r, g, b) qui changent, 20 fois par seconde.
//  Tous les autres exemples fonctionnent aussi en WiFi : il suffit de remplacer
//  leur ligne "beginEthernet" par la ligne "beginWifi" de l'étape 1 ci-dessous.
//
//  Matériel : n'importe quelle ESP32 (aussi les cartes PoE, sans câble).
//  Dans l'Arduino IDE : Outils > Type de carte > "ESP32 Dev Module"
//
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/globale/lumiere/bande1/couleur
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple11";    // lettres et chiffres, 24 max
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
// ----------------------------------------------------------------------------

FloatOut couleur;  // notre "tuyau" d'envoi de nombres

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte, nom de l'appareil et réseau WiFi
  //    Le username sera "esp32_exemple11". Pour une carte PoE en WiFi : Board::Esp32PoeIso
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande1", "couleur");

  // 3) Créer l'envoi : le topic, le nom de chaque valeur,
  //    20 envois par seconde au maximum, en continu
  couleur = CexiiMqtt.floatOut(topic, { "r", "g", "b" }, 20, SendMode::Continuous);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) Donner les valeurs (toujours entre 0.0 et 1.0)
  float t = millis() / 1000.0f;
  couleur.set("r", (sin(t) + 1) / 2);
  couleur.set("g", (sin(t + 2) + 1) / 2);
  couleur.set("b", (sin(t + 4) + 1) / 2);
}
```
