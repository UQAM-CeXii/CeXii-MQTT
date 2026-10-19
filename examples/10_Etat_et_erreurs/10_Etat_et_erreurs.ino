// ============================================================================
//  Exemple 10 : ÉTAT DE LA CONNEXION ET ERREURS
//
//  - CexiiMqtt.connected()  : la carte est-elle connectée au broker ?
//  - CexiiMqtt.username()   : le nom utilisé sur le broker
//  - CexiiMqtt.setAppName() : le nom de votre programme (apparaît dans "infos")
//  - CexiiMqtt.reportError(): envoyer VOTRE propre erreur
//
//  Les erreurs vont sur : saillance/sys/<username>/error
//  Dans MQTTX, abonnez-vous à : saillance/sys/#
//  (les messages "infos" et "ping" y arrivent aussi, tout seuls)
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple10";
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
// ----------------------------------------------------------------------------

unsigned long dernierAffichage = 0;
bool erreurEnvoyee = false;

void setup() {
  Serial.begin(115200);

  CexiiMqtt.setAppName("exemple10_v1.0");  // AVANT beginWifi. Apparaît dans le champ "app"
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);
}

void loop() {
  CexiiMqtt.update();

  // Toutes les 5 secondes : dire où on en est
  if (millis() - dernierAffichage > 5000) {
    dernierAffichage = millis();
    if (CexiiMqtt.connected()) {
      Serial.printf("Connecté au broker en tant que %s\n", CexiiMqtt.username());
    } else {
      Serial.println("Pas encore connecté...");
    }
  }

  // Après 15 secondes, envoyer une erreur de test (une seule fois)
  if (!erreurEnvoyee && millis() > 15000) {
    erreurEnvoyee = true;
    // source (qui parle), message, gravité : Info, Warning, Error ou Critical
    CexiiMqtt.reportError("capteur", "lecture impossible (test)", Severity::Warning);
  }
}
