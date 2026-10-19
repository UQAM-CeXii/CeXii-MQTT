# Comment fonctionne CeXii MQTT (sous le capot)

Cette page est pour les personnes **curieuses** : elle explique ce qui se passe réellement dans la bibliothèque,
comment elle est construite, et pourquoi elle a été programmée ainsi. Vous n'avez pas besoin de la lire pour
l'utiliser (le [README](../README.md) suffit), et la liste de toutes les fonctions est dans la
[référence](REFERENCE.md).

**Sur cette page** : [Vue d'ensemble](#1-vue-densemble) · [La construction en couches](#2-la-construction-en-couches) ·
[La vie d'un programme](#3-la-vie-dun-programme) · [Envoyer](#4-ce-qui-se-passe-quand-vous-envoyez) ·
[Recevoir](#5-ce-qui-se-passe-quand-vous-recevez) · [Les messages automatiques](#6-les-messages-automatiques) ·
[Qui fait quoi en même temps](#7-qui-fait-quoi-en-même-temps) · [Les choix de conception](#8-les-choix-de-conception-et-pourquoi) ·
[Les pièges rencontrés](#9-les-pièges-rencontrés) · [Comment c'est testé](#10-comment-cest-testé) ·
[Les limites](#11-les-limites-connues) · [Lire le code](#12-par-où-commencer-pour-lire-le-code) · [Glossaire](#13-glossaire)

---

## 1. Vue d'ensemble

Votre carte ne parle pas directement aux autres appareils. Elle parle à un **serveur** (le *broker*, ici EMQX à
l'adresse `10.0.21.210`), qui redistribue les messages à ceux que ça intéresse. Ce système s'appelle **MQTT** :

```
  Carte A ──envoie "saillance/jam/lumiere/…"──►  ┌────────────┐  ──►  TouchDesigner (abonné à ce topic)
                                                  │   Broker   │
  Carte B ◄──reçoit ce à quoi elle est abonnée──  │   (EMQX)   │  ◄──  autres programmes
                                                  └────────────┘
```

- Un message est rangé dans un **topic**, une sorte d'adresse : `saillance/jam/lumiere/bande1/couleur`.
- Celui qui veut recevoir s'**abonne** à un topic. Le broker lui transmet tout ce qui y est publié.
- La carte et le broker se parlent en **MQTT sur WebSocket** (`ws://10.0.21.210:8083/mqtt`) : c'est du MQTT
  « emballé » dans une connexion web, ce que EMQX propose sur le port 8083.

La bibliothèque cache tout ça derrière quelques objets (`FloatOut`, `TextOut`, `TableOut`, `Input`) et une
fonction à appeler dans `loop()` : `CexiiMqtt.update()`.

Elle ne réinvente pas MQTT. Elle s'appuie sur deux composants existants :

| Composant | Rôle | D'où vient-il |
|---|---|---|
| **esp-mqtt** | le client MQTT (connexion, WebSocket, envoi, abonnements, reconnexion) | inclus dans le framework ESP32 d'Espressif, rien à installer |
| **ArduinoJson** | **lire** les messages JSON reçus | bibliothèque à installer (version 7) |

Tout le reste a été écrit pour ce projet : environ 2 000 lignes de C++ dans `src/`, plus 376 lignes de tests.

---

## 2. La construction en couches

Le code est séparé en deux grandes parties, et cette séparation est volontaire.

```
  ┌────────────────────────────────────────────────────────────────────────┐
  │  Votre programme (.ino)       CexiiMqtt.floatOut(...)   couleur.set(...) │
  └───────────────┬────────────────────────────────────────────────────────┘
                  │  CexiiMqtt.h : l'interface qu'on vous montre
  ┌───────────────▼────────────────────────────────────────────────────────┐
  │  Couche ESP32 (utilise Arduino, WiFi, FreeRTOS, esp-mqtt)               │
  │    CexiiMqtt.cpp        l'objet principal, update(), création des canaux │
  │    CexiiMqttNet.cpp     WiFi et Ethernet                                 │
  │    CexiiMqttClient.cpp  le client MQTT (esp-mqtt), réception             │
  │    CexiiMqttSystem.cpp  erreurs, infos, ping, mesure du processeur       │
  └───────────────┬────────────────────────────────────────────────────────┘
                  │  appelle
  ┌───────────────▼────────────────────────────────────────────────────────┐
  │  Cœur (C++ « pur », ne dépend ni d'Arduino ni de l'ESP32)                │
  │    CexiiMqttCore.cpp/.h  validation, topics, écriture et lecture du JSON,│
  │                          cadence d'envoi, canaux d'envoi et de réception │
  │    CexiiMqttConfig.h     tous les réglages (serveur, zones, limites)     │
  │    CexiiMqttTypes.h      les types : Board, Zone, SendMode, Severity…    │
  └────────────────────────────────────────────────────────────────────────┘
```

**Pourquoi un cœur « pur » ?** Tout ce qui peut se tromper sans matériel (les règles de nom, la construction des
topics, le JSON, le calcul des cadences) est dans le cœur. Comme il ne dépend pas de la carte, on peut le
tester **sur l'ordinateur**, en une seconde, avec 26 tests automatiques. La couche ESP32, elle, ne peut être
vérifiée qu'en compilant et en la faisant tourner sur une vraie carte.

| Fichier | Rôle |
|---|---|
| `CexiiMqttConfig.h` | adresse du serveur, liste des zones, toutes les limites (24, 12, 6, 30 Hz, 64×16, 8 Ko…) |
| `CexiiMqttTypes.h` | les types énumérés : `Board`, `Zone`, `SendMode`, `Severity`, `ErrorCode` |
| `CexiiMqttCore.h/.cpp` | validation, `Topic`, `JsonWriter`, `Message`, `FloatChannel`, `TextChannel`, `TableChannel`, `InputChannel` |
| `CexiiMqtt.h/.cpp` | l'interface publique et l'objet global `CexiiMqtt` |
| `CexiiMqttNet.cpp` | démarrage du WiFi ou d'Ethernet, événements réseau |
| `CexiiMqttClient.cpp` | création du client MQTT, abonnements, réception, réassemblage |
| `CexiiMqttSystem.cpp` | messages `error`, `infos`, `ping`, capture des erreurs, mesure du processeur |

---

## 3. La vie d'un programme

### Au démarrage : `beginEthernet(...)` ou `beginWifi(...)`

1. **Valider le nom** de l'appareil (24 caractères, lettres et chiffres). S'il est invalide, la bibliothèque
   explique pourquoi sur le moniteur série et s'arrête là.
2. **Fabriquer le username** : le type de carte, un `_`, le nom en minuscules (`esp32poeiso_controlbouton`).
   C'est aussi l'identifiant de la connexion MQTT, ce qui explique que deux cartes de même nom se
   déconnectent l'une l'autre.
3. **Construire les trois topics système** : `saillance/sys/<username>/error`, `…/infos`, `…/ping`.
4. **Créer deux files d'attente** FreeRTOS (une pour les messages reçus, une pour les erreurs).
5. **Démarrer le système de surveillance** : on se branche sur les logs de l'ESP32 pour attraper les erreurs,
   on calibre la mesure du processeur (100 ms de repos), et on note si le dernier redémarrage était anormal.
6. **Démarrer le réseau** : WiFi (`WiFi.begin`) ou Ethernet (`ETH.begin` avec les broches de la carte Olimex).
   Cette étape ne attend pas : elle rend la main tout de suite.

Rien n'est encore connecté au serveur à ce stade.

### Le réseau arrive : les événements

Le réseau s'établit « en arrière-plan ». Quand la carte obtient une adresse IP, le système prévient la
bibliothèque par un **événement**. Elle fait alors deux choses : elle note « réseau prêt » et elle lance la
synchronisation de l'heure par NTP (pour les horodatages `ts`). Si le réseau tombe, l'événement inverse
repasse l'état à « réseau perdu » et une erreur est mise en file.

### Dans `loop()` : `update()`

`update()` est le **chef d'orchestre**. À chaque appel, dans cet ordre :

| # | Étape | Ce qu'elle fait |
|---|---|---|
| 1 | Mesure | note la durée écoulée depuis l'appel précédent (pour `fps` et `msmax`) |
| 2 | Synchroniser MQTT | si le réseau est prêt et le client MQTT pas démarré → le démarrer ; si le réseau est perdu → l'arrêter |
| 3 | S'abonner | si on est connecté : abonner tous les `Input` et le topic `ping` qui ne le sont pas encore |
| 4 | Distribuer | prendre les messages arrivés (file de réception), les décoder, les remettre aux bons `Input` et appeler vos fonctions |
| 5 | Envoyer | pour chaque `FloatOut`/`TextOut`/`TableOut` : est-il temps d'envoyer ? si oui, fabriquer le JSON et le publier |
| 6 | Erreurs | publier jusqu'à 3 erreurs en attente |
| 7 | Ping | envoyer le ping (toutes les 60 s) et enregistrer la latence quand il est revenu |
| 8 | Infos | toutes les 5 s, fabriquer et publier le message `infos` |

C'est pour ça que **tout dépend de `update()`** : si elle n'est pas appelée, rien ne part et rien n'est
distribué. C'est aussi pour ça qu'un long `delay()` dans `loop()` ralentit tout.

---

## 4. Ce qui se passe quand vous envoyez

Prenons `couleur.set("r", 0.5)` avec un `FloatOut` créé en mode `Event`.

```
  couleur.set("r", 0.5)            dans votre loop()
        │
        ▼
  FloatChannel::set                 cherche la clé "r", refuse NaN, ramène la valeur dans 0..1,
        │                           et SI la valeur est différente de l'ancienne → dirty = true
        ▼
  … plus tard, dans update() …
  due(maintenant) ?                 Event : dirty ET assez de temps depuis le dernier envoi ?
        │ oui                       Continuous : assez de temps depuis le dernier envoi ?
        ▼
  FloatChannel::build               fabrique à la main : {"v":1,"ts":…,"seq":…,"val":[…],"f":"float","label":[…]}
        │
        ▼
  esp_mqtt_client_publish           envoi immédiat sur le réseau, topic + JSON, QoS 0
```

### La cadence : comment on respecte « 30 fois par seconde au maximum »

Chaque canal calcule un intervalle : `intervalMs = ceil(1000 / maxHz)`, **arrondi vers le haut**. À 30 Hz,
c'est 34 ms, soit 29,4 envois par seconde : on ne dépasse jamais ce qu'on a promis (arrondir vers le bas
aurait donné 33 ms et 30,3 Hz).

- En mode **`Event`**, l'heure du dernier envoi est simplement « maintenant ». Si vous changez la valeur dix fois
  entre deux envois, un seul message part avec la **dernière** valeur : c'est voulu (les valeurs
  intermédiaires seraient périmées de toute façon).
- En mode **`Continuous`**, l'envoi suivant est calculé à partir de l'envoi *prévu* précédent
  (`dernier += intervalle`) et non de l'envoi *réel*. Sans cette précaution, un tour de `loop()` un peu
  long retarderait tous les envois suivants et la cadence dériverait. Si on a pris plus de deux intervalles de
  retard, on repart de zéro plutôt que d'envoyer en rafale pour « rattraper ».

### Hors connexion

- `Continuous` : l'occasion d'envoi est sautée et comptée (c'est le champ `df` de `infos`).
- `Event` : la valeur reste marquée « à envoyer » et part dès la reconnexion.

### Le JSON est écrit à la main

Le message est fabriqué par une petite classe, `JsonWriter`, qui écrit des caractères dans un tampon fixe : pas
d'allocation de mémoire, un format exactement maîtrisé (ordre des champs, `-1` plutôt que `-1.0`, une décimale
pour `fps`), et un message trop grand est détecté (la fonction retourne 0 au lieu de couper le JSON en deux).
Les nombres sont écrits avec 4 chiffres significatifs (`0.5`, `0.1234`).

Le numéro `seq` est un compteur **par canal** qui n'avance que si le message a bien été fabriqué.

---

## 5. Ce qui se passe quand vous recevez

La réception est plus délicate, parce que le client MQTT fonctionne dans **sa propre tâche**, en parallèle de
votre `loop()`.

```
  Broker ──► [tâche MQTT d'esp-mqtt] ──événement "données reçues"──►  notre gestionnaire
                                                                           │ copie le message
                                                                           ▼
                                                              file d'attente (8 places)
                                                                           │
  votre loop() ──► CexiiMqtt.update() ──► dispatchIncoming_ ◄────────────────┘
                                              │ décode le JSON (ArduinoJson)
                                              ▼
                                   pour chaque Input dont le topic est identique :
                                   garde le message + appelle votre fonction
```

**Pourquoi passer par une file ?** Si on appelait votre fonction directement depuis la tâche MQTT, elle
s'exécuterait *en même temps* que votre `loop()`, et les deux pourraient modifier la même variable au même
instant (un bogue très difficile à trouver). Avec la file, **vos fonctions s'exécutent toujours dans
`loop()`**, via `update()` : vous n'avez jamais à vous soucier de ça.

Détails :

- La comparaison des topics est **exacte** : lettre pour lettre, sans `#` ni `+`.
- Un `Input` garde le dernier message, ce qui permet `get("niv")` à tout moment, et un drapeau « nouveau »
  que `isNew()` remet à zéro.
- Les nombres reçus hors de 0 à 1 sont ramenés dans l'intervalle.
- Les clés reçues dans `label` sont limitées à 6 caractères.

### Les gros messages (tables)

Le client MQTT reçoit les données par morceaux d'au plus 1 024 octets. Un petit message (jusqu'à 512 octets)
est copié tel quel dans la file. Un message plus gros — typiquement une table, jusqu'à 8 Ko — est **réassemblé**
dans la mémoire : au premier morceau on réserve la place totale annoncée, on y recopie chaque morceau à sa
position, et quand le dernier arrive le message complet part dans la file. Un message annoncé au-delà de 8 Ko
est refusé sans rien réserver.

### Décoder une table

Le décodeur vérifie que `val` est une liste de listes, qu'il y a au plus 64 rangées et 16 colonnes, et que
chaque case est un texte. Par tolérance, un nombre est accepté et converti en texte. Au moindre doute, le message
est rejeté en bloc (« message illisible ») plutôt que d'en garder la moitié.

---

## 6. Les messages automatiques

Ils sont envoyés **par la bibliothèque**, avec les mêmes mécanismes que vos données.

### `error` : attraper les erreurs

Deux sources alimentent la file d'erreurs :

1. **Les erreurs que la bibliothèque détecte elle-même** (nom invalide, clé inconnue, lien perdu…) et celles que
   vous signalez avec `reportError`.
2. **Toutes les erreurs du système.** L'ESP32 écrit ses messages d'erreur (`ESP_LOGE`, `log_e`) à travers une fonction
   d'affichage unique. La bibliothèque **s'y glisse** (`esp_log_set_vprintf`) : chaque ligne passe d'abord chez
   elle, puis est renvoyée à l'affichage normal, donc le moniteur série ne change pas. Si la ligne commence
   par `E (` (erreur), elle est découpée en *source* (l'étiquette entre parenthèses) et *message*, puis mise en
   file.

L'erreur n'est **pas** envoyée depuis ce point d'accroche : il peut être appelé depuis n'importe quelle tâche, et
envoyer un message réseau à cet endroit pourrait créer une boucle (l'envoi produit lui-même un message de
log). Elle est seulement mise en file, et `update()` la publie plus tard.

Deux protections évitent de saturer le serveur : une même erreur (même source et même message) n'est envoyée
qu'**une fois toutes les 5 secondes**, et pas plus de 3 erreurs partent par appel d'`update()`. Les erreurs
survenues sans connexion attendent la reconnexion (10 au maximum). Elles sont envoyées en QoS 1, alors que les
données passent en QoS 0.

Un plantage (le processeur se bloque, redémarre) ne peut pas être signalé sur le coup : l'ESP32 est déjà en train de
redémarrer. La bibliothèque lit donc **la cause du dernier redémarrage** (`esp_reset_reason`) au démarrage suivant
et envoie l'erreur à ce moment.

### `infos` : l'état de santé, toutes les 5 secondes

| Champ | Comment il est calculé |
|---|---|
| `fps` | nombre d'appels d'`update()` depuis le dernier `infos`, divisé par le temps écoulé |
| `msmax` / `fpsmin` | le plus grand écart de temps entre deux appels d'`update()` (mesuré en microsecondes), et 1000 divisé par cet écart |
| `df` | envois manqués depuis le dernier `infos` |
| `cpu` | voir ci-dessous |
| `ram` | `(taille du tas − mémoire libre) / taille du tas` |
| `lat` | résultat du dernier ping |
| `rc` | nombre de connexions au serveur moins une |
| `gpu`, `gpumem` | `-1` : l'ESP32 n'a pas de carte graphique |
| `ac`, `performModeL` | toujours `1` |

**Mesurer le processeur sans outil dédié.** FreeRTOS lance, sur chaque cœur, une tâche « de repos » (*idle*) qui ne
tourne que lorsqu'il n'y a rien d'autre à faire. La bibliothèque lui accroche un petit compteur : plus le compteur
monte vite, plus le cœur est libre. Au démarrage, on mesure la vitesse du compteur pendant 100 ms de calme : c'est
la référence « 0 % occupé ». Ensuite, `cpu = 100 × (1 − vitesse_du_compteur / vitesse_de_référence)`, en moyenne sur
les deux cœurs. Si on observe une vitesse supérieure à la référence, la référence est mise à jour. C'est une
estimation, pas une mesure de laboratoire.

### `ping` : mesurer la latence

Toutes les 60 secondes, la carte publie `{"v":1,"ts":…,"seq":N}` sur son topic `ping`… **auquel elle est elle-même
abonnée.** Le serveur le lui renvoie donc, et la carte mesure le temps de l'aller-retour.

- Le temps est pris avec l'horloge interne de la carte (`micros()`), pas avec `ts` : ainsi la mesure ne dépend pas
  de la synchronisation NTP.
- L'heure d'arrivée est relevée **dans le gestionnaire MQTT, dès l'arrivée**, et non plus tard dans `update()` : sinon
  on mesurerait aussi le temps que votre `loop()` met à tourner.
- Le numéro `seq` de la réponse doit correspondre à celui qu'on a envoyé. Si le ping précédent n'est jamais
  revenu quand on envoie le suivant, `lat` repasse à `-1`.

### L'heure (`ts`)

Dès que la carte a une adresse IP, elle demande l'heure à un serveur NTP (`pool.ntp.org` par défaut). Tant qu'elle
ne l'a pas, `ts` contient le nombre de millisecondes depuis le démarrage ; ensuite, l'heure Unix en
millisecondes. C'est pourquoi un `ts` très petit signifie « horloge pas encore synchronisée ».

---

## 7. Qui fait quoi en même temps

Sur l'ESP32, plusieurs « tâches » tournent en parallèle. C'est la partie la plus subtile de la bibliothèque.

| Qui | Ce qu'il fait | Exécute du code utilisateur ? |
|---|---|---|
| **Tâche `loop()`** (Arduino) | votre programme + `update()` | **oui, c'est le seul endroit** |
| **Tâche MQTT** (esp-mqtt) | maintient la connexion, reçoit les messages, appelle notre gestionnaire d'événements | non |
| **Tâche d'événements réseau** | prévient quand le WiFi ou Ethernet change | non |
| **N'importe quelle tâche** | peut écrire un log d'erreur → notre point d'accroche | non |

Les règles qui rendent ça sûr :

1. **Les données ne se partagent que par des files d'attente FreeRTOS** (messages reçus, erreurs). Une file est
   faite pour ça : on y dépose d'un côté, on y prend de l'autre, sans collision.
2. **Les simples indicateurs** partagés (`réseau prêt`, `connecté`, `ping revenu`) sont écrits par une tâche et
   lus par une autre ; ils sont déclarés `volatile` pour que le compilateur relise toujours la vraie valeur.
3. **On ne bloque jamais** dans les gestionnaires : dépôt dans une file avec une attente de 0 (si elle est pleine, le
   message est abandonné plutôt que d'attendre).
4. **Un seul tampon d'écriture par rôle.** Toute la fabrication de messages se fait dans `loop()`, donc un seul
   tampon partagé suffit pour les envois ordinaires (512 octets) et un second pour les tables (8 Ko, réservé la
   première fois qu'on crée un `TableOut`).

---

## 8. Les choix de conception (et pourquoi)

| Choix | La raison |
|---|---|
| **Les clés sont déclarées à la création** (`{"r","g","b"}`) | le format d'un message est alors toujours le même, les tableaux sont de taille fixe (pas de mémoire qui grossit), et une faute de frappe (`"rr"`) est repérée tout de suite |
| **Les valeurs sont ramenées dans 0..1 plutôt que refusées** | un capteur qui dépasse de 0,001 ne doit pas faire disparaître la donnée ; l'avertissement n'est affiché qu'une fois par canal pour ne pas inonder le moniteur |
| **Les noms sont mis en minuscules** | MQTT distingue majuscules et minuscules : `Salon` et `salon` seraient deux topics différents, un classique des « ça ne marche pas » |
| **Pas de `/` au début des topics** | un `/` initial crée un premier niveau vide dans MQTT, que les outils affichent comme un niveau fantôme |
| **La zone est une énumération, pas un texte** | l'éditeur la propose, une faute de frappe est impossible, et on contrôle la liste à un seul endroit |
| **Construction du topic étape par étape** (`zone().module().component().property()`) | chaque point `.` fait proposer à l'éditeur la suite logique, ce qui guide les débutants |
| **`FloatOut`, `TextOut`, `TableOut`, `Input` sont de petites poignées** | ils ne contiennent qu'un pointeur vers un canal géré par la bibliothèque : on peut les copier, les déclarer en variable globale avant `begin`, et un appel sur une poignée invalide est ignoré (avec un message) au lieu de faire planter la carte |
| **Les canaux ne sont jamais détruits** | on les crée une fois dans `setup()` et ils vivent jusqu'à l'extinction ; ça évite tout risque d'utiliser un canal déjà libéré |
| **L'objet global `CexiiMqtt` est créé au démarrage de la carte** | son constructeur ne fait presque rien (il branche juste la fonction qui signale les erreurs) : toucher au matériel avant `setup()` serait dangereux |
| **ArduinoJson seulement pour lire** | lire du JSON quelconque est compliqué (espaces, ordre des champs, échappements), alors qu'écrire notre propre format est simple et se fait sans allouer de mémoire |
| **Tout ce qui est limite est dans `CexiiMqttConfig.h`** | un seul fichier à modifier, au lieu de chiffres dispersés dans le code |
| **Les messages de la bibliothèque sont en français et sans accent dans le code d'erreur** | les personnes visées parlent français ; les accents posent parfois problème dans certains moniteurs série |
| **Deux versions du code pour le framework ESP32 2.x et 3.x** (`#if ESP_ARDUINO_VERSION_MAJOR >= 3`) | la configuration du client MQTT et d'Ethernet a changé de forme entre les deux. Seule la 2.0.17 a été compilée : la branche 3.x est écrite d'après la documentation mais jamais essayée |

### Les valeurs en dur, ce que vaut chaque nombre

Le client MQTT utilise : *keepalive* de 15 s (un signe de vie régulier pour que le serveur sache que la carte est
toujours là), reprise automatique de la connexion toutes les 3 s, tampon de 1 024 octets, 8 Ko de pile pour sa
tâche. La file de réception a 8 places, celle des erreurs 10. Les cartes Olimex PoE utilisent la puce réseau
LAN8720 (adresse 0, alimentation sur la broche 12, MDC 23, MDIO 18, horloge sur la broche 17 — ou 0 si la
mémoire PSRAM est activée, car elle occupe alors la 17).

### Taille

Un programme vide pour ESP32 occupe déjà environ 233 Ko de mémoire de programme. Avec la bibliothèque (WiFi,
Ethernet, client MQTT, ArduinoJson et notre code), un exemple complet occupe environ 970 à 980 Ko (74 % de la
place disponible par défaut) et 47 Ko de mémoire vive.

---

## 9. Les pièges rencontrés

Ces trois histoires expliquent pourquoi certaines parties du code sont écrites comme elles le sont.

**La file de sortie d'esp-mqtt qui prend du retard.** La première version envoyait chaque message avec
`esp_mqtt_client_enqueue`, qui dépose le message dans une file interne du client. D'après ce qu'on
a observé sur le serveur, ce client n'en sort qu'**environ un par seconde** (la lecture du code montre qu'il ne
traite qu'un message en attente par tour, et qu'il patiente en attendant d'éventuelles données à lire), et il
**jette** les messages vieux de plus de 30 s. À 20 messages par seconde, la file grossissait jusqu'à ce que les
données arrivent avec 30 secondes de retard, et les messages `infos`, placés derrière toute cette file,
n'arrivaient jamais. Le symptôme visible : le `seq` de la carte avançait de 20 par seconde mais le serveur n'en
recevait qu'un, et l'horodatage `ts` d'un message avait 30 s de retard sur l'heure à laquelle il était reçu. La
correction : `esp_mqtt_client_publish`, qui envoie tout de suite. Contrepartie : si le réseau est très lent,
l'écriture bloque brièvement `loop()`. (Cette correction est dans le code, mais n'a pas encore été confirmée sur
carte par un nouvel essai.)

**Le message qui arrive par morceaux.** Une table de plusieurs Ko arrive en plusieurs événements. Ne traiter que le
premier donnerait un message tronqué. D'où le réassemblage décrit plus haut.

**Le message d'erreur qui crée une erreur.** Envoyer une erreur par le réseau fait écrire des logs ; si ces logs
sont eux-mêmes capturés et envoyés, on obtient une boucle sans fin. D'où la mise en file systématique, le tri des
doublons, et la limite de 3 erreurs par tour.

---

## 10. Comment c'est testé

| Quoi | Comment | Résultat actuel |
|---|---|---|
| Règles de noms, topics, clés | tests automatiques sur ordinateur (`pio test -e native`) | 26 tests passent |
| JSON envoyé et reçu (nombres, texte, table) | idem : on fabrique un message, on le relit, on compare au caractère près | idem |
| Cadences (événement, continu, 30 Hz) | idem : on simule 10 secondes de temps et on compte les envois | idem |
| Limites (65 rangées, 17 colonnes, plus de 8 Ko) | idem | idem |
| Tout le reste (réseau, MQTT, erreurs, ping) | compilation des 10 exemples pour ESP32 et ESP32-POE-ISO ; essais sur une vraie carte avec le serveur EMQX | tout compile |

Ce qui n'est **pas** testé automatiquement : tout ce qui touche à la carte réelle et au serveur. À ce jour, les
essais sur carte ont confirmé l'envoi de nombres jusqu'au serveur et leur réception. Les messages `infos`,
`error` et `ping`, les tables, le texte, la mesure du processeur et la capture des erreurs n'ont pas encore été
confirmés par un essai complet : leur code a été relu et compile, rien de plus. Si vous constatez un écart avec
ce qui est décrit ici, c'est le code qui fait foi, et c'est une information précieuse.

---

## 11. Les limites connues

- **Pas de chiffrement** : la connexion est en `ws://` (pas `wss://`). Elle convient à un réseau interne.
- **Données en QoS 0** : un message perdu n'est pas renvoyé. C'est un choix cohérent avec des valeurs qui changent vite
  (la suivante remplace la perdue), pas avec une commande qu'il faudrait absolument livrer.
- **Écriture bloquante** : `update()` attend la fin de l'écriture réseau.
- **Pas d'abonnements génériques** (`#`, `+`) : un `Input` correspond à un topic exact.
- **MQTT 5, ESP32-S2/S3/C3, framework ESP32 3.x** : non pris en charge ou non essayés.
- **Mémoire** : une table de 64×16 cases peut occuper plusieurs dizaines de Ko une fois décodée (chaque case est
  un texte). C'est pourquoi la taille est plafonnée à 8 Ko par message.

---

## 12. Par où commencer pour lire le code

1. [`src/CexiiMqttConfig.h`](../src/CexiiMqttConfig.h) : court, il donne tous les chiffres.
2. [`src/CexiiMqtt.h`](../src/CexiiMqtt.h) : l'interface, très commentée.
3. [`src/CexiiMqttCore.cpp`](../src/CexiiMqttCore.cpp) : les règles, le JSON, les cadences. Lisez en parallèle
   [`test/test_core/test_main.cpp`](../test/test_core/test_main.cpp) : les tests montrent des exemples concrets.
4. [`src/CexiiMqtt.cpp`](../src/CexiiMqtt.cpp) : `begin_`, `update()`, `publishOutputs_` : le chef d'orchestre.
5. [`src/CexiiMqttClient.cpp`](../src/CexiiMqttClient.cpp) : le gestionnaire d'événements MQTT.
6. [`src/CexiiMqttSystem.cpp`](../src/CexiiMqttSystem.cpp) : erreurs, infos, ping.

Pour essayer vous-même : modifier un chiffre dans `CexiiMqttConfig.h` (par exemple ajouter une zone), puis
lancer `pio test -e native` pour vérifier que les règles tiennent encore, et compiler un exemple avec `pio run`.

---

## 13. Glossaire

| Mot | Sens |
|---|---|
| **MQTT** | protocole de messagerie léger : on publie des messages dans des « topics », d'autres s'y abonnent |
| **Broker** | le serveur MQTT (ici EMQX) qui reçoit et redistribue les messages |
| **Topic** | l'adresse d'un message, comme un chemin de dossiers (`saillance/jam/…`) |
| **Publier / s'abonner** | envoyer un message sur un topic / demander à recevoir ce qui y est envoyé |
| **QoS** | « qualité de service » : 0 = on envoie une fois sans accusé de réception, 1 = on s'assure qu'il est arrivé |
| **WebSocket** | une connexion permanente construite sur le web ; MQTT peut s'y glisser |
| **esp-mqtt** | le client MQTT fourni par Espressif, le fabricant de l'ESP32 |
| **JSON** | format texte pour décrire des données : `{"v":1,"val":[0.5]}` |
| **Tâche (FreeRTOS)** | un petit programme qui tourne « en même temps » que les autres sur l'ESP32 |
| **File d'attente** | un tube où une tâche dépose des messages et une autre les retire, sans collision |
| **NTP** | service qui donne l'heure exacte par le réseau |
| **Tas (heap)** | la mémoire vive disponible pour les allocations pendant que le programme tourne |
| **Hook / point d'accroche** | un endroit où l'on « se branche » sur un mécanisme existant (ici, l'affichage des logs) |
| **Latence** | le temps qu'un message met à faire l'aller-retour jusqu'au serveur |
