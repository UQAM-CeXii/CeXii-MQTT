// Exemple 3 : recevoir des valeurs. Deux façons de faire.
#include <Saillance.h>

Input luminosite;  // façon 1 : on va lire la dernière valeur quand on en a besoin
const int PIN_LED = 2;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);

  Saillance.beginWifi(Board::Esp32, "exemple3", "NomDuWifi", "MotDePasse");

  // Façon 1 : garder le "tuyau" et lire avec get()
  Topic t1 = Saillance.topic(Zone::Salon, "lumiere", "plafonnier", "niveau");
  luminosite = Saillance.input(t1);

  // Façon 2 : une fonction appelée automatiquement à chaque message reçu
  Topic t2 = Saillance.topic(Zone::Scene, "fumee", "machine", "debit");
  Saillance.input(t2, [](const Message& m) {
    Serial.printf("debit de fumee recu : %.2f\n", m.get("debit"));
    for (uint8_t i = 0; i < m.count; i++)  // ou tout lire, clé par clé
      Serial.printf("  %s = %.2f\n", m.key(i), m.value(i));
  });
}

void loop() {
  Saillance.update();

  if (luminosite.hasData()) {
    // Valeur de la clé "niv" (0.0 à 1.0) -> PWM 0 à 255. 0.0 si rien reçu.
    analogWrite(PIN_LED, (int)(luminosite.get("niv") * 255));
  }
}
