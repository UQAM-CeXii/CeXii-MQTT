// ============================================================================
//  Exemple 04 : ENVOYER UNE TABLE
//
//  Une table = une liste de rangées, chaque rangée une liste de TEXTES
//  (même les nombres). Maximum 64 rangées x 16 colonnes, 8 Ko.
//
//      colonne 0   colonne 1   colonne 2
//      "a"         "0.00"      "off"        <- rangée 0
//      "b"         "0.00"      "off"        <- rangée 1
//
//  Dans MQTTX, abonnez-vous à : saillance/ecrans/tableau/liste/contenu
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple04";
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
// ----------------------------------------------------------------------------

TableOut tableau;
unsigned long dernierChangement = 0;
bool etat = false;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);

  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "tableau", "liste", "contenu");

  // 5 envois/s au maximum, seulement quand une case change
  tableau = CexiiMqtt.tableOut(topic, 5, SendMode::Event);

  // Remplir la table : addRow ajoute une rangée à la fin
  tableau.addRow({ "a", "0.00", "off" });  // rangée 0
  tableau.addRow({ "b", "0.00", "off" });  // rangée 1
}

void loop() {
  CexiiMqtt.update();

  if (millis() - dernierChangement > 2000) {  // toutes les 2 secondes
    dernierChangement = millis();
    etat = !etat;

    // set(rangée, colonne, valeur) change une case
    tableau.set(0, 1, (sin(millis() / 1000.0f) + 1) / 2);  // un nombre devient du texte
    tableau.set(0, 2, etat ? "on" : "off");
    tableau.set(1, 2, etat ? "off" : "on");

    // Autres outils :  tableau.clear();  vide toute la table
    //                  tableau.trigger(); renvoie la table même sans changement
  }
}
