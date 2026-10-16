// Exemple 5 : une table = une liste de rangées, chaque rangée une liste de textes.
//   rangée 0 : "a", "0.1", "on"
//   rangée 1 : "b", "0.5", "off"
// Maximum 64 rangées x 16 colonnes, 8 Ko par message.
#include <Saillance.h>

TableOut tableau;
unsigned long dernier = 0;
bool allume = false;

void setup() {
  Serial.begin(115200);
  Saillance.beginWifi(Board::Esp32, "exemple5", "NomDuWifi", "MotDePasse");

  // ENVOI : une table sur ce topic (5 envois/s max, seulement quand elle change)
  Topic topicEnvoi = Saillance.topic(Zone::Regie, "cues", "liste", "table");
  tableau = Saillance.tableOut(topicEnvoi, 5, SendMode::Event);

  tableau.addRow({ "a", "0.1", "on" });   // rangée 0
  tableau.addRow({ "b", "0.5", "off" });  // rangée 1

  // RECEPTION : on est appelé à chaque table reçue
  Topic topicReception = Saillance.topic(Zone::Regie, "cues", "retour", "table");
  Saillance.input(topicReception, [](const Message& m) {
    if (!m.isTable) return;
    for (size_t y = 0; y < m.rowCount(); y++) {
      for (size_t x = 0; x < m.colCount(y); x++) Serial.printf("[%s] ", m.cell(y, x));
      Serial.println();
    }
  });
}

void loop() {
  Saillance.update();

  if (millis() - dernier > 2000) {
    dernier = millis();
    allume = !allume;
    tableau.set(1, 2, allume ? "on" : "off");  // rangée 1, colonne 2 -> envoyée car elle change
    tableau.set(1, 1, 0.25f);                  // un nombre est converti en texte : "0.25"
  }
}
