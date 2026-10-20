// ============================================================================
//  Exemple 10 : ÉTAT DE LA CONNEXION ET ERREURS (ESP32 PoE en Ethernet)
//
//  - CexiiMqtt.connected()   : la carte est-elle connectée au serveur ?
//  - CexiiMqtt.username()    : le nom utilisé sur le serveur
//  - CexiiMqtt.setAppName()  : le nom de votre programme (apparaît dans "infos")
//  - CexiiMqtt.reportError() : envoyer VOTRE propre erreur
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Les erreurs vont sur : saillance/sys/<username>/error
//  Dans MQTTX, abonnez-vous à : saillance/sys/#
//  (les messages "infos" et "ping" y arrivent aussi, tout seuls)
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple10";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

unsigned long dernierAffichage = 0;
bool erreurEnvoyee = false;

void setup() {
  Serial.begin(115200);

  // 1) Donner un nom à votre programme (AVANT begin). Il apparaît dans le champ "app".
  CexiiMqtt.setAppName("exemple10_v1.0");

  // 2) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 3) Toutes les 5 secondes : dire où on en est
  if (millis() - dernierAffichage > 5000) {
    dernierAffichage = millis();
    if (CexiiMqtt.connected()) {
      Serial.printf("Connecté au serveur en tant que %s\n", CexiiMqtt.username());
    } else {
      Serial.println("Pas encore connecté...");
    }
  }

  // 4) Après 15 secondes, envoyer une erreur de test (une seule fois).
  //    Arguments : qui parle, le message, la gravité (Info, Warning, Error ou Critical)
  if (!erreurEnvoyee && millis() > 15000) {
    erreurEnvoyee = true;
    CexiiMqtt.reportError("capteur", "lecture impossible (test)", Severity::Warning);
  }
}
