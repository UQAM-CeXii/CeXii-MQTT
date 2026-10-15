// Exemple 1 : envoyer 3 valeurs (r, g, b) en WiFi, en continu à 20 fois par seconde.
#include <Saillance.h>

FloatOut couleur;  // notre "tuyau" d'envoi

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte, nom de l'appareil, réseau WiFi.
  //    Le nom : lettres et chiffres seulement, 24 caractères max.
  //    Ici le username sera "esp32_exemple1".
  Saillance.beginWifi(Board::Esp32, "exemple1", "NomDuWifi", "MotDePasse");

  // 2) Choisir où envoyer : zone / module / composante / propriété
  //    (chaque texte : 12 caractères max)
  Topic topic = Saillance.topic(Zone::Salon, "lumiere", "bande1", "couleur");

  // 3) Créer l'envoi : les clés des valeurs (6 caractères max), la fréquence max (1 à 30 Hz),
  //    et le mode : Continuous = en continu, Event = seulement quand ça change.
  couleur = Saillance.floatOut(topic, {"r", "g", "b"}, 20, SendMode::Continuous);
}

void loop() {
  Saillance.update();  // OBLIGATOIRE : à appeler à chaque tour

  // Les valeurs sont toujours entre 0.0 et 1.0
  float t = millis() / 1000.0f;
  couleur.set("r", (sin(t) + 1) / 2);
  couleur.set("g", (sin(t + 2) + 1) / 2);
  couleur.set("b", (sin(t + 4) + 1) / 2);

  delay(5);
}
