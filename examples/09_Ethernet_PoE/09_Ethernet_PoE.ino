// ============================================================================
//  Exemple 09 : ETHERNET FILAIRE (cartes Olimex ESP32-POE / ESP32-POE-ISO)
//
//  Comme l'exemple 01 (câble réseau, pas de WiFi), mais avec un bouton.
//  Le bouton envoie 1.0 quand on l'enfonce, 0.0 quand on le relâche.
//
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//  (ou "OLIMEX ESP32-POE" -> alors utilisez Board::Esp32Poe plus bas).
//  Dans MQTTX, abonnez-vous à : saillance/regie/interrupteur/bouton1/etat
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "controlbouton";  // le username sera "esp32poeiso_controlbouton"
const int   PIN_BOUTON   = 34;               // bouton "BUT1" de la carte
// ----------------------------------------------------------------------------

FloatOut bouton;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOUTON, INPUT);

  // Pas de WiFi ici : le câble suffit. (Board::Esp32Poe pour la carte sans "ISO")
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  Topic topic = CexiiMqtt.topic(Zone::Regie, "interrupteur", "bouton1", "etat");
  bouton = CexiiMqtt.floatOut(topic, { "on" }, 30, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();

  // Sur cette carte, le bouton lit LOW (0) quand il est enfoncé.
  bouton.set("on", digitalRead(PIN_BOUTON) == LOW ? 1.0f : 0.0f);
}
