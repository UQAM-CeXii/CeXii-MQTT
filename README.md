# Saillance (bibliothèque MQTT pour ESP32 / ESP32 PoE)

Envoyer et recevoir des messages MQTT vers le broker EMQX de Saillance
(`ws://10.0.21.210:8083`), en **WiFi** ou en **Ethernet filaire**, sans avoir à connaître MQTT.

> Version de démarrage. La documentation complète viendra après les tests sur les cartes.

## 1. Installer

### Avec l'Arduino IDE

1. **Outils > Gestionnaire de cartes** : installer « esp32 » (Espressif). Testé avec la version 2.0.17.
2. **Outils > Gérer les bibliothèques** : installer « ArduinoJson » (version 7 ou plus).
3. Installer Saillance : télécharger le dépôt en `.zip`, puis **Croquis > Inclure une bibliothèque > Ajouter la bibliothèque .ZIP**
   (ou copier le dossier dans `Documents/Arduino/libraries/`).
4. **Outils > Type de carte** : « ESP32 Dev Module », ou « OLIMEX ESP32-POE-ISO » / « OLIMEX ESP32-POE ».
5. **Outils > Core Debug Level > Error** : pour que les erreurs `log_e()` remontent aussi vers MQTT.
6. Ouvrir un exemple : **Fichier > Exemples > Saillance**.

### Avec PlatformIO

Dans le `platformio.ini` de votre projet :

```ini
[env:esp32]
platform = espressif32
board = esp32dev            ; ESP32-POE : esp32-poe     ESP32-POE-ISO : esp32-poe-iso
framework = arduino
monitor_speed = 115200
build_flags = -DCORE_DEBUG_LEVEL=1   ; pour que les erreurs log_e() remontent aussi
lib_deps =
    https://github.com/VOTRE-COMPTE/saillance-mqtt.git
```

Les dossiers `examples/` contiennent 4 exemples complets (Arduino IDE et PlatformIO).

## 2. Premier programme

```cpp
#include <Saillance.h>

FloatOut couleur;

void setup() {
  Serial.begin(115200);

  // Type de carte, nom de l'appareil, WiFi
  Saillance.beginWifi(Board::Esp32, "controlbouton", "NomDuWifi", "MotDePasse");

  // Où envoyer : zone / module / composante / propriété
  Topic topic = Saillance.topic(Zone::Salon, "lumiere", "bande1", "couleur");

  // Les valeurs envoyées, chacune avec sa clé
  couleur = Saillance.floatOut(topic, {"r", "g", "b"}, 20, SendMode::Continuous);
}

void loop() {
  Saillance.update();        // OBLIGATOIRE à chaque tour de loop()

  couleur.set("r", 0.5);     // toujours entre 0.0 et 1.0
  couleur.set("g", 0.2);
  couleur.set("b", 1.0);
}
```

Ouvrez le moniteur série : la bibliothèque affiche où elle en est (WiFi, IP, MQTT connecté…)
et explique les erreurs en français.

### En Ethernet (cartes Olimex PoE)

```cpp
Saillance.beginEthernet(Board::Esp32PoeIso, "controlbouton");   // ou Board::Esp32Poe
```

## 3. Le nom de l'appareil

Le username MQTT est `<carte>_<nom>` : `Board::Esp32PoeIso` + `"controlbouton"` →
**`esp32poeiso_controlbouton`**.

| Règle | Détail |
|---|---|
| Longueur du nom | 24 caractères maximum |
| Caractères | lettres et chiffres seulement (pas d'espace, accent, `_`, `-`) |
| Majuscules | converties en minuscules |

Deux appareils avec le même nom se déconnectent l'un l'autre : choisissez des noms uniques.
Mot de passe EMQX, si le broker en exige un : `Saillance.setMqttPassword("...")` avant `begin`.

## 4. Construire un topic

Le topic est `saillance / zone / module / composante / propriété`. Deux façons :

```cpp
// En une ligne
Topic t = Saillance.topic(Zone::Salon, "lumiere", "bande1", "couleur");

// Guidé : l'éditeur propose l'étape suivante après chaque point
Topic t = Saillance.zone(Zone::Salon).module("lumiere").component("bande1").property("couleur");
```

* `Zone` : on choisit dans une liste (voir `SAILLANCE_ZONES` dans `src/SaillanceConfig.h`).
* module / composante / propriété : 12 caractères max, lettres, chiffres, `_` ou `-`. Mis en minuscules.
* Un topic invalide est refusé avec un message qui dit quoi corriger.

## 5. Envoyer

```cpp
FloatOut f = Saillance.floatOut(topic, {"r", "g", "b"}, 30, SendMode::Event);
f.set("r", 0.7);
f.trigger();                         // forcer un envoi même sans changement

TextOut  t = Saillance.textOut(topic2);
t.set("bonjour");
```

* **Clés** : 6 caractères max (lettres, chiffres, `_`). Jusqu'à 16 valeurs par topic.
* **Valeurs** : entre 0 et 1. Hors limites → ramenées dans l'intervalle (avec un avertissement).
* **Fréquence maximale** : de 1 à 30 envois par seconde.
* **Mode** :
  * `SendMode::Event` : un message seulement quand une valeur change. Les changements rapides
    sont regroupés : c'est toujours la dernière valeur qui part.
  * `SendMode::Continuous` : un message à la fréquence demandée, même si rien ne change.

Format envoyé :

```json
{"v":1,"ts":1760000000123,"seq":42,"val":[0.5,0.2,1],"f":"float","label":["r","g","b"]}
{"v":1,"ts":1760000000123,"seq":7,"val":"bonjour","f":"string"}
```

## 6. Recevoir

```cpp
Input niveau;

niveau = Saillance.input(topic);                     // lire quand on veut
float v = niveau.get("niv");                         // 0.0 si rien reçu

Saillance.input(topic, [](const Message& m) {        // ou être appelé à chaque message
  Serial.println(m.get("niv"));
});
```

Vos fonctions sont toujours appelées depuis `Saillance.update()`, donc dans `loop()`.

## 7. Messages automatiques (rien à faire)

| Topic | Quand | Contenu |
|---|---|---|
| `/saillance/sys/<username>/infos` | toutes les 5 s | `v ts seq fps df gpu cpu ac performMode` |
| `/saillance/sys/<username>/error` | à chaque erreur | `v ts seq src msg sv t` |

Erreurs capturées : tout `ESP_LOGE` / `log_e` du système (WiFi, MQTT, mémoire…), perte de WiFi ou
du câble, refus du broker, mémoire basse, erreurs d'utilisation de la bibliothèque, et la cause
du redémarrage précédent (plantage, watchdog, baisse de tension).
Vos propres erreurs : `Saillance.reportError("capteur", "lecture impossible");`

`sv` : 0 info, 1 attention, 2 erreur, 3 critique. `t` : code (voir `ErrorCode` dans `src/SaillanceTypes.h`).

## 8. En cas de problème

| Symptôme | Piste |
|---|---|
| `nom d'appareil ... invalide` | lettres et chiffres seulement, 24 max |
| Rien après « connexion à… » | mauvais ssid / mot de passe WiFi |
| `broker injoignable` | l'ESP32 n'est pas sur le même réseau que `10.0.21.210`, ou le port 8083 est bloqué |
| `connexion refusee par le broker` | username / mot de passe refusé par EMQX |
| Aucune donnée | `Saillance.update()` oublié dans `loop()` |

## Développement

Tests de la logique (sans carte) : `pio test -e native`
