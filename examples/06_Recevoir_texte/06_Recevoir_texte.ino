// ============================================================================
//  Exemple 06 : RECEVOIR DU TEXTE (avec une fonction automatique)
//
//  La fonction ci-dessous est appelée toute seule à chaque message reçu.
//  Pour tester, avec MQTTX publiez sur : saillance/ecrans/affichage/ecran1/texte
//
//      {"v":1,"ts":0,"seq":0,"val":"bonjour","f":"string"}
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple06";
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
// ----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);

  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "affichage", "ecran1", "texte");

  // Tout ce qui est entre { } s'exécute à chaque message reçu sur ce topic.
  // "m" est le message : m.isText dit si c'est du texte, m.text est le texte.
  CexiiMqtt.input(topic, [](const Message& m) {
    if (!m.isText) {
      Serial.println("Ce message n'est pas du texte");
      return;
    }
    Serial.printf("Texte reçu : %s\n", m.text);
  });
}

void loop() {
  CexiiMqtt.update();  // c'est update() qui appelle la fonction ci-dessus
}
