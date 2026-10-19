# API de CeXii MQTT

Tout passe par l'objet global `CexiiMqtt` (`#include <CexiiMqtt.h>`). Les types sont dans l'espace de noms
`cexii`, déjà activé par un `using namespace cexii;` dans l'en-tête.

## Démarrage

| Appel | Rôle |
|---|---|
| `bool beginEthernet(Board, const char* nom)` | cartes PoE : refusé avec `Board::Esp32` |
| `bool beginWifi(Board, const char* nom, const char* ssid, const char* mdp)` | toutes les cartes |
| `void update()` | **à chaque tour de `loop()`** : envois, réception, rappels, messages système |
| `bool connected()` | connecté au broker ? |
| `const char* username()` | `<carte>_<nom>` |

Réglages optionnels, **avant** `begin…` : `setMqttPassword(const char*)`, `setNtpServer(const char*)`
(défaut `pool.ntp.org`, pour le champ `ts`), `setAppName(const char*)` (champ `app` de `infos`,
48 caractères), `setDebug(bool)` (messages `[CeXii MQTT]` sur Serial, défaut oui).

`begin…` retourne `false` si le nom est invalide ou si le réseau ne démarre pas ; la raison est sur Serial.
Les cartes : `Board::Esp32` (préfixe `esp32`), `Board::Esp32Poe` (`esp32poe`), `Board::Esp32PoeIso` (`esp32poeiso`).

## Topics

```cpp
Topic t = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande1", "couleur");
Topic t = CexiiMqtt.zone(Zone::Globale).module("lumiere").component("bande1").property("couleur");
```
`t.valid()` / `if (t)` ; `t.c_str()` donne le texte (`saillance/globale/lumiere/bande1/couleur`).
Un segment invalide produit un topic non valide et un message sur Serial ; les fonctions qui
reçoivent ce topic retournent un objet non valide (`valid()` faux) et les appels dessus sont ignorés.

## Envoi

| Appel | Rôle |
|---|---|
| `FloatOut floatOut(topic, {"r","g","b"}, maxHz = 30, mode = SendMode::Event)` | nombres, une clé par valeur |
| `FloatOut::set(const char* clé, float valeur)` | valeur 0 à 1 ; clé inconnue = avertissement une fois |
| `TextOut textOut(topic, maxHz = 30, mode = SendMode::Event)` | texte |
| `TextOut::set(const char*)` (ou `String`) | 128 caractères max |
| `TableOut tableOut(topic, maxHz = 10, mode = SendMode::Event)` | table |
| `TableOut::addRow({"a","0.1","on"})` | ajoute une rangée |
| `TableOut::set(rangée, colonne, texte / float / int)` | agrandit la table au besoin ; indices dès 0 |
| `TableOut::clear()` | vide la table |
| `trigger()` (les trois) | force un envoi même sans changement |
| `valid()` (les trois) | la création a-t-elle réussi ? |

`maxHz` hors de 1 à 30 est corrigé avec un avertissement. Si le broker est injoignable, le mode
`Continuous` saute les envois (comptés dans `df`), le mode `Event` garde la dernière valeur et l'envoie
à la reconnexion.

## Réception

```cpp
Input in = CexiiMqtt.input(topic);                          // lecture à la demande
CexiiMqtt.input(topic, [](const Message& m) { ... });       // ou rappel à chaque message
in.onMessage(callback);                                      // ajouter un rappel à un Input existant
```

| Sur `Input` | Rôle |
|---|---|
| `hasData()` | au moins un message reçu |
| `isNew()` | vrai **une seule fois** par nouveau message (consomme le drapeau) |
| `get("clé", défaut)`, `value(i)` | nombres |
| `isText()`, `text()` | texte |
| `isTable()`, `rowCount()`, `colCount(y)`, `cell(y, x)`, `cellFloat(y, x, défaut)` | table (`cell` renvoie `""` si la case n'existe pas) |
| `message()` | le dernier `Message` en entier |

`Message` (argument des rappels) : `isText`, `text`, `count`, `key(i)`, `value(i)`, `get(clé, défaut)`,
`has(clé)`, `isTable`, `rowCount()`, `colCount(y)`, `cell(y, x)`, `cellFloat(y, x, défaut)`, `ts`, `seq`, `version`.

Comportements : abonnement au topic exact (pas de `#` ni `+`), refait après chaque reconnexion ; un appareil
abonné à un topic sur lequel il publie reçoit ses propres messages ; les nombres reçus hors de 0 à 1 sont
ramenés dans l'intervalle ; une table reçue au-delà de 64 × 16 ou 8 Ko est refusée ; un message illisible
produit « message illisible sur … » sur Serial.

## Erreurs

`CexiiMqtt.reportError("source", "message", Severity::Warning)` envoie sur `saillance/sys/<username>/error`
(code `t` 900). Gravités : `Info` 0, `Warning` 1, `Error` 2, `Critical` 3. Les erreurs du système
(`ESP_LOGE`, `log_e` si `CORE_DEBUG_LEVEL=1`), la perte de lien, la mémoire basse et la cause d'un
redémarrage anormal sont envoyées toutes seules.

## Ce que la bibliothèque ne fait pas

TLS (`wss://`), MQTT 5, abonnements génériques, qualité de service 1 ou 2 pour les données (tout est en
QoS 0, sauf `error` en QoS 1), ESP32-S2/S3/C3 (non testés), Arduino-ESP32 3.x (branches de code prévues,
jamais compilées).
