# Messages : formats JSON et messages système

Tous les messages ont `v` (version du format, 1), `ts` (millisecondes : heure Unix si le NTP est synchronisé,
sinon temps écoulé depuis le démarrage) et `seq` (compteur par topic, repart à 0 au redémarrage).

## Données

```json
{"v":1,"ts":1792101066169,"seq":42,"val":[0.5,0.2,1],"f":"float","label":["r","g","b"]}
{"v":1,"ts":1792101066169,"seq":7,"val":"bonjour","f":"string"}
{"v":1,"ts":1792101066169,"seq":4,"val":[["a","0.1","on"],["b","0.5","off"]],"f":"table"}
```

- `float` : `val` liste de nombres 0 à 1 ; `label` donne la clé de chaque valeur, dans le même ordre.
- `string` : `val` est un texte, pas de `label`.
- `table` : `val` est une liste de rangées, chaque rangée une liste de textes (même les nombres), jamais de
  noms de colonnes, donc pas de `label`. La case rangée y, colonne x est `val[y][x]`.
  Les nombres reçus à la place de textes sont tolérés et convertis en texte.

Pour **tester la réception avec MQTTX**, publier sur le topic exact (ts et seq peuvent valoir 0) :

```json
{"v":1,"ts":0,"seq":0,"val":[0.7],"f":"float","label":["niv"]}
{"v":1,"ts":0,"seq":0,"val":"bonjour","f":"string"}
{"v":1,"ts":0,"seq":0,"val":[["a","0.1","on"],["b","0.5","off"]],"f":"table"}
```

## `saillance/sys/<username>/infos` (toutes les 5 s, et dès la connexion)

```json
{"v":1,"ts":1792101066169,"seq":42,"app":"","fps":59.8,"fpsmin":41.2,"msmax":24.3,"df":3,"cpu":18,"ram":61,"gpu":-1,"gpumem":-1,"ac":1,"performModeL":1,"lat":4,"rc":0}
```

`-1` = non disponible. Sur un ESP32 : `app` vide (ou `setAppName`), `fps` = tours de `loop()` par seconde,
`msmax` = tour de `loop()` le plus long des 5 s (`fpsmin` = 1000 / `msmax`), `df` = envois manqués,
`cpu` = % moyen des deux cœurs, `ram` = % du tas utilisé, `gpu` et `gpumem` = -1, `ac` = 1, `performModeL` = 1,
`lat` = dernier aller-retour du ping en ms (-1 si aucun ping n'est revenu), `rc` = reconnexions au broker.

## `saillance/sys/<username>/ping` (toutes les 60 s, et dès la connexion)

`{"v":1,"ts":…,"seq":…}` : la carte s'envoie le message, y est abonnée, et mesure le temps de retour
(`lat`), avec son horloge interne. Le compte EMQX doit donc pouvoir publier et s'abonner à ce topic.

## `saillance/sys/<username>/error` (à chaque erreur)

```json
{"v":1,"ts":1792101066169,"seq":0,"src":"wifi","msg":"lien WiFi perdu","sv":1,"t":200}
```

`sv` : 0 info, 1 attention, 2 erreur, 3 critique. `t` : 100 erreur système (logs), 101 redémarrage anormal,
102 mémoire basse, 200 lien perdu, 201 réseau impossible, 300 connexion refusée, 301 erreur MQTT,
400 nom invalide, 401 topic invalide, 402 clé invalide, 403 valeur invalide, 404 configuration invalide,
405 message reçu illisible, 900 erreur du programme. Une même erreur n'est envoyée qu'une fois toutes les 5 s.
