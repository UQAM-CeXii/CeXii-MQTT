# Saillance (bibliothèque MQTT pour ESP32 / ESP32 PoE)

Envoyer et recevoir des messages MQTT vers le broker EMQX de Saillance
(`ws://10.0.21.210:8083`), en **WiFi** ou en **Ethernet filaire**, sans avoir à connaître MQTT.

* Cartes : ESP32 et ESP32 PoE (Olimex ESP32-POE, ESP32-POE-ISO).
* Fonctionne avec l'**Arduino IDE** et avec **PlatformIO**.
* Construite sur `esp-mqtt` (inclus dans le framework ESP32) et `ArduinoJson`.

> Version 0.1.0. Elle compile et ses règles de validation sont testées, mais elle n'a pas encore été
> éprouvée sur les cartes ni contre le broker. Signalez ce qui ne se comporte pas comme décrit ici.

**Sommaire** : [Installer](#1-installer) · [Premier programme](#2-premier-programme) ·
[Ce qui s'affiche](#3-ce-qui-saffiche-dans-le-moniteur-série) · [Nom de l'appareil](#4-le-nom-de-lappareil) ·
[Topics](#5-construire-un-topic) · [Envoyer](#6-envoyer) · [Recevoir](#7-recevoir) ·
[Messages automatiques](#8-messages-automatiques-rien-à-faire) · [Problèmes](#9-en-cas-de-problème) ·
[Configuration](#10-configuration-pour-les-mainteneurs)

---

## 1. Installer

### Avec l'Arduino IDE

1. **Outils > Gestionnaire de cartes** : installer « esp32 » (Espressif). Testé avec la version 2.0.17.
2. **Outils > Gérer les bibliothèques** : installer « ArduinoJson » (version 7 ou plus).
3. Installer Saillance : **Croquis > Inclure une bibliothèque > Ajouter la bibliothèque .ZIP**
   (ou copier le dossier dans `Documents/Arduino/libraries/`).
   Si une ancienne version est déjà installée, supprimez d'abord son dossier.
4. **Outils > Type de carte** : « ESP32 Dev Module », ou « OLIMEX ESP32-POE-ISO » / « OLIMEX ESP32-POE ».
5. **Outils > Core Debug Level > Error** : pour que les erreurs `log_e()` remontent aussi vers MQTT.
6. Ouvrir un exemple : **Fichier > Exemples > Saillance**.
7. **Outils > Moniteur série**, vitesse **115200**.

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

### Les exemples

| Dossier | Ce qu'il montre |
|---|---|
| `01_envoi_wifi` | envoyer 3 valeurs (r, g, b) en continu, en WiFi |
| `02_ethernet_bouton` | Ethernet filaire sur ESP32-POE-ISO, un bouton en mode événement |
| `03_reception` | recevoir des valeurs, par lecture et par fonction appelée automatiquement |
| `04_texte` | envoyer du texte |
| `05_table` | envoyer et recevoir une table (liste de rangées de textes) |

---

## 2. Premier programme

```cpp
#include <Saillance.h>

FloatOut couleur;

void setup() {
  Serial.begin(115200);

  // Type de carte, nom de l'appareil, WiFi
  Saillance.beginWifi(Board::Esp32, "controlbouton", "NomDuWifi", "MotDePasse");

  // Où envoyer : zone / module / composante / propriété
  Topic topic = Saillance.topic(Zone::Jam, "lumiere", "bande1", "couleur");

  // Les valeurs envoyées, chacune avec sa clé ; 20 envois/s max ; en continu
  couleur = Saillance.floatOut(topic, {"r", "g", "b"}, 20, SendMode::Continuous);
}

void loop() {
  Saillance.update();        // OBLIGATOIRE à chaque tour de loop()

  couleur.set("r", 0.5);     // toujours entre 0.0 et 1.0
  couleur.set("g", 0.2);
  couleur.set("b", 1.0);
}
```

**Les trois règles à retenir**

1. Appelez `Saillance.update()` à **chaque** tour de `loop()`.
2. Évitez les longs `delay()` dans `loop()` : ils ralentissent les envois et la réception.
3. Toutes les valeurs numériques sont des nombres de **0.0 à 1.0**.

### En Ethernet (cartes Olimex PoE)

```cpp
Saillance.beginEthernet(Board::Esp32PoeIso, "controlbouton");   // ou Board::Esp32Poe
```

Le reste du programme est identique. `beginEthernet` est refusé avec `Board::Esp32` (pas d'Ethernet).
Les cartes PoE peuvent aussi utiliser `beginWifi`.

---

## 3. Ce qui s'affiche dans le moniteur série

La bibliothèque raconte ce qu'elle fait, en français. Exemple avec l'exemple 1 :

```
[Saillance] appareil : esp32_exemple1
[Saillance] topic erreurs : saillance/sys/esp32_exemple1/error
[Saillance] topic infos   : saillance/sys/esp32_exemple1/infos (toutes les 5 s)
[Saillance] topic ping    : saillance/sys/esp32_exemple1/ping (toutes les 60 s)
[Saillance] WiFi : connexion a "NomDuWifi"...
[Saillance] envoi    -> saillance/jam/lumiere/bande1/couleur  [r,g,b]  max 20 Hz, continu
[Saillance] WiFi connecte, IP 10.0.21.57
[Saillance] MQTT : connexion a ws://10.0.21.210:8083/mqtt en tant que "esp32_exemple1"...
[Saillance] MQTT connecte
```

Chaque topic d'envoi (`envoi ->`) ou de réception (`reception <-`) est affiché au moment où vous le créez.
Les erreurs sont signalées avec leur source, par exemple `[Saillance] ERREUR (begin) : nom d'appareil ...`.
`Saillance.setDebug(false)` (avant `begin`) coupe ces messages.

---

## 4. Le nom de l'appareil

Le username MQTT (envoyé à EMQX) est `<carte>_<nom>` :

| Carte | Préfixe |
|---|---|
| `Board::Esp32` | `esp32` |
| `Board::Esp32Poe` | `esp32poe` |
| `Board::Esp32PoeIso` | `esp32poeiso` |

Exemple : `Board::Esp32PoeIso` + `"controlbouton"` → **`esp32poeiso_controlbouton`**.

| Règle sur le nom choisi | Détail |
|---|---|
| Longueur | 24 caractères maximum (le préfixe s'ajoute en plus) |
| Caractères | lettres et chiffres seulement (pas d'espace, d'accent, `_` ni `-`) |
| Majuscules | converties en minuscules |

* Un nom invalide est refusé : `begin` retourne `false` et le moniteur série dit pourquoi.
* Deux appareils avec le même username se déconnectent l'un l'autre : choisissez des noms uniques.
* Le username sert aussi de nom d'appareil sur le réseau (avec `-` à la place de `_`).
* Si le broker exige un mot de passe : `Saillance.setMqttPassword("...")` avant `begin`.

---

## 5. Construire un topic

Le topic est `saillance / zone / module / composante / propriété`, par exemple
`saillance/jam/lumiere/bande1/couleur`. « saillance » est toujours ajouté pour vous.
Deux façons de le construire :

```cpp
// En une ligne
Topic t = Saillance.topic(Zone::Jam, "lumiere", "bande1", "couleur");

// Guidé : l'éditeur propose l'étape suivante après chaque point
Topic t = Saillance.zone(Zone::Jam).module("lumiere").component("bande1").property("couleur");
```

| Niveau | Valeur | Règles |
|---|---|---|
| zone | à choisir dans la liste : `Zone::Jam`, `Zone::Ecrans`, `Zone::Globale`, `Zone::Regie` | pas de texte libre |
| module | texte libre | 12 caractères max |
| composante | texte libre | 12 caractères max |
| propriété | texte libre | 12 caractères max |

Les textes libres : lettres, chiffres, `_` ou `-`, convertis en minuscules.
Un topic invalide est refusé avec un message qui dit quoi corriger, et les fonctions qui l'utilisent
retournent un objet non valide (`valid()` vaut `false`).

Pour ajouter une zone : voir [Configuration](#10-configuration-pour-les-mainteneurs).

---

## 6. Envoyer

### Des nombres

```cpp
FloatOut f = Saillance.floatOut(topic, {"r", "g", "b"}, 30, SendMode::Event);
f.set("r", 0.7);
f.trigger();            // forcer un envoi même sans changement
```

Les paramètres de `floatOut` : le topic, la liste des **clés**, la **fréquence maximale** (par défaut 30),
le **mode** (par défaut `Event`). Plusieurs valeurs peuvent partir dans un même message, une clé par valeur.

### Du texte

```cpp
TextOut t = Saillance.textOut(topic2);                // 30 Hz max, mode Event
t.set("bonjour");
```

### Une table

Une table est une **liste de rangées**, chaque rangée une liste de **textes** (même les nombres).
Il n'y a jamais de noms de colonnes. La case de la rangée `y`, colonne `x`, est `val[y][x]`.

```cpp
TableOut t = Saillance.tableOut(topic3, 5, SendMode::Event);   // 5 envois/s max (défaut : 10)

t.addRow({"a", "0.1", "on"});     // ajoute une rangée à la fin
t.addRow({"b", "0.5", "off"});
t.set(1, 2, "on");                // rangée 1, colonne 2 (la table s'agrandit au besoin)
t.set(1, 1, 0.25f);               // un nombre est converti en texte : "0.25"
t.clear();                        // vide la table
```

Maximum **64 rangées**, **16 colonnes** et **8 Ko** par message. Une case hors limites est refusée avec un
avertissement ; une table qui dépasse 8 Ko n'est pas envoyée (erreur signalée). Les rangées peuvent avoir des
longueurs différentes. Une table de 8 Ko à 30 Hz représente beaucoup de trafic : gardez une fréquence basse.

### Les règles

| Élément | Règle |
|---|---|
| Clés | 6 caractères max (lettres, chiffres, `_`), déclarées à la création, 16 par topic au plus |
| Valeurs | entre 0 et 1. Hors limites : ramenées dans l'intervalle, avec un avertissement (une fois). `NaN` : ignoré |
| Fréquence maximale | de 1 à 30 envois par seconde (hors limites : corrigée avec un avertissement) |
| Texte | 128 caractères max (coupé au-delà) |
| Taille d'un message | 512 octets max (nombres et texte), 8 Ko pour une table |
| Table | 64 rangées x 16 colonnes max, tout en texte |

### Les deux modes

* `SendMode::Event` : un message n'est envoyé que lorsqu'une valeur **change**, sans dépasser la fréquence
  maximale. Si la valeur change plusieurs fois entre deux envois, c'est la dernière qui part.
  Aucun message n'est envoyé tant qu'aucune valeur n'a changé : utilisez `trigger()` pour envoyer l'état de départ.
* `SendMode::Continuous` : un message à la fréquence demandée, même si rien ne change.

Hors connexion au broker, le mode `Continuous` saute les envois (comptés dans `df`, voir plus bas) ;
le mode `Event` garde la dernière valeur et l'envoie à la reconnexion.

### Format des messages envoyés

```json
{"v":1,"ts":1760000000123,"seq":42,"val":[0.5,0.2,1],"f":"float","label":["r","g","b"]}
{"v":1,"ts":1760000000123,"seq":7,"val":"bonjour","f":"string"}
{"v":1,"ts":1760000000123,"seq":4,"val":[["a","0.1","on"],["b","0.5","off"]],"f":"table"}
```

| Champ | Sens |
|---|---|
| `v` | version du format (1) |
| `ts` | horodatage en millisecondes (voir ci-dessous) |
| `seq` | compteur qui augmente à chaque message de ce topic (repart à 0 au redémarrage) |
| `val` | liste de nombres (0 à 1), texte, ou liste de rangées de textes (table) |
| `f` | `"float"`, `"string"` ou `"table"` |
| `label` | pour `float` seulement : la clé de chaque valeur, dans le même ordre que `val` (une table n'a pas de `label`) |

**À propos de `ts`** : la carte se synchronise par NTP (`pool.ntp.org` par défaut) dès qu'elle a le réseau.
Tant que ce n'est pas fait, `ts` contient le nombre de millisecondes depuis le démarrage.
Un réseau sans accès à Internet : `Saillance.setNtpServer("adresse")` (avant `begin`).

---

## 7. Recevoir

```cpp
Input niveau;

niveau = Saillance.input(topic);                     // lire quand on veut
float v = niveau.get("niv");                         // valeur de la clé "niv", 0.0 si rien reçu

Saillance.input(topic, [](const Message& m) {        // ou être appelé à chaque message
  Serial.println(m.get("niv"));
});
```

Ce que l'on peut faire avec un `Input` ou un `Message` :

| Appel | Résultat |
|---|---|
| `in.hasData()` | au moins un message reçu ? |
| `in.isNew()` | un message est arrivé depuis le dernier appel ? |
| `in.get("clé", défaut)` | valeur d'une clé |
| `in.value(i)` | valeur à la position `i` |
| `in.isText()` / `in.text()` | le message est du texte / son contenu |
| `in.message()` | le message entier |
| `m.count`, `m.key(i)`, `m.value(i)`, `m.has("clé")` | parcourir toutes les valeurs d'un `Message` |
| `m.ts`, `m.seq` | horodatage et compteur de l'expéditeur |
| `in.isTable()` / `m.isTable` | le message est une table ? |
| `in.rowCount()`, `in.colCount(y)` | nombre de rangées / de colonnes de la rangée `y` |
| `in.cell(y, x)`, `in.cellFloat(y, x, défaut)` | la case `val[y][x]` en texte (`""` si elle n'existe pas) / lue comme un nombre |

* Vos fonctions sont toujours appelées depuis `Saillance.update()`, donc dans `loop()`.
* Les valeurs reçues hors de 0 à 1 sont ramenées dans l'intervalle.
* Une table reçue est refusée si elle dépasse 64 rangées, 16 colonnes ou 8 Ko. Les cases numériques
  (`0.5` au lieu de `"0.5"`) sont acceptées et converties en texte.
* L'abonnement se fait au topic exact (pas de `#` ni `+`) et se refait tout seul après une reconnexion.
* Un appareil abonné à un topic sur lequel il envoie aussi reçoit ses propres messages.

---

## 8. Messages automatiques (rien à faire)

| Topic | Quand | Contenu |
|---|---|---|
| `saillance/sys/<username>/infos` | toutes les 5 s (et dès la connexion) | `v ts seq app fps fpsmin msmax df cpu ram gpu gpumem ac performModeL lat rc` |
| `saillance/sys/<username>/ping` | toutes les 60 s | `v ts seq` (sert à mesurer `lat`) |
| `saillance/sys/<username>/error` | à chaque erreur | `v ts seq src msg sv t` |

### `infos`

Même format que celui des autres appareils Saillance (par exemple le patch TouchDesigner). `-1` veut dire
« non disponible » ; le champ `app` est vide si on ne l'a pas renseigné.

```json
{"v":1,"ts":1792101066169,"seq":42,"app":"","fps":59.8,"fpsmin":41.2,"msmax":24.3,"df":3,"cpu":18,"ram":61,"gpu":-1,"gpumem":-1,"ac":1,"performModeL":1,"lat":4,"rc":0}
```

| Champ | Sens | Sur un ESP32 |
|---|---|---|
| `app` | nom du programme, version comprise | vide, ou ce que vous donnez à `Saillance.setAppName("controlbouton_v1.2")` |
| `fps` | images par seconde, moyenne des 5 s | tours de `loop()` par seconde |
| `fpsmin` | fps de l'image la plus lente des 5 s (1000 / `msmax`) | idem |
| `msmax` | durée en ms de l'image la plus longue des 5 s | durée du tour de `loop()` le plus long |
| `df` | images perdues pendant les 5 s | envois manqués (hors connexion, ou échec d'envoi) |
| `cpu` | % du processeur, moyenne depuis le dernier info | moyenne des deux cœurs |
| `ram` | % de mémoire vive utilisée | mémoire (tas) utilisée |
| `gpu`, `gpumem` | carte graphique | toujours `-1` (pas de GPU) |
| `ac` | 1 = branché sur le secteur | toujours `1` |
| `performModeL` | 1 = Perform Mode | toujours `1` |
| `lat` | aller-retour jusqu'au serveur en ms, mesuré par le ping précédent | voir `ping` ci-dessous ; `-1` tant qu'aucun ping n'est revenu |
| `rc` | reconnexions au broker depuis le démarrage | 0 à la première connexion |

L'image la plus lente (`msmax`) inclut tout ce que fait votre `loop()` : un `delay(100)` donnera `msmax` ≈ 100.

### `ping`

Toutes les 60 s (et dès la connexion), la carte s'envoie une enveloppe seule sur
`saillance/sys/<username>/ping` ; elle est abonnée à ce topic et reçoit donc le message en retour du broker :

```json
{"v":1,"ts":1792101066169,"seq":42}
```

La latence (`lat` de `infos`) est le temps entre l'envoi et le retour du message, en millisecondes. Elle est
mesurée avec l'horloge interne de la carte, donc sans dépendre du NTP. Si le ping ne revient pas avant le
suivant, `lat` repasse à `-1`. Le compte EMQX de l'appareil doit donc pouvoir publier **et** s'abonner à
`saillance/sys/<username>/#`.

### `error`

Erreurs capturées automatiquement :

* tout `ESP_LOGE` / `log_e` du système (WiFi, MQTT, mémoire…) ;
* perte du WiFi ou du câble Ethernet, refus du broker ;
* mémoire presque épuisée (moins de 20 000 octets libres) ;
* erreurs d'utilisation de la bibliothèque (nom, topic, clé, valeur invalides) ;
* la cause du redémarrage précédent : plantage, watchdog, baisse de tension. Un plantage ne peut pas être
  envoyé sur le coup, il est donc signalé au démarrage suivant.

Une erreur qui se répète à l'identique n'est envoyée qu'une fois toutes les 5 s. Les erreurs survenues
sans connexion au broker sont envoyées à la reconnexion (jusqu'à 10 en attente).

Vos propres erreurs :

```cpp
Saillance.reportError("capteur", "lecture impossible");                       // gravité Erreur
Saillance.reportError("capteur", "valeur douteuse", Severity::Warning);
```

| `sv` (gravité) | Sens |
|---|---|
| 0 | `Severity::Info` |
| 1 | `Severity::Warning` |
| 2 | `Severity::Error` |
| 3 | `Severity::Critical` |

| `t` (code) | Sens |
|---|---|
| 100 | erreur du système capturée dans les logs |
| 101 | redémarrage anormal |
| 102 | mémoire basse |
| 200 | lien WiFi / Ethernet perdu |
| 201 | démarrage du réseau impossible |
| 300 | connexion refusée par le broker |
| 301 | autre erreur MQTT |
| 400 – 405 | erreur d'utilisation : nom, topic, clé, valeur, configuration, message reçu illisible |
| 900 | erreur envoyée par votre programme |

---

## 9. En cas de problème

| Symptôme | Piste |
|---|---|
| `nom d'appareil ... invalide` | lettres et chiffres seulement, 24 max |
| `topic invalide` ou `module/composante/propriete ... invalide` | 12 caractères max, lettres/chiffres/`_`/`-` |
| `cle ... inconnue` | la clé n'a pas été déclarée dans la liste de `floatOut` |
| Rien après « connexion à… » | mauvais ssid / mot de passe WiFi |
| Ethernet : rien ne se passe | câble branché ? bonne carte (`Esp32Poe` / `Esp32PoeIso`) ? |
| `broker injoignable` | l'ESP32 n'est pas sur le même réseau que `10.0.21.210`, ou le port 8083 est bloqué |
| `connexion refusee par le broker` | username / mot de passe refusé par EMQX |
| Aucune donnée n'est envoyée | `Saillance.update()` oublié dans `loop()`, ou mode `Event` sans changement de valeur |
| Caractères illisibles dans le moniteur série | vitesse différente de 115200 |
| `table trop grosse` | la table dépasse 8 Ko : moins de rangées ou de texte |
| `lat` reste à `-1` | le ping ne revient pas : EMQX doit autoriser l'abonnement à `saillance/sys/<username>/ping` |
| `FloatOut non valide` | la création a échoué : lisez le message juste au-dessus |

Pour voir ce que reçoit le broker, un client comme [MQTTX](https://mqttx.app) connecté en WebSocket
à `ws://10.0.21.210:8083/mqtt` et abonné à `saillance/#` suffit.

---

## 10. Configuration (pour les mainteneurs)

Tout se règle dans `src/SaillanceConfig.h` :

| Réglage | Rôle |
|---|---|
| `SAILLANCE_BROKER_HOST`, `_PORT`, `_PATH` | adresse du broker (`/mqtt` est le chemin WebSocket par défaut d'EMQX) |
| `SAILLANCE_TOPIC_LEADING_SLASH` | `1` : `/saillance/...`, `0` : `saillance/...` (défaut) |
| `SAILLANCE_ZONES` | liste des zones |
| `SAILLANCE_NAME_MAX`, `_SEGMENT_MAX`, `_KEY_MAX`, `_MAX_HZ`… | les limites |
| `SAILLANCE_INFOS_PERIOD_MS`, `SAILLANCE_PING_PERIOD_MS` | périodes de `infos` et de `ping` |
| `SAILLANCE_TABLE_MAX_ROWS`, `_COLS`, `_BYTES` | limites d'une table |

**Ajouter une zone** : une ligne dans `SAILLANCE_ZONES` :

```cpp
#define SAILLANCE_ZONES(X)         \
  X(Jam, "jam")                    \
  X(Ecrans, "ecrans")              \
  X(Globale, "globale")            \
  X(Regie, "regie")                \
  X(Salle, "salle")                 // ← ajoutée, utilisable avec Zone::Salle
```

Le texte doit respecter les règles des textes libres (12 caractères max, lettres/chiffres/`_`/`-`).
Attention : toutes les lignes sauf la dernière finissent par `\`.

**Tests** (logique pure, sans carte) : `pio test -e native`

**Fichiers**

| Fichier | Contenu |
|---|---|
| `src/Saillance.h` | l'interface publique |
| `src/SaillanceCore.*` | validation, topics, JSON, cadence d'envoi (testé sur ordinateur) |
| `src/SaillanceNet.cpp` | WiFi / Ethernet |
| `src/SaillanceMqtt.cpp` | client MQTT (esp-mqtt) |
| `src/SaillanceSystem.cpp` | messages `error` et `infos`, capture des erreurs, mesure du CPU |
