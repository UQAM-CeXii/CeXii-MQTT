#pragma once
// ============================================================================
//  CexiiMqttConfig.h : réglages de la bibliothèque.
//  C'est ICI qu'on change l'adresse du broker, la liste des zones, les limites.
// ============================================================================

// --- Broker EMQX (MQTT sur WebSocket) ---------------------------------------
#define CEXII_BROKER_HOST "10.0.21.210"
#define CEXII_BROKER_PORT 8083
#define CEXII_BROKER_PATH "/mqtt"  // chemin WebSocket par défaut d'EMQX

// --- Topics -----------------------------------------------------------------
#define CEXII_PROJECT "saillance"
// 1 => "/saillance/sys/..."   0 => "saillance/sys/..."
#define CEXII_TOPIC_LEADING_SLASH 0

// --- Zones (liste des choix pour le 2e niveau du topic) ---------------------
// À MODIFIER : une ligne par zone  X(NomDansLeCode, "texte-dans-le-topic")
// Le texte doit faire 12 caractères max (lettres, chiffres, _ ou -).
#define CEXII_ZONES(X)         \
  X(Jam, "jam")                    \
  X(Ecrans, "ecrans")              \
  X(Globale, "globale")            \
  X(Regie, "regie")

// --- Limites ----------------------------------------------------------------
#define CEXII_NAME_MAX 24      // nom d'appareil choisi par l'utilisateur
#define CEXII_SEGMENT_MAX 12   // module / composante / propriété
#define CEXII_KEY_MAX 6        // clé associée à une valeur
#define CEXII_MAX_KEYS 16      // nb de valeurs par topic
#define CEXII_MIN_HZ 1
#define CEXII_MAX_HZ 30
#define CEXII_TEXT_MAX 128     // longueur max d'un message texte
#define CEXII_TABLE_MAX_COLS 16    // colonnes d'une table
#define CEXII_TABLE_MAX_ROWS 64    // rangées d'une table
#define CEXII_TABLE_MAX_BYTES 8192 // taille max d'un message table (JSON complet)
#define CEXII_TOPIC_MAX 96
#define CEXII_PAYLOAD_MAX 512  // taille max d'un message JSON

// --- Messages système automatiques ------------------------------------------
#define CEXII_FORMAT_VERSION 1       // champ "v"
#define CEXII_INFOS_PERIOD_MS 5000   // période du topic /infos
#define CEXII_PING_PERIOD_MS 60000   // période du topic /ping (mesure de latence)
#define CEXII_LOW_HEAP_BYTES 20000   // seuil d'alerte mémoire
