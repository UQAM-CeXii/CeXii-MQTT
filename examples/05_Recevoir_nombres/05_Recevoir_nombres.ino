// ============================================================================
//  Exemple 05 : RECEVOIR DES NOMBRES
//
//  Allume la LED de la carte selon une valeur reçue (0.0 = éteinte, 1.0 = pleine).
//  Pour tester, avec MQTTX publiez sur : saillance/jam/lumiere/plafonnier/niveau
//
//      {"v":1,"ts":0,"seq":0,"val":[0.7],"f":"float","label":["niv"]}
//
//  "niv" est le nom de la valeur (la clé). Il doit être le même chez
//  celui qui envoie et chez celui qui reçoit.
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple05";
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
const int   PIN_LED           = 2;  // LED de la plupart des cartes ESP32
// ----------------------------------------------------------------------------

Input niveau;  // notre "tuyau" de réception

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);

  Topic topic = CexiiMqtt.topic(Zone::Jam, "lumiere", "plafonnier", "niveau");
  niveau = CexiiMqtt.input(topic);  // on s'abonne à ce topic
}

void loop() {
  CexiiMqtt.update();

  // isNew() : vrai une seule fois pour chaque nouveau message
  if (niveau.isNew()) {
    Serial.printf("niv = %.2f\n", niveau.get("niv"));
  }

  // get("niv") donne la DERNIÈRE valeur reçue (0.0 tant que rien n'est arrivé)
  analogWrite(PIN_LED, (int)(niveau.get("niv") * 255));
}
