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
