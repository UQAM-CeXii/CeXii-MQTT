#pragma once
// ============================================================================
//  SaillanceCore : logique pure (sans Arduino) -> testable sur l'ordinateur.
//  Validation, construction des topics, JSON, cadence d'envoi.
//  L'utilisateur n'a normalement pas besoin de lire ce fichier.
// ============================================================================
#include <stddef.h>
#include <stdint.h>

#include <functional>

#include "SaillanceConfig.h"
#include "SaillanceTypes.h"

namespace saillance {

// --- Signalement des problèmes ----------------------------------------------
// La classe Saillance branche ce hook : message à l'écran (Serial) + topic /error.
typedef void (*IssueHook)(Severity sv, ErrorCode code, const char* src, const char* msg);
void setIssueHook(IssueHook hook);
void reportIssue(Severity sv, ErrorCode code, const char* src, const char* fmt, ...)
    __attribute__((format(printf, 4, 5)));

// --- Validation -------------------------------------------------------------
enum class Check : uint8_t { Ok, Null, Empty, TooLong, BadChar };

Check checkDeviceName(const char* s);  // lettres + chiffres, 1..24
Check checkSegment(const char* s);     // lettres, chiffres, _ -, 1..12
Check checkKey(const char* s);         // lettres, chiffres, _, 1..6
const char* checkText(Check c);        // explication en français

// --- Noms et topics ---------------------------------------------------------
const char* boardPrefix(Board b);  // "esp32", "esp32poe", "esp32poeiso"
bool boardHasEthernet(Board b);
const char* zoneName(Zone z);

// "esp32poeiso_controlbouton" (nom mis en minuscules). Retourne false si invalide.
bool buildUsername(Board b, const char* deviceName, char* out, size_t cap);
// "/saillance/sys/<username>/<leaf>"
bool buildSysTopic(const char* username, const char* leaf, char* out, size_t cap);

// Un topic complet et validé : saillance/zone/module/composante/propriete
class Topic {
 public:
  Topic() : valid_(false) { path_[0] = '\0'; }
  bool valid() const { return valid_; }
  explicit operator bool() const { return valid_; }
  const char* c_str() const { return path_; }

 private:
  friend Topic makeTopic(Zone, const char*, const char*, const char*);
  char path_[SAILLANCE_TOPIC_MAX + 1];
  bool valid_;
};

Topic makeTopic(Zone zone, const char* module, const char* component, const char* property);

// Construction guidée, étape par étape (l'éditeur propose la suite à chaque point) :
//   Saillance.zone(Zone::MusiqueJam).module("lumiere").component("bande1").property("couleur")
class TopicNeedsProperty {
 public:
  Topic property(const char* name) const;

 private:
  friend class TopicNeedsComponent;
  Zone zone_ = Zone::Count_;
  char module_[SAILLANCE_SEGMENT_MAX + 1] = {0};
  char component_[SAILLANCE_SEGMENT_MAX + 1] = {0};
  bool ok_ = false;
};

class TopicNeedsComponent {
 public:
  TopicNeedsProperty component(const char* name) const;

 private:
  friend class TopicNeedsModule;
  Zone zone_ = Zone::Count_;
  char module_[SAILLANCE_SEGMENT_MAX + 1] = {0};
  bool ok_ = false;
};

class TopicNeedsModule {
 public:
  explicit TopicNeedsModule(Zone zone);
  TopicNeedsComponent module(const char* name) const;

 private:
  Zone zone_;
  bool ok_;
};

// --- Écriture JSON minimale -------------------------------------------------
class JsonWriter {
 public:
  JsonWriter(char* buf, size_t cap) : b_(buf), cap_(cap) { if (cap_) b_[0] = '\0'; }
  void beginObject() { sep(); put('{'); comma_ = false; }
  void endObject() { put('}'); comma_ = true; }
  void beginArray() { sep(); put('['); comma_ = false; }
  void endArray() { put(']'); comma_ = true; }
  void key(const char* k);
  void str(const char* s);
  void u64(uint64_t v);
  void i32(int32_t v);
  void num(float v);
  // Retourne la longueur écrite, ou 0 si le buffer était trop petit.
  size_t finish() const { return overflow_ ? 0 : len_; }

 private:
  void sep() { if (comma_) put(','); }
  void put(char c);
  void raw(const char* s);
  char* b_;
  size_t cap_, len_ = 0;
  bool comma_ = false, overflow_ = false;
};

// --- Messages reçus ---------------------------------------------------------
struct Message {
  uint8_t version = 0;
  uint64_t ts = 0;
  uint32_t seq = 0;
  bool isText = false;
  char text[SAILLANCE_TEXT_MAX + 1] = {0};
  uint8_t count = 0;  // nb de valeurs (si float)
  char keys[SAILLANCE_MAX_KEYS][SAILLANCE_KEY_MAX + 1] = {{0}};
  float values[SAILLANCE_MAX_KEYS] = {0};

  bool has(const char* key) const;
  float get(const char* key, float fallback = 0.0f) const;
  float value(uint8_t i) const { return i < count ? values[i] : 0.0f; }
  const char* key(uint8_t i) const { return i < count ? keys[i] : ""; }
};

// Décode {"v":..,"ts":..,"seq":..,"val":..,"f":"float|string","label":[..]}
bool parseMessage(const char* payload, size_t len, Message& out);

typedef std::function<void(const Message&)> MessageCallback;

// --- Canaux d'envoi ---------------------------------------------------------
struct OutChannel {
  Topic topic;
  SendMode mode = SendMode::Event;
  uint32_t intervalMs = 34;
  uint8_t maxHz = 30;
  uint32_t lastSentMs = 0;
  uint32_t seq = 0;
  bool sentOnce = false;
  bool dirty = false;

  void configure(const Topic& t, uint8_t maxHz, SendMode m);
  bool due(uint32_t nowMs) const;
  void markSent(uint32_t nowMs);  // appelé après un envoi (ou une occasion manquée)
};

struct FloatChannel : OutChannel {
  uint8_t count = 0;
  char keys[SAILLANCE_MAX_KEYS][SAILLANCE_KEY_MAX + 1] = {{0}};
  float values[SAILLANCE_MAX_KEYS] = {0};
  bool warnedKey = false, warnedRange = false;

  bool addKey(const char* key);
  void set(const char* key, float v);
  size_t build(char* buf, size_t cap, uint64_t ts);  // incrémente seq
};

struct TextChannel : OutChannel {
  char text[SAILLANCE_TEXT_MAX + 1] = {0};
  bool warnedLen = false;

  void set(const char* s);
  size_t build(char* buf, size_t cap, uint64_t ts);
};

// --- Canal de réception -----------------------------------------------------
struct InputChannel {
  Topic topic;
  Message last;
  bool hasData = false;
  bool fresh = false;
  bool subscribed = false;
  MessageCallback callback;

  void deliver(const Message& m);
};

// --- Messages système -------------------------------------------------------
size_t buildErrorPayload(char* buf, size_t cap, uint64_t ts, uint32_t seq, const char* src,
                         const char* msg, Severity sv, ErrorCode code);
size_t buildInfosPayload(char* buf, size_t cap, uint64_t ts, uint32_t seq, uint32_t fps,
                         uint32_t droppedFrames, uint32_t gpu, uint32_t cpu);

}  // namespace saillance
