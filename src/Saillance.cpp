#include "Saillance.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdarg.h>
#include <sys/time.h>

SaillanceClass Saillance;

namespace saillance {

// ============================================================================
//  Poignées (FloatOut / TextOut / Input)
// ============================================================================
static void warnInvalidHandle(const char* what) {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (last == 0 || now - last > 5000) {
    last = now ? now : 1;
    Serial.printf("[Saillance] %s non valide : la creation a echoue (voir message plus haut)\n", what);
  }
}

void FloatOut::set(const char* key, float value) {
  if (!ch_) return warnInvalidHandle("FloatOut");
  ch_->set(key, value);
}
void FloatOut::trigger() {
  if (!ch_) return warnInvalidHandle("FloatOut");
  ch_->dirty = true;
}

void TextOut::set(const char* text) {
  if (!ch_) return warnInvalidHandle("TextOut");
  ch_->set(text);
}
void TextOut::trigger() {
  if (!ch_) return warnInvalidHandle("TextOut");
  ch_->dirty = true;
}

static const Message kEmptyMessage;

bool Input::hasData() const { return ch_ && ch_->hasData; }
bool Input::isNew() {
  if (!ch_ || !ch_->fresh) return false;
  ch_->fresh = false;
  return true;
}
float Input::get(const char* key, float fallback) const { return ch_ ? ch_->last.get(key, fallback) : fallback; }
float Input::value(uint8_t i) const { return ch_ ? ch_->last.value(i) : 0.0f; }
bool Input::isText() const { return ch_ && ch_->last.isText; }
const char* Input::text() const { return ch_ ? ch_->last.text : ""; }
const Message& Input::message() const { return ch_ ? ch_->last : kEmptyMessage; }
void Input::onMessage(MessageCallback cb) {
  if (!ch_) return warnInvalidHandle("Input");
  ch_->callback = cb;
}

// ============================================================================
//  SaillanceClass : démarrage et API
// ============================================================================
SaillanceClass::SaillanceClass() { setIssueHook(&SaillanceClass::onIssue_); }

void SaillanceClass::say(const char* fmt, ...) {
  if (!debug_) return;
  char line[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);
  Serial.print("[Saillance] ");
  Serial.println(line);
}

void SaillanceClass::setMqttPassword(const char* password) {
  strncpy(mqttPass_, password ? password : "", sizeof(mqttPass_) - 1);
}
void SaillanceClass::setNtpServer(const char* server) {
  if (server && server[0]) strncpy(ntpServer_, server, sizeof(ntpServer_) - 1);
}
void SaillanceClass::setDebug(bool on) { debug_ = on; }

bool SaillanceClass::beginWifi(Board board, const char* deviceName, const char* ssid, const char* password) {
  if (!ssid || !ssid[0]) {
    reportIssue(Severity::Error, ErrorCode::ApiBadConfig, "begin", "le nom du reseau WiFi (ssid) est vide");
    return false;
  }
  strncpy(ssid_, ssid, sizeof(ssid_) - 1);
  strncpy(wifiPass_, password ? password : "", sizeof(wifiPass_) - 1);
  return begin_(board, deviceName, Link::Wifi);
}

bool SaillanceClass::beginEthernet(Board board, const char* deviceName) {
  if (!boardHasEthernet(board)) {
    reportIssue(Severity::Error, ErrorCode::ApiBadConfig, "begin",
                "cette carte n'a pas d'Ethernet : utilisez beginWifi() ou Board::Esp32Poe / Esp32PoeIso");
    return false;
  }
  return begin_(board, deviceName, Link::Ethernet);
}

bool SaillanceClass::begin_(Board board, const char* deviceName, Link link) {
  if (started_) {
    reportIssue(Severity::Warning, ErrorCode::ApiBadConfig, "begin", "begin deja appele, ignore");
    return false;
  }
  if (!buildUsername(board, deviceName, username_, sizeof(username_))) return false;
  buildSysTopic(username_, "error", errorTopic_, sizeof(errorTopic_));
  buildSysTopic(username_, "infos", infosTopic_, sizeof(infosTopic_));

  // Nom réseau : le "_" n'est pas permis dans un hostname.
  strncpy(hostname_, username_, sizeof(hostname_) - 1);
  for (char* c = hostname_; *c; c++)
    if (*c == '_') *c = '-';
  snprintf(uri_, sizeof(uri_), "ws://%s:%d%s", SAILLANCE_BROKER_HOST, SAILLANCE_BROKER_PORT, SAILLANCE_BROKER_PATH);

  board_ = board;
  link_ = link;
  incomingQueue_ = xQueueCreate(8, sizeof(Incoming));
  errorQueue_ = xQueueCreate(10, sizeof(ErrorItem));

  startSystem_();  // capture des erreurs, mesure du CPU, cause du dernier redémarrage
  say("appareil : %s", username_);
  say("topic erreurs : %s", errorTopic_);
  say("topic infos   : %s (toutes les %d s)", infosTopic_, SAILLANCE_INFOS_PERIOD_MS / 1000);
  if (!startNetwork_()) return false;

  started_ = true;
  lastInfosMs_ = millis();
  return true;
}

uint64_t SaillanceClass::timestampMs() const {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  if (tv.tv_sec > 1700000000L)  // l'heure a été synchronisée (NTP)
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec / 1000);
  return (uint64_t)millis();  // sinon : millisecondes depuis le démarrage
}

FloatOut SaillanceClass::floatOut(const Topic& topic, std::initializer_list<const char*> keys, uint8_t maxHz,
                                  SendMode mode) {
  if (!topic.valid()) {
    reportIssue(Severity::Error, ErrorCode::ApiBadTopic, "floatOut", "topic invalide (voir erreur plus haut)");
    return FloatOut();
  }
  if (keys.size() == 0) {
    reportIssue(Severity::Error, ErrorCode::ApiBadKey, "floatOut", "il faut au moins une cle, ex: {\"r\", \"g\"}");
    return FloatOut();
  }
  FloatChannel* ch = new FloatChannel();
  ch->configure(topic, maxHz, mode);
  for (const char* k : keys) {
    if (!ch->addKey(k)) {
      delete ch;
      return FloatOut();
    }
  }
  floatOuts_.push_back(ch);
  char keyList[SAILLANCE_MAX_KEYS * (SAILLANCE_KEY_MAX + 1) + 1] = {0};
  for (uint8_t i = 0; i < ch->count; i++) {
    if (i) strcat(keyList, ",");
    strcat(keyList, ch->keys[i]);
  }
  say("envoi    -> %s  [%s]  max %u Hz, %s", topic.c_str(), keyList, (unsigned)ch->maxHz,
      mode == SendMode::Continuous ? "continu" : "sur evenement");
  return FloatOut(ch);
}

TextOut SaillanceClass::textOut(const Topic& topic, uint8_t maxHz, SendMode mode) {
  if (!topic.valid()) {
    reportIssue(Severity::Error, ErrorCode::ApiBadTopic, "textOut", "topic invalide (voir erreur plus haut)");
    return TextOut();
  }
  TextChannel* ch = new TextChannel();
  ch->configure(topic, maxHz, mode);
  textOuts_.push_back(ch);
  say("envoi    -> %s  [texte]  max %u Hz, %s", topic.c_str(), (unsigned)ch->maxHz,
      mode == SendMode::Continuous ? "continu" : "sur evenement");
  return TextOut(ch);
}

Input SaillanceClass::input(const Topic& topic) {
  if (!topic.valid()) {
    reportIssue(Severity::Error, ErrorCode::ApiBadTopic, "input", "topic invalide (voir erreur plus haut)");
    return Input();
  }
  InputChannel* ch = new InputChannel();
  ch->topic = topic;
  inputs_.push_back(ch);
  say("reception <- %s", topic.c_str());
  return Input(ch);
}

Input SaillanceClass::input(const Topic& topic, MessageCallback cb) {
  Input in = input(topic);
  in.onMessage(cb);
  return in;
}

// ============================================================================
//  update() : appelée à chaque tour de loop()
// ============================================================================
void SaillanceClass::update() {
  if (!started_) return;
  uint32_t now = millis();
  frames_++;
  syncMqtt_();
  if (mqttConnected_) subscribeAll_();
  dispatchIncoming_();
  publishOutputs_(now);
  publishErrors_();
  publishInfos_(now);
}

void SaillanceClass::publishOutputs_(uint32_t now) {
  const bool online = mqttConnected_;

  for (FloatChannel* ch : floatOuts_) {
    if (!ch->due(now)) continue;
    if (!online) {
      if (ch->mode == SendMode::Continuous) {  // occasion manquée
        ch->markSent(now);
        droppedFrames_++;
      }
      continue;  // en mode Event on garde la valeur pour la reconnexion
    }
    size_t n = ch->build(payload_, sizeof(payload_), timestampMs());
    if (n == 0 || !publish_(ch->topic.c_str(), payload_, n, 0)) droppedFrames_++;
    ch->markSent(now);
  }

  for (TextChannel* ch : textOuts_) {
    if (!ch->due(now)) continue;
    if (!online) {
      if (ch->mode == SendMode::Continuous) {
        ch->markSent(now);
        droppedFrames_++;
      }
      continue;
    }
    size_t n = ch->build(payload_, sizeof(payload_), timestampMs());
    if (n == 0 || !publish_(ch->topic.c_str(), payload_, n, 0)) droppedFrames_++;
    ch->markSent(now);
  }
}

}  // namespace saillance
