#pragma once
// ============================================================================
//  Saillance : envoyer et recevoir des messages MQTT (ESP32 / ESP32 PoE)
//
//  Utilisation minimale :
//
//    #include <Saillance.h>
//
//    FloatOut lumiere;
//
//    void setup() {
//      Serial.begin(115200);
//      Saillance.beginWifi(Board::Esp32, "controlbouton", "MonWifi", "MotDePasse");
//      Topic t = Saillance.topic(Zone::Salon, "lumiere", "bande1", "couleur");
//      lumiere = Saillance.floatOut(t, {"r", "g", "b"});
//    }
//
//    void loop() {
//      Saillance.update();              // OBLIGATOIRE, à chaque tour de loop()
//      lumiere.set("r", 0.5);           // valeurs entre 0 et 1
//    }
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>

#include <initializer_list>
#include <vector>

#include "SaillanceConfig.h"
#include "SaillanceCore.h"
#include "SaillanceTypes.h"

typedef struct esp_mqtt_client* esp_mqtt_client_handle_t;

namespace saillance {

// ---------------------------------------------------------------------------
//  Sortie de nombres : une liste de valeurs (0..1), chacune avec sa clé.
// ---------------------------------------------------------------------------
class FloatOut {
 public:
  FloatOut() {}
  // Change la valeur associée à une clé (0.0 à 1.0). Envoi selon le mode choisi.
  void set(const char* key, float value);
  void set(const String& key, float value) { set(key.c_str(), value); }
  // Force un envoi même si rien n'a changé (utile en mode Event).
  void trigger();
  bool valid() const { return ch_ != nullptr; }

 private:
  friend class SaillanceClass;
  explicit FloatOut(FloatChannel* ch) : ch_(ch) {}
  FloatChannel* ch_ = nullptr;
};

// ---------------------------------------------------------------------------
//  Sortie de texte.
// ---------------------------------------------------------------------------
class TextOut {
 public:
  TextOut() {}
  void set(const char* text);
  void set(const String& text) { set(text.c_str()); }
  void trigger();
  bool valid() const { return ch_ != nullptr; }

 private:
  friend class SaillanceClass;
  explicit TextOut(TextChannel* ch) : ch_(ch) {}
  TextChannel* ch_ = nullptr;
};

// ---------------------------------------------------------------------------
//  Réception : soit on lit la dernière valeur, soit on est appelé à chaque message.
// ---------------------------------------------------------------------------
class Input {
 public:
  Input() {}
  bool valid() const { return ch_ != nullptr; }
  bool hasData() const;                 // au moins un message reçu ?
  bool isNew();                         // un nouveau message depuis le dernier appel ?
  float get(const char* key, float fallback = 0.0f) const;  // valeur d'une clé
  float get(const String& key, float fallback = 0.0f) const { return get(key.c_str(), fallback); }
  float value(uint8_t index) const;     // valeur par position
  bool isText() const;
  const char* text() const;             // dernier texte reçu
  const Message& message() const;       // tout le message
  void onMessage(MessageCallback cb);   // appelé dans loop() via update()

 private:
  friend class SaillanceClass;
  explicit Input(InputChannel* ch) : ch_(ch) {}
  InputChannel* ch_ = nullptr;
};

// ---------------------------------------------------------------------------
//  Objet principal : utiliser l'instance globale "Saillance".
// ---------------------------------------------------------------------------
class SaillanceClass {
 public:
  SaillanceClass();

  // --- 1. Démarrage ---------------------------------------------------------
  // deviceName : lettres et chiffres seulement, 24 caractères max.
  // Le username MQTT sera "<carte>_<nom>", ex: "esp32poeiso_controlbouton".
  bool beginWifi(Board board, const char* deviceName, const char* ssid, const char* password);
  bool beginEthernet(Board board, const char* deviceName);

  // Réglages optionnels (à appeler AVANT begin) :
  void setMqttPassword(const char* password);
  void setNtpServer(const char* server);   // pour l'horodatage (défaut pool.ntp.org)
  void setDebug(bool on);                  // messages d'état sur Serial (défaut: oui)

  // --- 2. À appeler dans loop() ----------------------------------------------
  void update();

  // --- 3. Construire un topic (guidé) ---------------------------------------
  //   Saillance.zone(Zone::Salon).module("lumiere").component("bande1").property("couleur")
  TopicNeedsModule zone(Zone z) { return TopicNeedsModule(z); }
  //   ou en une ligne :
  Topic topic(Zone z, const char* module, const char* component, const char* property) {
    return makeTopic(z, module, component, property);
  }

  // --- 4. Envoyer -----------------------------------------------------------
  // keys : les clés des valeurs envoyées sur ce topic (6 caractères max chacune)
  // maxHz : fréquence d'envoi maximale, de 1 à 30
  FloatOut floatOut(const Topic& topic, std::initializer_list<const char*> keys, uint8_t maxHz = 30,
                    SendMode mode = SendMode::Event);
  TextOut textOut(const Topic& topic, uint8_t maxHz = 30, SendMode mode = SendMode::Event);

  // --- 5. Recevoir ----------------------------------------------------------
  Input input(const Topic& topic);
  Input input(const Topic& topic, MessageCallback cb);

  // --- Outils ---------------------------------------------------------------
  bool connected() const { return mqttConnected_; }          // connecté au broker ?
  const char* username() const { return username_; }
  // Envoyer ses propres erreurs vers /saillance/sys/<username>/error
  void reportError(const char* source, const char* message, Severity sv = Severity::Error,
                   ErrorCode code = ErrorCode::User);

 private:
  enum class Link : uint8_t { None, Wifi, Ethernet };
  struct ErrorItem {
    uint64_t ts;
    Severity sv;
    ErrorCode code;
    char src[16];
    char msg[100];
  };
  struct Incoming {
    uint16_t len;
    char topic[SAILLANCE_TOPIC_MAX + 1];
    char payload[SAILLANCE_PAYLOAD_MAX + 1];
  };

  bool begin_(Board board, const char* deviceName, Link link);
  uint64_t timestampMs() const;
  void say(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

  // réseau (SaillanceNet.cpp)
  bool startNetwork_();
  static void onNetEvent_(arduino_event_id_t event, arduino_event_info_t info);

  // MQTT (SaillanceMqtt.cpp)
  bool startMqtt_();
  void stopMqtt_();
  bool publish_(const char* topic, const char* data, size_t len, int qos);
  void syncMqtt_();
  void subscribeAll_();
  void dispatchIncoming_();
  static void onMqttEvent_(void* arg, const char* base, int32_t id, void* data);

  // système (SaillanceSystem.cpp)
  void startSystem_();
  void publishErrors_();
  void publishInfos_(uint32_t now);
  void publishOutputs_(uint32_t now);
  float sampleCpu_();
  static int logHook_(const char* fmt, va_list args);
  static void onIssue_(Severity sv, ErrorCode code, const char* src, const char* msg);

  // état
  Link link_ = Link::None;
  Board board_ = Board::Esp32;
  bool started_ = false;
  bool debug_ = true;
  char username_[SAILLANCE_NAME_MAX + 16] = {0};
  char hostname_[SAILLANCE_NAME_MAX + 16] = {0};
  char ssid_[33] = {0};
  char wifiPass_[65] = {0};
  char mqttPass_[65] = {0};
  char ntpServer_[48] = "pool.ntp.org";
  char uri_[64] = {0};
  char errorTopic_[SAILLANCE_TOPIC_MAX + 1] = {0};
  char infosTopic_[SAILLANCE_TOPIC_MAX + 1] = {0};
  char payload_[SAILLANCE_PAYLOAD_MAX + 1] = {0};

  esp_mqtt_client_handle_t client_ = nullptr;
  volatile bool netUp_ = false;
  volatile bool mqttConnected_ = false;
  bool mqttStarted_ = false;
  bool timeStarted_ = false;

  std::vector<FloatChannel*> floatOuts_;
  std::vector<TextChannel*> textOuts_;
  std::vector<InputChannel*> inputs_;

  void* incomingQueue_ = nullptr;  // QueueHandle_t
  void* errorQueue_ = nullptr;     // QueueHandle_t

  // statistiques /infos
  uint32_t errorSeq_ = 0, infosSeq_ = 0;
  uint32_t frames_ = 0, droppedFrames_ = 0;
  uint32_t lastInfosMs_ = 0;
  bool infosDue_ = false;
  uint32_t idleLast_[2] = {0, 0};
  float idleMaxRate_[2] = {0, 0};
  uint32_t lastCpuMs_ = 0;
  // anti-répétition des erreurs
  uint32_t lastErrHash_ = 0, lastErrMs_ = 0;
  uint32_t lowHeapMs_ = 0;

  friend class FloatOut;
  friend class TextOut;
};

}  // namespace saillance

extern saillance::SaillanceClass Saillance;

// Pour ne pas avoir à écrire "saillance::" partout (désactiver avec
// -DSAILLANCE_NO_USING dans platformio.ini si ça entre en conflit avec votre code).
#ifndef SAILLANCE_NO_USING
using namespace saillance;
#endif
