# Dépannage

Commencer par demander le texte du moniteur série (115200) : la bibliothèque y explique tout, en français,
avec le préfixe `[CeXii MQTT]`. Séquence normale au démarrage : `appareil`, trois `topic …`, `Ethernet : demarrage`
ou `WiFi : connexion`, `envoi ->` / `reception <-` pour chaque canal, `… connecte, IP …`, `MQTT connecte`.

| Constat | Cause probable | Action |
|---|---|---|
| `nom d'appareil … invalide` | espace, `_`, `-`, accent, vide ou plus de 24 caractères | `scripts/check_names.py device <nom>` |
| `module/composante/propriete … invalide` | 12 caractères max, lettres/chiffres/`_`/`-` | idem avec `segment` |
| `cle … inconnue` | clé absente de la liste donnée à `floatOut` | la déclarer à la création |
| `FloatOut non valide` (ou Text/Table/Input) | la création a échoué | lire le message juste au-dessus |
| `table trop grosse` | message de plus de 8192 octets | moins de rangées ou de texte |
| rien après `WiFi : connexion` | ssid ou mot de passe | vérifier ; le réseau doit voir `10.0.21.210` |
| `Ethernet debranche` | câble, ou mauvais type de carte | `Board::Esp32PoeIso` / `Esp32Poe` ; carte POE choisie dans l'IDE |
| `broker injoignable` | pas sur le même réseau que le broker, ou port 8083 filtré | tester depuis MQTTX |
| `connexion refusee par le broker` | EMQX refuse ce username | règles d'authentification EMQX |
| connecté mais rien n'arrive | `update()` absent de `loop()`, mode `Event` sans changement, topic différent | `trigger()` pour forcer ; comparer les topics lettre pour lettre |
| `infos` ou `ping` n'arrivent pas, les données oui | EMQX refuse `saillance/sys/…` en publication ou en abonnement | règle d'autorisation EMQX : `saillance/sys/${clientid}/#` en publication **et** abonnement |
| `lat` reste à -1 | le ping ne revient pas | même cause : abonnement à `…/ping` refusé |
| texte ou table reçus mais rien ne s'affiche | rappel qui ne lit que `m.count` | tester `m.isText` et `m.isTable` |
| `message illisible sur …` | JSON invalide ou `f` incorrect | voir `references/messages.md` |
| caractères illisibles sur Serial | mauvaise vitesse | 115200 |
| données qui arrivent avec 30 s de retard | (ancienne version) envoi par `esp_mqtt_client_enqueue` | utiliser la version actuelle |

Un `ts` très petit (quelques milliers) veut dire que le NTP n'est pas synchronisé : normal sans accès à
Internet ; `setNtpServer("adresse")` si le réseau a son propre serveur de temps.
