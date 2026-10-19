# CeXii MQTT — référence

Cette page décrit **tout** en détail. Les topics commencent par `saillance/`. Pour commencer, lisez plutôt le [README](../README.md).

**Sur cette page** : [Aide-mémoire](#aide-mémoire-des-fonctions) · [Nom de l'appareil](#le-nom-de-lappareil) ·
[Topics](#construire-un-topic) · [Envoyer](#envoyer) · [Recevoir](#recevoir) ·
[Messages automatiques](#messages-automatiques) · [Configuration](#configuration-pour-les-mainteneurs)

---

## Aide-mémoire des fonctions

| Fonction | Rôle |
|---|---|
| `CexiiMqtt.beginWifi(carte, nom, ssid, motDePasse)` | démarrer en WiFi |
| `CexiiMqtt.beginEthernet(carte, nom)` | démarrer avec un câble (cartes PoE) |
| `CexiiMqtt.update()` | à appeler à chaque tour de `loop()` |
| `CexiiMqtt.topic(zone, module, composante, propriété)` | construire un topic |
| `CexiiMqtt.zone(z).module(m).component(c).property(p)` | idem, étape par étape |
| `CexiiMqtt.floatOut(topic, {clés}, hz, mode)` | envoyer des nombres → `FloatOut` |
| `CexiiMqtt.textOut(topic, hz, mode)` | envoyer du texte → `TextOut` |
| `CexiiMqtt.tableOut(topic, hz, mode)` | envoyer une table → `TableOut` |
| `CexiiMqtt.input(topic)` / `input(topic, fonction)` | recevoir → `Input` |
| `CexiiMqtt.connected()` / `username()` | état de la connexion / nom sur le serveur |
| `CexiiMqtt.reportError(source, message, gravité)` | envoyer sa propre erreur |
| `CexiiMqtt.setMqttPassword(...)`, `setNtpServer(...)`, `setAppName(...)`, `setDebug(...)` | réglages, **avant** `begin` |

Types de carte : `Board::Esp32`, `Board::Esp32Poe`, `Board::Esp32PoeIso`.
Modes : `SendMode::Event`, `SendMode::Continuous`.
Gravités : `Severity::Info`, `Warning`, `Error`, `Critical`.
Zones : `Zone::Jam`, `Zone::Ecrans`, `Zone::Globale`, `Zone::Regie`.

---

## Le nom de l'appareil

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
* Si le broker exige un mot de passe : `CexiiMqtt.setMqttPassword("...")` avant `begin`.

---

## Construire un topic

Le topic est `saillance / zone / module / composante / propriété`, par exemple
`saillance/jam/lumiere/bande1/couleur`. « saillance » est toujours ajouté pour vous.
Deux façons de le construire :

```cpp
// En une ligne
Topic t = CexiiMqtt.topic(Zone::Jam, "lumiere", "bande1", "couleur");

// Guidé : l'éditeur propose l'étape suivante après chaque point
Topic t = CexiiMqtt.zone(Zone::Jam).module("lumiere").component("bande1").property("couleur");
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

Pour ajouter une zone : voir [Configuration](#configuration-pour-les-mainteneurs).

---

## Envoyer

### Des nombres

```cpp
FloatOut f = CexiiMqtt.floatOut(topic, {"r", "g", "b"}, 30, SendMode::Event);
f.set("r", 0.7);
f.trigger();            // forcer un envoi même sans changement
```

Les paramètres de `floatOut` : le topic, la liste des **clés**, la **fréquence maximale** (par défaut 30),
le **mode** (par défaut `Event`). Plusieurs valeurs peuvent partir dans un même message, une clé par valeur.

### Du texte

```cpp
TextOut t = CexiiMqtt.textOut(topic2);                // 30 Hz max, mode Event
t.set("bonjour");
```

### Une table

Une table est une **liste de rangées**, chaque rangée une liste de **textes** (même les nombres).
Il n'y a jamais de noms de colonnes. La case de la rangée `y`, colonne `x`, est `val[y][x]`.

```cpp
TableOut t = CexiiMqtt.tableOut(topic3, 5, SendMode::Event);   // 5 envois/s max (défaut : 10)

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
Un réseau sans accès à Internet : `CexiiMqtt.setNtpServer("adresse")` (avant `begin`).

---

## Recevoir

```cpp
Input niveau;

niveau = CexiiMqtt.input(topic);                     // lire quand on veut
float v = niveau.get("niv");                         // valeur de la clé "niv", 0.0 si rien reçu

CexiiMqtt.input(topic, [](const Message& m) {        // ou être appelé à chaque message
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

* Vos fonctions sont toujours appelées depuis `CexiiMqtt.update()`, donc dans `loop()`.
* Les valeurs reçues hors de 0 à 1 sont ramenées dans l'intervalle.
* Une table reçue est refusée si elle dépasse 64 rangées, 16 colonnes ou 8 Ko. Les cases numériques
  (`0.5` au lieu de `"0.5"`) sont acceptées et converties en texte.
* L'abonnement se fait au topic exact (pas de `#` ni `+`) et se refait tout seul après une reconnexion.
* Un appareil abonné à un topic sur lequel il envoie aussi reçoit ses propres messages.

---

## Messages automatiques

| Topic | Quand | Contenu |
|---|---|---|
| `saillance/sys/<username>/infos` | toutes les 5 s (et dès la connexion) | `v ts seq app fps fpsmin msmax df cpu ram gpu gpumem ac performModeL lat rc` |
| `saillance/sys/<username>/ping` | toutes les 60 s | `v ts seq` (sert à mesurer `lat`) |
| `saillance/sys/<username>/error` | à chaque erreur | `v ts seq src msg sv t` |

### `infos`

Même format que celui des autres appareils CeXii (par exemple le patch TouchDesigner). `-1` veut dire
« non disponible » ; le champ `app` est vide si on ne l'a pas renseigné.

```json
{"v":1,"ts":1792101066169,"seq":42,"app":"","fps":59.8,"fpsmin":41.2,"msmax":24.3,"df":3,"cpu":18,"ram":61,"gpu":-1,"gpumem":-1,"ac":1,"performModeL":1,"lat":4,"rc":0}
```

| Champ | Sens | Sur un ESP32 |
|---|---|---|
| `app` | nom du programme, version comprise | vide, ou ce que vous donnez à `CexiiMqtt.setAppName("controlbouton_v1.2")` |
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
CexiiMqtt.reportError("capteur", "lecture impossible");                       // gravité Erreur
CexiiMqtt.reportError("capteur", "valeur douteuse", Severity::Warning);
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

## Configuration (pour les mainteneurs)

Tout se règle dans `src/CexiiMqttConfig.h` :

| Réglage | Rôle |
|---|---|
| `CEXII_BROKER_HOST`, `_PORT`, `_PATH` | adresse du broker (`/mqtt` est le chemin WebSocket par défaut d'EMQX) |
| `CEXII_TOPIC_LEADING_SLASH` | `1` : `/saillance/...`, `0` : `saillance/...` (défaut) |
| `CEXII_ZONES` | liste des zones |
| `CEXII_NAME_MAX`, `_SEGMENT_MAX`, `_KEY_MAX`, `_MAX_HZ`… | les limites |
| `CEXII_INFOS_PERIOD_MS`, `CEXII_PING_PERIOD_MS` | périodes de `infos` et de `ping` |
| `CEXII_TABLE_MAX_ROWS`, `_COLS`, `_BYTES` | limites d'une table |

**Ajouter une zone** : une ligne dans `CEXII_ZONES` :

```cpp
#define CEXII_ZONES(X)         \
  X(Jam, "jam")                    \
  X(Ecrans, "ecrans")              \
  X(Globale, "globale")            \
  X(Regie, "regie")                \
  X(Salle, "salle")                 // la nouvelle zone, utilisable avec Zone::Salle
```

Le texte doit respecter les règles des textes libres (12 caractères max, lettres/chiffres/`_`/`-`).
Attention : toutes les lignes sauf la dernière finissent par `\`.

**Tests** (logique pure, sans carte) : `pio test -e native`

**Fichiers**

| Fichier | Contenu |
|---|---|
| `src/CexiiMqtt.h` | l'interface publique |
| `src/CexiiMqttCore.*` | validation, topics, JSON, cadence d'envoi (testé sur ordinateur) |
| `src/CexiiMqttNet.cpp` | WiFi / Ethernet |
| `src/CexiiMqttClient.cpp` | client MQTT (esp-mqtt) |
| `src/CexiiMqttSystem.cpp` | messages `error` et `infos`, capture des erreurs, mesure du CPU |
