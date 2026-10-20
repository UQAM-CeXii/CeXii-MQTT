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
