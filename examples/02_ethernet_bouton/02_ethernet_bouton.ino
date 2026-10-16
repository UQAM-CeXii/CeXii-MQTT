// Exemple 2 : Ethernet filaire (carte ESP32-POE-ISO) + un bouton.
// Mode "Event" : un message n'est envoyé que lorsque l'état du bouton change.
#include <Saillance.h>

const int PIN_BOUTON = 34;  // bouton "BUT1" de la carte Olimex ESP32-POE-ISO

FloatOut bouton;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOUTON, INPUT);

  // Username MQTT : "esp32poeiso_controlbouton"
  Saillance.beginEthernet(Board::Esp32PoeIso, "controlbouton");

  Topic topic = Saillance.topic(Zone::Ecrans, "interrupteur", "bouton1", "etat");
  bouton = Saillance.floatOut(topic, {"on"}, 30, SendMode::Event);
}

void loop() {
  Saillance.update();

  // Sur cette carte le bouton lit 0 quand il est enfoncé.
  bouton.set("on", digitalRead(PIN_BOUTON) == LOW ? 1.0f : 0.0f);
}
