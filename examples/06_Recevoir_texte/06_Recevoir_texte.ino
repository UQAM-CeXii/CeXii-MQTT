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
