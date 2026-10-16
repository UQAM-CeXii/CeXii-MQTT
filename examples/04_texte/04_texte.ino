// Exemple 4 : envoyer du texte (au lieu de nombres), une fois par seconde.
#include <Saillance.h>

TextOut message;
unsigned long dernier = 0;
int compteur = 0;

void setup() {
  Serial.begin(115200);
  Saillance.beginWifi(Board::Esp32, "exemple4", "NomDuWifi", "MotDePasse");

  Topic topic = Saillance.topic(Zone::Ecrans, "affichage", "ecran1", "texte");
  message = Saillance.textOut(topic, 5, SendMode::Event);  // 5 fois par seconde max
}

void loop() {
  Saillance.update();

  if (millis() - dernier > 1000) {
    dernier = millis();
    message.set(String("compteur = ") + compteur++);  // envoyé car le texte a changé
  }
}
