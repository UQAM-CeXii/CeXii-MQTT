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
