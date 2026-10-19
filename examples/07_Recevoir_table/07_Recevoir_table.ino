// ============================================================================
//  Exemple 07 : RECEVOIR UNE TABLE
//
//  Affiche chaque table reçue, rangée par rangée.
//  Pour tester, avec MQTTX publiez sur : saillance/ecrans/tableau/liste/contenu
//
//      {"v":1,"ts":0,"seq":0,"val":[["a","0.1","on"],["b","0.5","off"]],"f":"table"}
//
//  La case de la rangée y, colonne x : tableau.cell(y, x)   (du texte)
//  Pour lire une case comme un nombre : tableau.cellFloat(y, x)
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL      = "exemple07";
const char* WIFI_NOM          = "NomDuWifi";
const char* WIFI_MOT_DE_PASSE = "MotDePasse";
// ----------------------------------------------------------------------------

Input tableau;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginWifi(Board::Esp32, NOM_APPAREIL, WIFI_NOM, WIFI_MOT_DE_PASSE);

  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "tableau", "liste", "contenu");
  tableau = CexiiMqtt.input(topic);
}

void loop() {
  CexiiMqtt.update();

  if (tableau.isNew() && tableau.isTable()) {
    Serial.printf("--- table reçue : %u rangées ---\n", (unsigned)tableau.rowCount());

    for (size_t y = 0; y < tableau.rowCount(); y++) {          // chaque rangée
      for (size_t x = 0; x < tableau.colCount(y); x++) {       // chaque colonne
        Serial.printf("[%s] ", tableau.cell(y, x));
      }
      Serial.println();
    }

    // Exemple : la case rangée 0, colonne 1 lue comme un nombre
    Serial.printf("rangée 0, colonne 1 = %.2f\n", tableau.cellFloat(0, 1));
  }
}
