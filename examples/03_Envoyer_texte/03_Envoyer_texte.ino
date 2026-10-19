// ============================================================================
//  Exemple 03 : ENVOYER DU TEXTE
//
//  Envoie un nouveau texte chaque seconde ("compteur = 1", "compteur = 2"...).
//  Dans MQTTX, abonnez-vous à : saillance/ecrans/affichage/ecran1/texte
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple03";
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
// ----------------------------------------------------------------------------

TextOut message;
unsigned long dernierEnvoi = 0;
int compteur = 0;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);

  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "affichage", "ecran1", "texte");

  // Pas de liste de clés pour du texte. 5 envois/s au maximum, seulement si le texte change.
  message = CexiiMqtt.textOut(topic, 5, SendMode::Event);
}

void loop() {
  CexiiMqtt.update();

  if (millis() - dernierEnvoi > 1000) {  // une fois par seconde
    dernierEnvoi = millis();
    compteur++;
    message.set(String("compteur = ") + compteur);  // 128 caractères max
  }
}
