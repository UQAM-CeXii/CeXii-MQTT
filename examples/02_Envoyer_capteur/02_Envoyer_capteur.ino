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
