// ============================================================================
//  Exemple 01 : ENVOYER DES NOMBRES
//
//  Envoie 3 valeurs (r, g, b) qui changent, 20 fois par seconde.
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/globale/lumiere/bande1/couleur
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple01";   // lettres et chiffres, 24 max
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
// ----------------------------------------------------------------------------

FloatOut couleur;  // notre "tuyau" d'envoi de nombres

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte, nom de l'appareil, WiFi
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
