// ============================================================================
//  Exemple 03 : ENVOYER DU TEXTE (ESP32 PoE en Ethernet)
//
//  Envoie un nouveau texte chaque seconde ("compteur = 1", "compteur = 2"...).
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/ecrans/affichage/ecran1/texte
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple03";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

TextOut message;  // notre "tuyau" d'envoi de texte
unsigned long dernierEnvoi = 0;
int compteur = 0;

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "affichage", "ecran1", "texte");

  // 3) Créer l'envoi : pas de liste de clés pour du texte.
  //    5 envois par seconde au maximum, seulement quand le texte change
  message = CexiiMqtt.textOut(topic, 5, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) Donner un nouveau texte une fois par seconde (128 caractères max)
  if (millis() - dernierEnvoi > 1000) {
    dernierEnvoi = millis();
    compteur++;
    message.set(String("compteur = ") + compteur);
  }
}
