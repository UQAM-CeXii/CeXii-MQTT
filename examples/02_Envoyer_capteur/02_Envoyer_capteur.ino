// ============================================================================
//  Exemple 02 : ENVOYER LA VALEUR D'UN CAPTEUR
//
//  Lit un potentiomètre (ou tout capteur analogique) et envoie sa valeur
//  de 0.0 à 1.0, SEULEMENT quand elle change (mode "Event").
//
//  Branchement : milieu du potentiomètre -> broche 34, côtés -> 3V3 et GND.
//  Dans MQTTX, abonnez-vous à : saillance/regie/controle/potard1/valeur
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple02";
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
const int   PIN_CAPTEUR       = 34;  // broche analogique (32, 33, 34, 35, 36 ou 39)
// ----------------------------------------------------------------------------

FloatOut potard;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);

  Topic topic = CexiiMqtt.topic(Zone::Regie, "controle", "potard1", "valeur");

  // Mode Event : on n'envoie que si la valeur change (30 fois par seconde au maximum)
  potard = CexiiMqtt.floatOut(topic, { "val" }, 30, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();

  // analogRead donne 0 à 4095 -> on le ramène entre 0.0 et 1.0
  float valeur = analogRead(PIN_CAPTEUR) / 4095.0f;

  // On arrondit à 2 décimales : sans ça, le petit "bruit" du capteur
  // ferait envoyer des messages en permanence.
  valeur = round(valeur * 100) / 100.0f;

  potard.set("val", valeur);
}
