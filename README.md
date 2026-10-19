# CeXii MQTT — envoyer et recevoir des messages avec un ESP32

Cette bibliothèque permet à une carte **ESP32** (ou **ESP32 PoE**) d'envoyer et de recevoir des
messages avec le reste du système CeXii (TouchDesigner, autres cartes…), en **WiFi** ou avec un
**câble réseau**. Vous n'avez pas besoin de connaître MQTT.

```
  Votre carte ESP32  ──── WiFi ou câble ────►  Serveur (broker)  ────►  TouchDesigner,
                     ◄─────────────────────    10.0.21.210       ◄────  autres cartes…
```

**Sur cette page** : [1. Installer](#1-installer) · [2. Premier programme](#2-votre-premier-programme) ·
[3. Comprendre](#3-comprendre-en-une-minute) · [4. Je veux…](#4-je-veux) · [5. Limites](#5-les-limites) ·
[6. Envois automatiques](#6-ce-que-la-carte-envoie-toute-seule) · [7. Exemples](#7-les-exemples) ·
[8. Dépannage](#8-ça-ne-marche-pas-)

> La bibliothèque compile et ses règles sont testées, mais elle n'a pas encore été éprouvée à fond sur
> les cartes. Si quelque chose ne se comporte pas comme décrit ici, notez-le : c'est utile.

---

## 1. Installer

### Dans l'Arduino IDE

1. **Outils > Gestionnaire de cartes** → cherchez **esp32** (par Espressif) → *Installer*.
   (Testé avec la version 2.0.17.)
2. **Outils > Gérer les bibliothèques** → cherchez **ArduinoJson** → *Installer* (version 7 ou plus).
3. **Croquis > Inclure une bibliothèque > Ajouter la bibliothèque .ZIP…** → choisissez le fichier
   `CeXii_MQTT.zip`.
   *Si une ancienne version est déjà installée, supprimez d'abord son dossier (`Saillance` ou `CeXii_MQTT`) dans `Documents/Arduino/libraries/`.*
4. **Outils > Type de carte** → **OLIMEX ESP32-POE-ISO** (ou **OLIMEX ESP32-POE**).
   Pour une carte ESP32 classique (WiFi seulement) : **ESP32 Dev Module**.
5. **Outils > Core Debug Level** → **Error** (pour que les erreurs de la carte soient aussi signalées).
6. Redémarrez l'Arduino IDE, puis ouvrez un exemple : **Fichier > Exemples > CeXii MQTT > 01_Envoyer_nombres**.
7. Branchez la carte en USB **et le câble réseau** (Ethernet), choisissez le **Port** dans le menu Outils,
   puis cliquez *Téléverser* (la flèche →).
8. Ouvrez **Outils > Moniteur série** et réglez la vitesse sur **115200**.

<details>
<summary><b>Avec PlatformIO</b> (cliquez pour afficher)</summary>

Dans le fichier `platformio.ini` de votre projet :

```ini
[env:esp32]
platform = espressif32
board = esp32-poe-iso       ; ESP32-POE : esp32-poe     ESP32 classique : esp32dev
framework = arduino
monitor_speed = 115200
build_flags = -DCORE_DEBUG_LEVEL=1
lib_deps =
    https://github.com/VOTRE-COMPTE/cexii-mqtt.git
```

Chaque dossier de `examples/` contient aussi un `platformio.ini` prêt à l'emploi.
</details>

---

## 2. Votre premier programme

Ouvrez l'exemple **01_Envoyer_nombres**. Il est écrit pour une carte **ESP32 PoE branchée en Ethernet**
(un câble réseau, pas de WiFi). Voici l'essentiel :

```cpp
#include <CexiiMqtt.h>

FloatOut couleur;                       // un "tuyau" pour envoyer des nombres

void setup() {
  Serial.begin(115200);

  // 1) Démarrer : type de carte et nom de votre appareil (le câble suffit)
  CexiiMqtt.beginEthernet(Board::Esp32PoeIso, "monappareil");

  // 2) Choisir où envoyer : zone, module, composante, propriété
  Topic topic = CexiiMqtt.topic(Zone::Globale, "lumiere", "bande1", "couleur");

  // 3) Créer l'envoi : le topic, le nom de chaque valeur, 20 envois/s max, en continu
  couleur = CexiiMqtt.floatOut(topic, { "r", "g", "b" }, 20, SendMode::Continuous);
}

void loop() {
  CexiiMqtt.update();                   // 4) OBLIGATOIRE, à chaque tour de loop()

  couleur.set("r", 0.5);                // 5) donner les valeurs, toujours entre 0.0 et 1.0
  couleur.set("g", 0.2);
  couleur.set("b", 1.0);
}
```

**À modifier avant de téléverser** : le nom de l'appareil. Choisissez des lettres et des chiffres
seulement (24 au maximum), **différent pour chaque carte**. Avec `Board::Esp32PoeIso` et
`"monappareil"`, le nom sur le serveur sera `esp32poeiso_monappareil`.

**Les trois règles à retenir**
1. Appelez `CexiiMqtt.update()` à **chaque** tour de `loop()`.
2. Évitez les longs `delay()` dans `loop()` : ils ralentissent les envois et la réception.
3. Les nombres sont **toujours entre 0.0 et 1.0**.

Dans le moniteur série, vous devriez voir :

```
[CeXii MQTT] appareil : esp32poeiso_monappareil
[CeXii MQTT] topic erreurs : saillance/sys/esp32poeiso_monappareil/error
[CeXii MQTT] topic infos   : saillance/sys/esp32poeiso_monappareil/infos (toutes les 5 s)
[CeXii MQTT] topic ping    : saillance/sys/esp32poeiso_monappareil/ping (toutes les 60 s)
[CeXii MQTT] Ethernet : demarrage...
[CeXii MQTT] envoi    -> saillance/globale/lumiere/bande1/couleur  [r,g,b]  max 20 Hz, continu
[CeXii MQTT] Ethernet connecte, IP 10.0.21.57
[CeXii MQTT] MQTT : connexion a ws://10.0.21.210:8083/mqtt en tant que "esp32poeiso_monappareil"...
[CeXii MQTT] MQTT connecte
```

Quand vous voyez **MQTT connecte**, ça marche. Pour regarder les messages, installez le logiciel
gratuit [MQTTX](https://mqttx.app), connectez-le en WebSocket à `ws://10.0.21.210:8083/mqtt` et
abonnez-vous à `saillance/#`.

### Avec le WiFi (carte ESP32 classique, ou carte PoE sans câble)

Remplacez la ligne `beginEthernet` par :

```cpp
CexiiMqtt.beginWifi(Board::Esp32, "monappareil", "NomDuWifi", "MotDePasse");
```

Le reste du programme est identique. Les autres exemples (02 à 08 et 10) utilisent le WiFi : modifiez
simplement les lignes « À MODIFIER » au début de chacun.

---

## 3. Comprendre en une minute

### Le topic : l'adresse d'un message

Chaque message est envoyé à une « adresse » qu'on appelle **topic**. Elle a 5 parties :

```
   saillance / globale / lumiere / bande1 / couleur
       │         │         │         │         └─ propriété    (vous choisissez, 12 caractères max)
       │         │         │         └─ composante               (vous choisissez, 12 max)
       │         │         └─ module                              (vous choisissez, 12 max)
       │         └─ zone : à choisir dans la liste → jam, ecrans, globale, regie
       └─ toujours « saillance » (ajouté tout seul)
```

* Dans le code, on écrit seulement : `CexiiMqtt.topic(Zone::Globale, "lumiere", "bande1", "couleur")`.
* Pour les parties libres : lettres, chiffres, `_` ou `-`. Pas d'espace ni d'accent. Tout est mis en minuscules.
* Dans l'éditeur, tapez `Zone::` et il vous propose la liste des zones.

### Les trois formats de données

| Format | Pour envoyer… | Dans le code | Exemple |
|---|---|---|---|
| **Nombres** | une ou plusieurs valeurs entre 0 et 1, chacune avec un nom (une « clé ») | `FloatOut` | r = 0.5, g = 0.2 |
| **Texte** | une phrase | `TextOut` | "bonjour" |
| **Table** | un tableau de textes (rangées × colonnes) | `TableOut` | `a / 0.1 / on` sur une rangée |

### Les deux modes d'envoi

* **`SendMode::Event`** — la carte n'envoie que **quand quelque chose change** (pratique pour un bouton).
* **`SendMode::Continuous`** — la carte envoie **toujours**, à la cadence demandée, même si rien ne change.

Dans les deux cas, vous choisissez une **cadence maximale** de 1 à 30 envois par seconde.

---

## 4. Je veux…

### …envoyer des nombres

```cpp
FloatOut f;
f = CexiiMqtt.floatOut(topic, { "r", "g", "b" }, 30, SendMode::Event);  // dans setup()

f.set("r", 0.7);        // dans loop() : donner une valeur à une clé
```
→ exemples **01_Envoyer_nombres** et **02_Envoyer_capteur** (lire un potentiomètre).

Les clés (`"r"`, `"g"`…) : 6 caractères maximum, lettres/chiffres/`_`. Jusqu'à 16 valeurs par topic.

### …envoyer du texte

```cpp
TextOut t;
t = CexiiMqtt.textOut(topic, 5, SendMode::Event);   // dans setup()

t.set("bonjour");                                   // dans loop()
```
→ exemple **03_Envoyer_texte**.

### …envoyer une table

```cpp
TableOut tab;
tab = CexiiMqtt.tableOut(topic, 5, SendMode::Event);   // dans setup()
tab.addRow({ "a", "0.1", "on" });                      // ajouter une rangée
tab.addRow({ "b", "0.5", "off" });

tab.set(1, 2, "on");        // dans loop() : rangée 1, colonne 2
tab.set(1, 1, 0.25);        // un nombre est converti en texte
```
→ exemple **04_Envoyer_table**. Les rangées et colonnes se comptent à partir de 0.

### …recevoir des nombres

```cpp
Input niveau;
niveau = CexiiMqtt.input(topic);                 // dans setup()

float v = niveau.get("niv");                     // dans loop() : dernière valeur reçue (0 si rien)
if (niveau.isNew()) { /* un nouveau message vient d'arriver */ }
```
→ exemple **05_Recevoir_nombres**. Le nom (`"niv"`) doit être le même que chez celui qui envoie.

### …recevoir du texte ou une table

Le plus simple : une **fonction appelée automatiquement** à chaque message.

```cpp
CexiiMqtt.input(topic, [](const Message& m) {      // dans setup()
  if (m.isText)  Serial.println(m.text);           // du texte
  if (m.isTable) Serial.println(m.cell(0, 1));     // une table : la case rangée 0, colonne 1
});
```
→ exemples **06_Recevoir_texte** et **07_Recevoir_table**.
Pour un message de **nombres**, parcourez-les ainsi :
`for (uint8_t i = 0; i < m.count; i++) Serial.println(m.value(i));`

### …tout faire à la fois

→ exemple **08_Tous_les_formats** (envoie les 3 formats et reçoit les 3).

### …savoir si je suis connecté, ou signaler une erreur

```cpp
if (CexiiMqtt.connected()) { /* connecté au serveur */ }
CexiiMqtt.reportError("capteur", "lecture impossible");      // envoyée sur le topic /error
```
→ exemple **10_Etat_et_erreurs**.

---

## 5. Les limites

| Quoi | Limite |
|---|---|
| Nom de l'appareil | 24 caractères, lettres et chiffres seulement |
| Module, composante, propriété | 12 caractères chacun |
| Clé d'une valeur | 6 caractères |
| Valeurs par topic (nombres) | 16 |
| Valeur d'un nombre | entre 0.0 et 1.0 (sinon ramenée dans l'intervalle) |
| Cadence d'envoi | 1 à 30 par seconde |
| Texte | 128 caractères |
| Table | 64 rangées, 16 colonnes, 8 Ko par message |

Un nom, un topic ou une clé qui ne respecte pas ces règles est **refusé avec un message clair** dans le
moniteur série.

---

## 6. Ce que la carte envoie toute seule

Sans rien écrire dans votre programme, la carte envoie sur le serveur :

| Topic | Quand | Pour quoi faire |
|---|---|---|
| `saillance/sys/<appareil>/infos` | toutes les 5 secondes | savoir si la carte va bien (vitesse, mémoire, charge…) |
| `saillance/sys/<appareil>/error` | quand il y a une erreur | savoir pourquoi quelque chose a planté |
| `saillance/sys/<appareil>/ping` | toutes les 60 secondes | mesurer la vitesse de la connexion (champ `lat`) |

`<appareil>` est le nom complet, par exemple `esp32_monappareil` ou `esp32poeiso_controlbouton`
(type de carte + « _ » + le nom que vous avez choisi).

Le détail de chaque champ est dans [docs/REFERENCE.md](docs/REFERENCE.md#messages-automatiques).

---

## 7. Les exemples

Dans l'Arduino IDE : **Fichier > Exemples > CeXii MQTT**. Chaque exemple commence par quelques lignes
« À MODIFIER » (nom de l'appareil, WiFi) et explique comment le tester.

| Exemple | Ce qu'il fait |
|---|---|
| `01_Envoyer_nombres` | envoie 3 nombres qui changent (ESP32 PoE en Ethernet) |
| `02_Envoyer_capteur` | envoie la valeur d'un potentiomètre, seulement quand elle change |
| `03_Envoyer_texte` | envoie un texte chaque seconde |
| `04_Envoyer_table` | envoie une table dont certaines cases changent |
| `05_Recevoir_nombres` | allume une LED selon une valeur reçue |
| `06_Recevoir_texte` | affiche chaque texte reçu |
| `07_Recevoir_table` | affiche chaque table reçue |
| `08_Tous_les_formats` | envoie et reçoit les trois formats |
| `09_Ethernet_PoE` | envoie l'état d'un bouton, par câble réseau |
| `10_Etat_et_erreurs` | connexion, nom de l'application, erreur personnalisée |

---

## 8. Ça ne marche pas ?

Regardez d'abord le **moniteur série (115200)** : la bibliothèque y explique les problèmes.

| Ce que vous voyez | Ce que ça veut dire |
|---|---|
| `nom d'appareil ... invalide` | lettres et chiffres seulement, 24 caractères maximum |
| `module/composante/propriete ... invalide` | 12 caractères max, lettres/chiffres/`_`/`-`, pas d'espace |
| `cle ... inconnue` | cette clé n'est pas dans la liste donnée à `floatOut` |
| Rien après `WiFi : connexion a ...` | nom ou mot de passe du WiFi incorrect |
| `Ethernet debranche` | vérifiez le câble, et le type de carte choisi |
| `broker injoignable` | la carte n'est pas sur le même réseau que `10.0.21.210` |
| `connexion refusee par le broker` | le serveur refuse ce nom d'appareil |
| `FloatOut non valide` (ou TextOut/TableOut) | la création a échoué : lisez le message juste au-dessus |
| `message illisible sur ...` | le message reçu n'a pas le bon format |
| Des caractères bizarres | la vitesse du moniteur série n'est pas 115200 |
| Connecté, mais rien n'est envoyé | `CexiiMqtt.update()` manque dans `loop()`, ou le mode est `Event` et rien ne change |
| Je ne reçois rien | le topic doit être **exactement** le même, lettre pour lettre, chez l'envoyeur |

---

## Pour aller plus loin

* [docs/GUIDE_SKILL.md](docs/GUIDE_SKILL.md) : le skill Claude « cexii-mqtt » qui aide à écrire du code avec cette
  bibliothèque ; comment l'utiliser, le partager et le publier.
* [docs/REFERENCE.md](docs/REFERENCE.md) : toutes les fonctions, le format exact des messages, les champs
  de `infos`, les codes d'erreur, la configuration (ajouter une zone, changer l'adresse du serveur).
