#pragma once
// ============================================================================
//  SaillanceConfig.h : réglages de la bibliothèque.
//  C'est ICI qu'on change l'adresse du broker, la liste des zones, les limites.
// ============================================================================

// --- Broker EMQX (MQTT sur WebSocket) ---------------------------------------
#define SAILLANCE_BROKER_HOST "10.0.21.210"
#define SAILLANCE_BROKER_PORT 8083
#define SAILLANCE_BROKER_PATH "/mqtt"  // chemin WebSocket par défaut d'EMQX

// --- Topics -----------------------------------------------------------------
#define SAILLANCE_PROJECT "saillance"
// 1 => "/saillance/sys/..."   0 => "saillance/sys/..."
#define SAILLANCE_TOPIC_LEADING_SLASH 0

// --- Zones (liste des choix pour le 2e niveau du topic) ---------------------
// À MODIFIER : une ligne par zone  X(NomDansLeCode, "texte-dans-le-topic")
// Le texte doit faire 12 caractères max (lettres, chiffres, _ ou -).
#define SAILLANCE_ZONES(X)         \
  X(MusiqueJam, "musiquejam")      \
  X(Screen, "screen")

// --- Limites ----------------------------------------------------------------
#define SAILLANCE_NAME_MAX 24      // nom d'appareil choisi par l'utilisateur
#define SAILLANCE_SEGMENT_MAX 12   // module / composante / propriété
#define SAILLANCE_KEY_MAX 6        // clé associée à une valeur
#define SAILLANCE_MAX_KEYS 16      // nb de valeurs par topic
#define SAILLANCE_MIN_HZ 1
#define SAILLANCE_MAX_HZ 30
#define SAILLANCE_TEXT_MAX 128     // longueur max d'un message texte
#define SAILLANCE_TOPIC_MAX 96
#define SAILLANCE_PAYLOAD_MAX 512  // taille max d'un message JSON

// --- Messages système automatiques ------------------------------------------
#define SAILLANCE_FORMAT_VERSION 1       // champ "v"
#define SAILLANCE_INFOS_PERIOD_MS 5000   // période du topic /infos
#define SAILLANCE_LOW_HEAP_BYTES 20000   // seuil d'alerte mémoire
