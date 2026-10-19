---
name: cexii-mqtt
description: Écrire, expliquer et dépanner du code pour la bibliothèque Arduino/PlatformIO « CeXii MQTT » (objet CexiiMqtt, ancien nom Saillance) qui envoie et reçoit des messages MQTT depuis un ESP32 ou un ESP32 PoE (Olimex ESP32-POE / POE-ISO) vers le broker EMQX en WebSocket, en WiFi ou en Ethernet. Utiliser dès que la personne parle de CeXii MQTT, CexiiMqtt, FloatOut, TextOut, TableOut, Input, de topics saillance/zone/module/composante/propriété, des zones jam, ecrans, globale ou regie, des topics saillance/sys/…/infos, error ou ping, ou qu'elle veut qu'un ESP32 envoie ou reçoive des valeurs (0 à 1), du texte ou une table vers TouchDesigner ou EMQX, même sans nommer la bibliothèque. Also use for ESP32 MQTT sketches in this project, Arduino IDE or PlatformIO.
---

# CeXii MQTT : aider à utiliser la bibliothèque

La bibliothèque permet à un ESP32 d'envoyer et de recevoir des messages MQTT sans connaître MQTT.
Les personnes qui l'utilisent sont souvent débutantes en PlatformIO et en C++ : la simplicité d'abord,
des commentaires clairs en français, des exemples qui marchent tels quels.

## Les faits à connaître

- `#include <CexiiMqtt.h>` ; l'objet global s'appelle `CexiiMqtt` (l'ancien nom était `Saillance`, ne plus l'utiliser).
- Le premier niveau des **topics** reste `saillance/` : `saillance/<zone>/<module>/<composante>/<propriété>`,
  sans `/` au début. Ce n'est pas une faute, c'est le nom du projet dans l'adresse des messages.
- Broker : `ws://10.0.21.210:8083/mqtt` (EMQX, MQTT sur WebSocket). Réglable dans `src/CexiiMqttConfig.h`.
- Cartes : `Board::Esp32` (WiFi seulement), `Board::Esp32Poe`, `Board::Esp32PoeIso` (WiFi + Ethernet).
- Testé avec Arduino-ESP32 2.0.17 (PlatformIO `espressif32` 7.x) et ArduinoJson 7. La version 0.1.0 n'a pas
  été éprouvée à fond sur matériel : si un comportement surprend, demander ce que dit le moniteur série (115200).

## Écrire un sketch : la marche à suivre

1. **Clarifier le besoin** (ne demander que ce qui manque) : quelle carte et quel réseau (Ethernet PoE ou WiFi) ?
   Que faut-il envoyer ou recevoir, et sous quelle forme (nombres, texte, table) ? Quelle zone ? Quelle cadence ?
   En continu ou seulement quand ça change ?
2. **Choisir les noms** et les valider avec `scripts/check_names.py` (nom d'appareil, topic, clés) : les règles
   ci-dessous sont strictes et la bibliothèque refuse ce qui ne les respecte pas.
3. **Écrire le sketch** à partir des gabarits de `references/examples.md`. Garder la structure en
   « À MODIFIER » (nom d'appareil, WiFi) en haut du fichier, `CexiiMqtt.update()` en premier dans `loop()`.
4. **Dire comment tester** : dans MQTTX (`ws://10.0.21.210:8083/mqtt`), s'abonner au topic, et pour la
   réception donner le JSON à publier (voir `references/messages.md`).
5. Si ça ne marche pas, passer par `references/troubleshooting.md`.

Réponses en français par défaut. Pour le WiFi, mettre des valeurs génériques (`"NomDuWifi"`, `"MotDePasse"`)
et ne jamais recopier un vrai mot de passe dans un fichier destiné à un dépôt.

## Les règles (la bibliothèque les applique, autant les respecter dès l'écriture)

| Quoi | Règle |
|---|---|
| Nom d'appareil | 1 à 24 caractères, lettres et chiffres seulement (pas de `_`, `-`, espace, accent). Mis en minuscules. Le username devient `<carte>_<nom>` : `esp32poeiso_controlbouton` |
| Zone | choisie dans l'énumération `Zone::` (aujourd'hui `Jam`, `Ecrans`, `Globale`, `Regie`) |
| Module, composante, propriété | 1 à 12 caractères, lettres, chiffres, `_` ou `-`. Mis en minuscules |
| Clé d'une valeur | 1 à 6 caractères, lettres, chiffres, `_`. Déclarée à la création du `FloatOut`. 16 clés max |
| Nombres | toujours entre 0.0 et 1.0 (sinon ramenés dans l'intervalle, avec un avertissement) |
| Cadence max | 1 à 30 envois par seconde |
| Texte | 128 caractères max |
| Table | 64 rangées × 16 colonnes, toutes les cases en texte, 8192 octets pour le message entier |

Les zones peuvent changer : si le dépôt de la bibliothèque est accessible, lire la liste dans
`CEXII_ZONES` de `src/CexiiMqttConfig.h` plutôt que de se fier à cette page.

## Le code en bref

```cpp
#include <CexiiMqtt.h>

FloatOut couleur;

void setup() {
  Serial.begin(115200);
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, "monappareil");          // ou beginWifi(Board::Esp32, "monappareil", ssid, mdp)
  Topic topic = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande1", "couleur");
  couleur = CexiiMqtt.floatOut(topic, { "r", "g", "b" }, 20, SendMode::Continuous);
}

void loop() {
  CexiiMqtt.update();          // obligatoire, à chaque tour
  couleur.set("r", 0.5);
}
```

- `SendMode::Event` : envoie seulement quand une valeur change (la dernière valeur gagne, cadence plafonnée).
  Rien n'est envoyé tant qu'aucune valeur n'a changé : utiliser `trigger()` pour envoyer l'état de départ.
- `SendMode::Continuous` : envoie à la cadence demandée même sans changement.
- Réception : `Input in = CexiiMqtt.input(topic);` puis `in.get("clé")`, `in.isNew()`, `in.text()`,
  `in.cell(y, x)` ; ou `CexiiMqtt.input(topic, [](const Message& m){ … });`. Les fonctions de rappel
  s'exécutent dans `update()`, donc dans `loop()` : pas de souci de concurrence.
- Le détail complet des fonctions est dans `references/api.md`.

## Pourquoi ces points reviennent souvent

- **`update()` oublié ou `delay()` long** : tout (envois, réception, messages système) passe par `update()`.
  Un long `delay()` espace les envois et ralentit la réception.
- **Clé qui ne correspond pas** : la clé `"niv"` côté envoi doit être la même côté réception, sinon `get()` renvoie 0.
- **Topic différent d'une lettre** : la réception compare le topic exact, sans caractères génériques (`#`, `+`).
- **Texte ou table reçus mais rien d'affiché** : `m.count` ne compte que les nombres ; tester `m.isText` et `m.isTable`.
- **Les trois messages système** (rien à écrire) : `saillance/sys/<username>/infos` toutes les 5 s,
  `…/error` à chaque erreur, `…/ping` toutes les 60 s (sert à calculer `lat`). Le compte EMQX doit pouvoir y
  publier et s'abonner à `…/ping`.

## Modifier la bibliothèque elle-même

Seulement si on le demande. Fichiers : `src/CexiiMqtt.h` (API publique), `CexiiMqttCore.*` (logique pure,
testée sur ordinateur), `CexiiMqttNet.cpp` (WiFi/Ethernet), `CexiiMqttClient.cpp` (esp-mqtt),
`CexiiMqttSystem.cpp` (error/infos/ping). Tests : `pio test -e native`. Compiler un exemple :
`cd examples/01_Envoyer_nombres && pio run`. Toute limite ou zone modifiée dans `CexiiMqttConfig.h` doit
aussi être répercutée dans `README.md`, `docs/REFERENCE.md` et ce skill.
Piège connu à ne pas réintroduire : envoyer avec `esp_mqtt_client_enqueue` (file interne d'esp-mqtt qui ne
libère qu'un message par seconde et jette tout après 30 s) ; la bibliothèque utilise `esp_mqtt_client_publish`.
