// ============================================================================
//  Exemple 04 : ENVOYER UNE TABLE (ESP32 PoE en Ethernet)
//
//  Une table = une liste de rangées, chaque rangée une liste de TEXTES
//  (même les nombres). Maximum 64 rangées x 16 colonnes, 8 Ko.
//
//      colonne 0   colonne 1   colonne 2
//      "a"         "0.00"      "off"        <- rangée 0
//      "b"         "0.00"      "off"        <- rangée 1
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour voir le résultat, dans MQTTX abonnez-vous à :
//      saillance/ecrans/tableau/liste/contenu
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple04";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

TableOut tableau;  // notre "tuyau" d'envoi de table
unsigned long dernierChangement = 0;
bool etat = false;

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "tableau", "liste", "contenu");

  // 3) Créer l'envoi : 5 envois par seconde au maximum, seulement quand une case change
  tableau = CexiiMqtt.tableOut(topic, 5, SendMode::Event);

  // 4) Remplir la table de départ : addRow ajoute une rangée à la fin
  tableau.addRow({ "a", "0.00", "off" });  // rangée 0
  tableau.addRow({ "b", "0.00", "off" });  // rangée 1
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 5) Changer des cases toutes les 2 secondes : set(rangée, colonne, valeur)
  if (millis() - dernierChangement > 2000) {
    dernierChangement = millis();
    etat = !etat;

    tableau.set(0, 1, (sin(millis() / 1000.0f) + 1) / 2);  // un nombre devient du texte
    tableau.set(0, 2, etat ? "on" : "off");
    tableau.set(1, 2, etat ? "off" : "on");

    // Autres outils :  tableau.clear();   vide toute la table
    //                  tableau.trigger(); renvoie la table même sans changement
  }
}
