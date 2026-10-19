# Utiliser Claude pour programmer avec CeXii MQTT

Il existe un **skill Claude** pour la bibliothèque CeXii MQTT. Un skill, c'est un mode d'emploi que Claude
lit tout seul quand vous parlez de la bibliothèque. Une fois installé, vous pouvez demander à Claude, en
français et avec vos mots :

- d'**écrire un programme complet** pour votre carte ESP32 (prêt à copier dans l'Arduino IDE) ;
- d'**expliquer** une partie du code ou un message d'erreur du moniteur série ;
- de **trouver pourquoi** vos messages n'arrivent pas.

Claude connaît les règles de la bibliothèque (24 caractères pour le nom, valeurs de 0 à 1, 16 colonnes pour
une table…), le format des topics et les zones (`jam`, `ecrans`, `globale`, `regie`). Vous n'avez pas besoin
de les lui expliquer.

**Sur cette page** : [Installer](#1-installer-le-skill) · [Lui parler](#2-comment-lui-parler) ·
[Utiliser sa réponse](#3-utiliser-la-réponse-de-claude) · [Exemples](#4-exemples) ·
[Bon à savoir](#5-bon-à-savoir) · [Dépannage](#6-ça-ne-marche-pas-)

---

## 1. Installer le skill

On vous a remis un fichier nommé **`cexii-mqtt.skill`**. Choisissez l'application que vous utilisez.

### Dans Claude (site claude.ai ou application Claude)

1. Ouvrez les **Paramètres** de Claude.
2. Allez dans **Capacités** (en anglais *Capabilities*), puis dans la section **Skills**.
3. Cliquez sur **Téléverser un skill** (*Upload skill*) et choisissez le fichier `cexii-mqtt.skill`.
4. Vérifiez que **cexii-mqtt** apparaît dans la liste et qu'il est **activé**.

Les noms des menus changent un peu selon la version de l'application et le forfait. Si vous ne trouvez pas
« Skills », cherchez ce mot dans les paramètres, ou demandez à la personne qui gère votre compte Claude
(certains forfaits ne permettent pas d'ajouter un skill).

### Dans Claude Code (programmation dans le terminal)

Copiez le **dossier** `cexii-mqtt` (celui du dépôt : `skill/cexii-mqtt`) dans le dossier `.claude/skills/` de
votre projet, ou dans `~/.claude/skills/` pour l'avoir dans tous vos projets :

```bash
mkdir -p ~/.claude/skills
cp -R chemin/vers/cexii-mqtt ~/.claude/skills/
```

Puis **fermez et rouvrez** Claude Code : le skill est lu au démarrage.

---

## 2. Comment lui parler

Parlez normalement. Pas besoin de nommer le skill ni la bibliothèque : dès que vous parlez de votre ESP32 et
d'envoyer ou de recevoir des messages, Claude s'en sert.

Plus votre demande est précise, moins Claude a de questions à poser. Essayez de donner :

| Information | Exemple |
|---|---|
| **La carte et le réseau** | « une ESP32-POE-ISO en câble réseau » ou « une ESP32 normale en WiFi » |
| **Ce qu'elle doit envoyer ou recevoir** | « la valeur d'un potentiomètre », « un texte », « une table de 3 colonnes » |
| **La zone** | `jam`, `ecrans`, `globale` ou `regie` |
| **Le nom de l'appareil** | « controlbouton » (lettres et chiffres seulement, 24 caractères au maximum) |
| **Quand envoyer** | « en continu 20 fois par seconde » ou « seulement quand ça change » |

Si vous ne savez pas, dites-le : Claude choisira une valeur raisonnable et vous l'expliquera.

**Ne collez pas vos mots de passe** dans la conversation. Claude met des valeurs génériques (`"NomDuWifi"`,
`"MotDePasse"`) que vous remplacez vous-même dans le programme.

---

## 3. Utiliser la réponse de Claude

1. **Copiez le programme** dans l'Arduino IDE (ou dans PlatformIO).
2. **Changez les lignes « À MODIFIER »** tout en haut : le nom de l'appareil et, en WiFi, le nom du réseau
   et son mot de passe.
3. Vérifiez dans l'Arduino IDE le **type de carte** (Outils > Type de carte) et **Core Debug Level : Error**.
4. **Téléversez**, puis ouvrez le **moniteur série** à la vitesse **115200**. Vous devez voir
   `MQTT connecte`.
5. **Testez avec MQTTX** : Claude vous donne le topic à surveiller et, pour tester la réception, le message à
   envoyer.

Si ça ne fonctionne pas, **copiez ce que dit le moniteur série** dans la conversation et demandez à Claude
ce que ça veut dire.

---

## 4. Exemples

### Écrire un programme

> « Écris-moi un programme pour une ESP32-POE-ISO branchée en câble réseau. Un bouton sur la broche 34 doit
> envoyer 1 quand je l'appuie et 0 quand je le relâche, dans la zone régie. »

Claude écrit le programme complet, vous dit quel topic regarder dans MQTTX et ce qu'il faut modifier.

### Recevoir des données

> « Je veux que ma carte allume une LED plus ou moins fort selon une valeur que TouchDesigner envoie sur
> `saillance/jam/lumiere/plafonnier/niveau`. »

### Envoyer une table

> « Comment j'envoie une table de 3 rangées et 4 colonnes, avec des cases qui changent toutes les secondes ? »

### Comprendre un problème

> « Mon moniteur série dit "cle zzz inconnue sur saillance/jam/lumiere/bande1/couleur". Qu'est-ce que ça veut
> dire ? »

> « Ma carte est connectée mais je ne reçois pas les messages `infos` dans MQTTX. »

### Adapter un programme existant

Collez votre programme et dites ce que vous voulez changer :

> « Voici mon programme. Je voudrais envoyer aussi un texte toutes les 2 secondes, dans la zone écrans. »

---

## 5. Bon à savoir

- **Claude peut se tromper.** Compilez toujours le programme et testez-le avec MQTTX. Si une réponse
  semble étrange, demandez-lui de vérifier ou de l'expliquer.
- **La bibliothèque s'appelle maintenant « CeXii MQTT »** (`#include <CexiiMqtt.h>`, objet `CexiiMqtt`).
  Si Claude écrit `Saillance.update()` ou `#include <Saillance.h>`, votre skill est une ancienne version :
  redemandez le fichier le plus récent.
- **Le skill suit la version de la bibliothèque.** Si on ajoute une zone ou si une limite change, il faut
  réinstaller la dernière version du fichier `cexii-mqtt.skill`.
- **Les topics commencent toujours par `saillance/`**, c'est voulu : c'est le nom du projet dans l'adresse des
  messages.
- Le skill aide à écrire du code : il ne se connecte pas au serveur et ne voit pas votre carte. Pour qu'il
  comprenne ce qui se passe, il lui faut ce que vous voyez (moniteur série, MQTTX).

---

## 6. Ça ne marche pas ?

| Problème | Piste |
|---|---|
| Claude ne semble pas connaître la bibliothèque | vérifiez que le skill **cexii-mqtt** est dans la liste et activé (section 1) ; ouvrez une **nouvelle conversation** ; ou écrivez « utilise le skill cexii-mqtt » |
| Je ne trouve pas « Skills » dans les paramètres | votre forfait ne le permet peut-être pas : demandez à la personne qui gère le compte |
| Le téléversement est refusé | redemandez le fichier `cexii-mqtt.skill` : il est peut-être abîmé ou incomplet |
| Claude propose une zone qui n'existe pas | les zones actuelles sont `jam`, `ecrans`, `globale`, `regie` ; votre skill est peut-être ancien |
| Le programme ne compile pas | collez l'erreur dans la conversation avec la version de la carte (Outils > Type de carte) |
| Le programme compile mais rien n'arrive | voir la table « Ça ne marche pas ? » du [README](../README.md#8-ça-ne-marche-pas-), ou collez le moniteur série à Claude |

Pour toutes les fonctions en détail : [docs/REFERENCE.md](REFERENCE.md).
