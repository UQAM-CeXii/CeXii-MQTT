#pragma once
#include <stdint.h>
#include "SaillanceConfig.h"

namespace saillance {

// Type de carte : sert de préfixe au username EMQX (ex: "esp32poeiso_monnom").
enum class Board : uint8_t {
  Esp32,        // ESP32 classique (WiFi seulement)
  Esp32Poe,     // Olimex ESP32-POE (WiFi + Ethernet)
  Esp32PoeIso,  // Olimex ESP32-POE-ISO (WiFi + Ethernet)
};

// Quand une sortie envoie-t-elle ses données ?
enum class SendMode : uint8_t {
  Event,       // seulement quand une valeur change (ou trigger())
  Continuous,  // en continu, à la fréquence demandée
};

enum class Severity : uint8_t { Info = 0, Warning = 1, Error = 2, Critical = 3 };

// Codes d'erreur (champ "t" du topic /error).
enum class ErrorCode : uint16_t {
  SysLog = 100,         // erreur capturée dans les logs (ESP_LOGE / log_e)
  SysReset = 101,       // redémarrage anormal (panic, watchdog, brownout)
  SysLowHeap = 102,     // mémoire presque épuisée
  NetLinkLost = 200,    // lien WiFi / Ethernet perdu
  NetFailed = 201,      // impossible de démarrer le réseau
  MqttRefused = 300,    // broker a refusé la connexion (identifiants ?)
  MqttError = 301,
  ApiBadName = 400,     // nom d'appareil invalide
  ApiBadTopic = 401,    // segment de topic invalide
  ApiBadKey = 402,      // clé invalide ou inconnue
  ApiBadValue = 403,    // valeur hors de 0..1, NaN, texte trop long
  ApiBadConfig = 404,   // fréquence hors limites, mauvaise carte, etc.
  ApiBadMessage = 405,  // message reçu illisible
  User = 900,           // erreur envoyée par le programme de l'utilisateur
};

// Zones : générées à partir de SAILLANCE_ZONES (SaillanceConfig.h).
enum class Zone : uint8_t {
#define SAILLANCE_X(id, text) id,
  SAILLANCE_ZONES(SAILLANCE_X)
#undef SAILLANCE_X
      Count_
};

}  // namespace saillance
