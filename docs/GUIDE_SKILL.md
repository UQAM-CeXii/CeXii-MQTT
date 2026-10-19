# Le skill Claude « cexii-mqtt » : l'utiliser et le publier

Un **skill** est un petit dossier de consignes que Claude charge tout seul quand la conversation s'y prête.
Celui-ci apprend à Claude à utiliser la bibliothèque CeXii MQTT : écrire un sketch ESP32 qui marche du premier
coup, respecter les limites (24 caractères, 0 à 1, 16 colonnes…), choisir le bon topic, tester avec MQTTX et
dépanner à partir du moniteur série.

Il se trouve dans le dépôt : [`skill/cexii-mqtt/`](../skill/cexii-mqtt/).

**Sur cette page** : [Ce qu'il y a dedans](#1-ce-quil-y-a-dedans) · [L'utiliser](#2-lutiliser) ·
[Le partager](#3-le-partager-aux-autres) · [Le garder à jour](#4-le-garder-à-jour) ·
[Publier la bibliothèque](#5-publier-la-bibliothèque-elle-même) · [Dépannage](#6-ça-ne-marche-pas-)

---

## 1. Ce qu'il y a dedans

```
skill/cexii-mqtt/
├── SKILL.md                      le cœur : quand s'en servir, la marche à suivre, les règles
├── references/
│   ├── api.md                    toutes les fonctions de la bibliothèque
│   ├── examples.md               7 sketchs complets qui compilent (à copier et adapter)
│   ├── messages.md               formats JSON, champs de infos, codes d'erreur, messages de test MQTTX
│   └── troubleshooting.md        tableau de dépannage
└── scripts/
    └── check_names.py            vérifie un nom d'appareil, un topic, des clés avant d'écrire le code
```

Claude ne lit que `SKILL.md` au départ, puis ouvre les fichiers de `references/` seulement quand il en a
besoin : ça garde la conversation légère.

---

## 2. L'utiliser

### Dans Claude Code (terminal ou onglet Code de l'application Claude)

Il suffit que le dossier soit à l'un de ces deux endroits :

| Où | Pour qui | Commande |
|---|---|---|
| `~/.claude/skills/cexii-mqtt/` | vous, dans tous vos projets | voir ci-dessous |
| `<votre projet>/.claude/skills/cexii-mqtt/` | toute personne qui ouvre ce projet | voir ci-dessous |

Pour vous (la **copie** garde le skill figé ; le **lien** le met à jour tout seul quand vous faites un `git pull`) :

```bash
mkdir -p ~/.claude/skills
ln -s /Users/gabpelland/Developpeur/Github/saillance-mqtt/skill/cexii-mqtt ~/.claude/skills/cexii-mqtt
```

Pour un projet (le dossier est alors versionné avec le projet) :

```bash
mkdir -p .claude/skills
cp -R /chemin/vers/saillance-mqtt/skill/cexii-mqtt .claude/skills/
```

Ensuite, **ouvrez une nouvelle session** Claude Code : les skills sont lus au démarrage.

### Dans Claude.ai et l'application Claude (discussion, Cowork)

1. Récupérez le fichier **`cexii-mqtt.skill`** (voir [Le partager](#3-le-partager-aux-autres)).
2. Dans Claude : **Paramètres > Capacités (Capabilities) > Skills > Téléverser un skill**, puis choisissez le fichier.
3. Activez le skill dans la liste.

Les noms des menus changent un peu selon la version et le forfait, et ce réglage peut être réservé à certains
forfaits : si vous ne trouvez pas l'option, cherchez « Skills » dans les paramètres ou demandez à la personne qui
administre votre compte.

### Vérifier que ça marche

Ouvrez une conversation et demandez, sans nommer le skill :

> « Écris-moi un sketch pour une ESP32-POE-ISO qui envoie la valeur d'un potentiomètre au serveur. »

Vous devriez voir que Claude :
- utilise `#include <CexiiMqtt.h>` et l'objet `CexiiMqtt` ;
- appelle `beginEthernet(Board::Esp32PoeIso, ...)` avec un nom en lettres et chiffres ;
- construit un topic de la forme `saillance/<zone>/…` avec une zone de la liste (`Jam`, `Ecrans`, `Globale`, `Regie`) ;
- garde les valeurs entre 0 et 1 et met `CexiiMqtt.update()` dans `loop()` ;
- vous dit comment vérifier le résultat dans MQTTX.

Autres demandes qui devraient le déclencher :
- « Mon ESP32 reçoit un texte mais rien ne s'affiche dans le moniteur série. »
- « Comment j'envoie une table de 3 colonnes à TouchDesigner ? »
- « Pourquoi je ne reçois pas les messages infos de ma carte ? »

Si Claude ne s'y réfère pas, dites-le-lui : « utilise le skill cexii-mqtt ».

---

## 3. Le partager aux autres

Choisissez selon les personnes à qui vous voulez le donner.

### A. Par le dépôt git (le plus simple pour l'équipe)

Le skill voyage avec la bibliothèque : tout le monde qui clone le dépôt a le dossier `skill/cexii-mqtt/`
et suit la section [2](#2-lutiliser). Pour qu'il soit actif automatiquement dans un projet, copiez-le dans le
`.claude/skills/` de ce projet et commitez-le.

### B. Par un fichier `.skill` (pour Claude.ai, ou pour envoyer par courriel)

Un fichier `.skill` est un simple zip du dossier du skill. Pour le créer depuis la racine du dépôt :

```bash
cd skill && zip -rX ../cexii-mqtt.skill cexii-mqtt -x "*.DS_Store" "*__pycache__*" && cd ..
```

Donnez le fichier `cexii-mqtt.skill` : chaque personne le téléverse comme décrit à la section 2.
Un fichier `.skill` ne se met pas à jour tout seul : redonnez-le à chaque nouvelle version.

### C. Comme plugin Claude Code (pour que les gens l'installent d'une commande)

Pour une diffusion plus large, un dépôt GitHub peut servir de « marketplace » de plugins. Il faut y ajouter
deux fichiers (le format exact évolue : confirmez-le dans la documentation officielle de Claude Code,
chapitre *Plugins*).

`.claude-plugin/plugin.json` :
```json
{
  "name": "cexii-mqtt",
  "description": "Aide à utiliser la bibliothèque CeXii MQTT sur ESP32",
  "version": "0.1.0"
}
```

`.claude-plugin/marketplace.json` :
```json
{
  "name": "cexii",
  "owner": { "name": "CeXii" },
  "plugins": [
    { "name": "cexii-mqtt", "source": "./", "description": "Skill CeXii MQTT pour ESP32" }
  ]
}
```

et le skill doit alors se trouver dans `skills/cexii-mqtt/` à la racine de ce dépôt (au lieu de `skill/`).
Les personnes l'installent ensuite avec :

```text
/plugin marketplace add VOTRE-COMPTE/cexii-mqtt
/plugin install cexii-mqtt@cexii
```

### D. Pour toute une organisation Claude

Si votre organisation utilise Claude Team ou Enterprise, les personnes qui l'administrent peuvent parfois ajouter
un skill pour tout le monde depuis les paramètres de l'organisation. Selon les offres, cette option n'existe pas :
vérifiez auprès de l'administration.

---

## 4. Le garder à jour

Le skill contient une **copie** des règles de la bibliothèque. S'il n'est pas mis à jour, Claude donnera des
conseils périmés (une zone qui n'existe plus, une limite qui a changé).

| Si vous changez… | Mettez à jour… |
|---|---|
| la liste des zones (`CEXII_ZONES`) | `SKILL.md` (tableau des règles), `scripts/check_names.py` (`DEFAULT_ZONES`), `docs/REFERENCE.md` |
| une limite (24, 12, 6, 30 Hz, 64×16, 8 Ko) | `SKILL.md`, `references/api.md`, `scripts/check_names.py`, README |
| un nom de fonction ou son comportement | `references/api.md` et les gabarits de `references/examples.md` |
| un champ de `infos`, un code d'erreur | `references/messages.md` |
| l'adresse du serveur ou les règles EMQX | `SKILL.md` (« Les faits à connaître »), `references/troubleshooting.md` |

Après une modification des exemples du skill, vérifiez qu'ils compilent toujours : chaque bloc `cpp` de
`references/examples.md` doit rester un sketch complet (copiez-le dans un projet PlatformIO et lancez `pio run`).

Ensuite : recréez le `.skill` (section 3B) et redonnez-le, ou faites un `git pull` si vous utilisez le lien.

---

## 5. Publier la bibliothèque elle-même

Le skill aide à *utiliser* la bibliothèque ; ceci explique comment la *mettre à disposition*.

**Sans registre (le plus simple)** : mettre le dépôt sur GitHub. Les personnes l'installent avec le zip dans
l'Arduino IDE, ou avec `lib_deps = https://github.com/VOTRE-COMPTE/cexii-mqtt.git` dans PlatformIO.

**Gestionnaire de bibliothèques de l'Arduino IDE** (public) :
1. Dépôt GitHub **public** avec `library.properties` à la racine (adresse `url=` à corriger) et un fichier `LICENSE`.
2. Vérifier : `arduino-lint --library-manager submit`.
3. Créer une release GitHub dont le tag est identique à `version=` (`0.1.0`).
4. Faire une demande d'ajout sur [arduino/library-registry](https://github.com/arduino/library-registry)
   (une ligne dans `repositories.txt`). Les versions suivantes sont détectées toutes seules (nouveau tag).

**Registre PlatformIO** (public) : `pio account register`, compléter `library.json` (`license`, `repository`,
`authors`), puis `pio pkg publish` à la racine. Une version publiée ne peut pas être remplacée.

**Attention** : publier dans un registre public rend visibles l'adresse du serveur interne (`10.0.21.210`) et la
structure des topics. Pour un outil interne, la diffusion par GitHub privé ou par zip suffit.

---

## 6. Ça ne marche pas ?

| Problème | Piste |
|---|---|
| Claude n'a pas l'air de connaître la bibliothèque | le dossier doit s'appeler exactement `cexii-mqtt`, contenir `SKILL.md`, et être dans `~/.claude/skills/` ou `.claude/skills/` ; ouvrez une **nouvelle** session |
| Claude Code ne voit pas le skill après un lien `ln -s` | vérifiez la cible du lien : `ls ~/.claude/skills/cexii-mqtt/SKILL.md` doit afficher le fichier |
| Le téléversement dans Claude.ai est refusé | le zip doit avoir le dossier `cexii-mqtt/` à sa racine, avec `SKILL.md` dedans (pas un dossier de plus) |
| Claude écrit `Saillance.update()` | il utilise une ancienne version du skill : remplacez-la, la bibliothèque s'appelle maintenant `CexiiMqtt` |
| Claude propose une zone inexistante | le skill n'est pas à jour avec `CEXII_ZONES` : voir la section 4 |
| Le script `check_names.py` ne démarre pas | il faut Python 3 (`python3 --version`) |

Pour tester le script à la main :

```bash
python3 skill/cexii-mqtt/scripts/check_names.py username esp32poeiso controlbouton
python3 skill/cexii-mqtt/scripts/check_names.py topic globale lumiere bande1 couleur --config src/CexiiMqttConfig.h
```
