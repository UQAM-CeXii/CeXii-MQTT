// ============================================================================
//  Exemple 07 : RECEVOIR UNE TABLE (ESP32 PoE en Ethernet)
//
//  Affiche chaque table reçue, rangée par rangée.
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
//
//  Pour tester, avec MQTTX publiez sur : saillance/ecrans/tableau/liste/contenu
//
//      {"v":1,"ts":0,"seq":0,"val":[["a","0.1","on"],["b","0.5","off"]],"f":"table"}
//
//  La case de la rangée y, colonne x : tableau.cell(y, x)   (du texte)
//  Pour lire une case comme un nombre : tableau.cellFloat(y, x)
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple07";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

Input tableau;  // notre "tuyau" de réception

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir où recevoir : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Ecrans, "tableau", "liste", "contenu");

  // 3) S'abonner à ce topic : la dernière table reçue sera gardée dans "tableau"
  tableau = CexiiMqtt.input(topic);
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()

  // 4) Quand une nouvelle table arrive, l'afficher
  if (tableau.isNew() && tableau.isTable()) {
    Serial.printf("--- table reçue : %u rangées ---\n", (unsigned)tableau.rowCount());

    for (size_t y = 0; y < tableau.rowCount(); y++) {     // chaque rangée
      for (size_t x = 0; x < tableau.colCount(y); x++) {  // chaque colonne de la rangée
        Serial.printf("[%s] ", tableau.cell(y, x));
      }
      Serial.println();
    }

    // 5) Exemple : la case rangée 0, colonne 1 lue comme un nombre
    Serial.printf("rangée 0, colonne 1 = %.2f\n", tableau.cellFloat(0, 1));
  }
}
