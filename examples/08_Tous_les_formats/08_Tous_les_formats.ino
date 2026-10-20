// ============================================================================
//  Exemple 08 : TOUS LES FORMATS (ESP32 PoE en Ethernet)
//
//  Envoie les 3 formats (un topic par format) ET reçoit les 3 formats.
//
//  ENVOI :
//      saillance/globale/test/float/couleur   (nombres, en continu)
//      saillance/globale/test/texte/message   (texte, chaque seconde)
//      saillance/globale/test/table/liste     (table, toutes les 2 secondes)
//  Dans MQTTX, abonnez-vous à : saillance/globale/test/#
//
//  RECEPTION : tout ce qui arrive sur saillance/globale/lumiere/bande2/couleur
//  est affiché, que ce soit des nombres, du texte ou une table.
//
//  Matériel : carte Olimex ESP32-POE-ISO (ou ESP32-POE) + câble réseau.
//  Dans l'Arduino IDE : Outils > Type de carte > "OLIMEX ESP32-POE-ISO"
// ============================================================================
#include <CexiiMqtt.h>

// ---- À MODIFIER ------------------------------------------------------------
const char* NOM_APPAREIL = "exemple08";  // lettres et chiffres, 24 max
// ----------------------------------------------------------------------------

FloatOut couleur;  // format 1 : nombres
TextOut  message;  // format 2 : texte
TableOut tableau;  // format 3 : table

unsigned long dernierTexte = 0;
unsigned long dernierTableau = 0;
int compteur = 0;
bool etat = false;

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de l'appareil (le câble suffit, pas de WiFi)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, NOM_APPAREIL);

  // 2) Choisir les topics : un par format à envoyer, un pour recevoir
  Topic topicFloat = CexiiMqtt.topic(Zone::Globale, "test", "float", "couleur");
  Topic topicTexte = CexiiMqtt.topic(Zone::Globale, "test", "texte", "message");
  Topic topicTable = CexiiMqtt.topic(Zone::Globale, "test", "table", "liste");
  Topic topicReception = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande2", "couleur");

  // 3) Créer les envois
  couleur = CexiiMqtt.floatOut(topicFloat, { "r", "g", "b" }, 20, SendMode::Continuous);
  message = CexiiMqtt.textOut(topicTexte, 5, SendMode::Event);
  tableau = CexiiMqtt.tableOut(topicTable, 5, SendMode::Event);
  tableau.addRow({ "a", "0.00", "off" });
  tableau.addRow({ "b", "0.00", "off" });
  tableau.addRow({ "c", "0", "off" });

  // 4) Créer la réception : la fonction traite les trois formats possibles
  CexiiMqtt.input(topicReception, [](const Message& m) {
    if (m.isTable) {
      Serial.printf("table : %u rangées\n", (unsigned)m.rowCount());
      for (size_t y = 0; y < m.rowCount(); y++) {
        for (size_t x = 0; x < m.colCount(y); x++) Serial.printf("[%s] ", m.cell(y, x));
        Serial.println();
      }
    } else if (m.isText) {
      Serial.printf("texte : %s\n", m.text);
    } else {
      for (uint8_t i = 0; i < m.count; i++) Serial.printf("%s = %.2f\n", m.key(i), m.value(i));
    }
  });
}

void loop() {
  CexiiMqtt.update();  // OBLIGATOIRE, à chaque tour de loop()
  float t = millis() / 1000.0f;

  // 5) Nombres : trois ondes (toujours entre 0.0 et 1.0)
  couleur.set("r", (sin(t) + 1) / 2);
  couleur.set("g", (sin(t + 2) + 1) / 2);
  couleur.set("b", (sin(t + 4) + 1) / 2);

  // 6) Texte : un nouveau message chaque seconde
  if (millis() - dernierTexte > 1000) {
    dernierTexte = millis();
    compteur++;
    message.set(String("compteur = ") + compteur);
  }

  // 7) Table : des cases qui changent toutes les 2 secondes
  if (millis() - dernierTableau > 2000) {
    dernierTableau = millis();
    etat = !etat;
    tableau.set(0, 1, (sin(t) + 1) / 2);
    tableau.set(1, 1, (cos(t) + 1) / 2);
    tableau.set(2, 1, compteur);
    tableau.set(0, 2, etat ? "on" : "off");
    tableau.set(1, 2, etat ? "off" : "on");
    tableau.set(2, 2, etat ? "on" : "off");
  }
}
