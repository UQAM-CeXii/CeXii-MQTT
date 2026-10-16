#include "SaillanceCore.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace saillance {

// ============================================================================
//  Signalement
// ============================================================================
static IssueHook s_hook = nullptr;

void setIssueHook(IssueHook hook) { s_hook = hook; }

void reportIssue(Severity sv, ErrorCode code, const char* src, const char* fmt, ...) {
  if (!s_hook) return;
  char msg[112];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);
  s_hook(sv, code, src, msg);
}

// ============================================================================
//  Validation
// ============================================================================
static Check checkChars(const char* s, size_t maxLen, bool allowUnderscore, bool allowDash) {
  if (!s) return Check::Null;
  size_t n = strlen(s);
  if (n == 0) return Check::Empty;
  if (n > maxLen) return Check::TooLong;
  for (size_t i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    bool ok = (c < 128 && isalnum(c)) || (allowUnderscore && c == '_') || (allowDash && c == '-');
    if (!ok) return Check::BadChar;
  }
  return Check::Ok;
}

Check checkDeviceName(const char* s) { return checkChars(s, SAILLANCE_NAME_MAX, false, false); }
Check checkSegment(const char* s) { return checkChars(s, SAILLANCE_SEGMENT_MAX, true, true); }
Check checkKey(const char* s) { return checkChars(s, SAILLANCE_KEY_MAX, true, false); }

const char* checkText(Check c) {
  switch (c) {
    case Check::Ok: return "ok";
    case Check::Null: return "valeur manquante";
    case Check::Empty: return "ne peut pas etre vide";
    case Check::TooLong: return "trop long";
    case Check::BadChar: return "caractere interdit (pas d'espace, d'accent ni de symbole)";
  }
  return "?";
}

// ============================================================================
//  Noms et topics
// ============================================================================
const char* boardPrefix(Board b) {
  switch (b) {
    case Board::Esp32: return "esp32";
    case Board::Esp32Poe: return "esp32poe";
    case Board::Esp32PoeIso: return "esp32poeiso";
  }
  return "esp32";
}

bool boardHasEthernet(Board b) { return b != Board::Esp32; }

const char* zoneName(Zone z) {
  static const char* const names[] = {
#define SAILLANCE_X(id, text) text,
      SAILLANCE_ZONES(SAILLANCE_X)
#undef SAILLANCE_X
  };
  return (uint8_t)z < (uint8_t)Zone::Count_ ? names[(uint8_t)z] : "";
}

static void lowerCopy(char* dst, const char* src, size_t cap) {
  size_t i = 0;
  for (; src[i] && i + 1 < cap; i++) dst[i] = (char)tolower((unsigned char)src[i]);
  dst[i] = '\0';
}

bool buildUsername(Board b, const char* deviceName, char* out, size_t cap) {
  Check c = checkDeviceName(deviceName);
  if (c != Check::Ok) {
    reportIssue(Severity::Error, ErrorCode::ApiBadName, "begin",
                "nom d'appareil \"%s\" invalide : %s (max %d, lettres et chiffres seulement)",
                deviceName ? deviceName : "", checkText(c), SAILLANCE_NAME_MAX);
    return false;
  }
  char lower[SAILLANCE_NAME_MAX + 1];
  lowerCopy(lower, deviceName, sizeof(lower));
  int n = snprintf(out, cap, "%s_%s", boardPrefix(b), lower);
  return n > 0 && (size_t)n < cap;
}

static const char* rootPrefix() { return SAILLANCE_TOPIC_LEADING_SLASH ? "/" SAILLANCE_PROJECT : SAILLANCE_PROJECT; }

bool buildSysTopic(const char* username, const char* leaf, char* out, size_t cap) {
  int n = snprintf(out, cap, "%s/sys/%s/%s", rootPrefix(), username, leaf);
  return n > 0 && (size_t)n < cap;
}

// Valide un segment ; copie en minuscules dans dst. Signale l'erreur sinon.
static bool takeSegment(const char* what, const char* in, char* dst) {
  Check c = checkSegment(in);
  if (c != Check::Ok) {
    reportIssue(Severity::Error, ErrorCode::ApiBadTopic, "topic",
                "%s \"%s\" invalide : %s (max %d, lettres/chiffres/_/-)", what, in ? in : "",
                checkText(c), SAILLANCE_SEGMENT_MAX);
    return false;
  }
  lowerCopy(dst, in, SAILLANCE_SEGMENT_MAX + 1);
  return true;
}

static bool zoneOk(Zone z) {
  if ((uint8_t)z >= (uint8_t)Zone::Count_) {
    reportIssue(Severity::Error, ErrorCode::ApiBadTopic, "topic", "zone inconnue");
    return false;
  }
  return true;
}

Topic makeTopic(Zone zone, const char* module, const char* component, const char* property) {
  Topic t;
  char m[SAILLANCE_SEGMENT_MAX + 1], c[SAILLANCE_SEGMENT_MAX + 1], p[SAILLANCE_SEGMENT_MAX + 1];
  if (!zoneOk(zone) || !takeSegment("module", module, m) || !takeSegment("composante", component, c) ||
      !takeSegment("propriete", property, p))
    return t;
  int n = snprintf(t.path_, sizeof(t.path_), "%s/%s/%s/%s/%s", rootPrefix(), zoneName(zone), m, c, p);
  t.valid_ = n > 0 && (size_t)n < sizeof(t.path_);
  return t;
}

TopicNeedsModule::TopicNeedsModule(Zone zone) : zone_(zone), ok_(zoneOk(zone)) {}

TopicNeedsComponent TopicNeedsModule::module(const char* name) const {
  TopicNeedsComponent next;
  next.zone_ = zone_;
  next.ok_ = ok_ && takeSegment("module", name, next.module_);
  return next;
}

TopicNeedsProperty TopicNeedsComponent::component(const char* name) const {
  TopicNeedsProperty next;
  next.zone_ = zone_;
  memcpy(next.module_, module_, sizeof(module_));
  next.ok_ = ok_ && takeSegment("composante", name, next.component_);
  return next;
}

Topic TopicNeedsProperty::property(const char* name) const {
  if (!ok_) return Topic();
  return makeTopic(zone_, module_, component_, name);
}

// ============================================================================
//  JsonWriter
// ============================================================================
void JsonWriter::put(char c) {
  if (len_ + 1 < cap_) {
    b_[len_++] = c;
    b_[len_] = '\0';
  } else {
    overflow_ = true;
  }
}

void JsonWriter::raw(const char* s) {
  while (*s) put(*s++);
}

void JsonWriter::key(const char* k) {
  sep();
  put('"');
  raw(k);
  put('"');
  put(':');
  comma_ = false;
}

void JsonWriter::str(const char* s) {
  sep();
  put('"');
  for (; *s; s++) {
    unsigned char c = (unsigned char)*s;
    if (c == '"' || c == '\\') {
      put('\\');
      put((char)c);
    } else if (c < 0x20) {
      char tmp[8];
      snprintf(tmp, sizeof(tmp), "\\u%04x", c);
      raw(tmp);
    } else {
      put((char)c);
    }
  }
  put('"');
  comma_ = true;
}

void JsonWriter::u64(uint64_t v) {
  sep();
  char tmp[24];
  snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)v);
  raw(tmp);
  comma_ = true;
}

void JsonWriter::i32(int32_t v) {
  sep();
  char tmp[16];
  snprintf(tmp, sizeof(tmp), "%ld", (long)v);
  raw(tmp);
  comma_ = true;
}

void JsonWriter::num(float v) {
  sep();
  char tmp[24];
  snprintf(tmp, sizeof(tmp), "%.4g", (double)v);
  raw(tmp);
  comma_ = true;
}

// ============================================================================
//  Message reçu
// ============================================================================
bool Message::has(const char* k) const {
  for (uint8_t i = 0; i < count; i++)
    if (strcmp(keys[i], k) == 0) return true;
  return false;
}

float Message::get(const char* k, float fallback) const {
  for (uint8_t i = 0; i < count; i++)
    if (strcmp(keys[i], k) == 0) return values[i];
  return fallback;
}

static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

bool parseMessage(const char* payload, size_t len, Message& out) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) return false;
  if (!doc.is<JsonObject>()) return false;

  out = Message();
  out.version = doc["v"] | 0;
  out.ts = doc["ts"].as<uint64_t>();
  out.seq = doc["seq"] | 0u;

  JsonVariantConst val = doc["val"];
  const char* f = doc["f"] | "";
  bool text = strcmp(f, "string") == 0 || (f[0] == '\0' && val.is<const char*>());

  if (text) {
    if (!val.is<const char*>()) return false;
    out.isText = true;
    strncpy(out.text, val.as<const char*>(), SAILLANCE_TEXT_MAX);
    out.text[SAILLANCE_TEXT_MAX] = '\0';
    return true;
  }

  JsonArrayConst arr = val.as<JsonArrayConst>();
  if (arr.isNull()) {
    if (!val.is<float>()) return false;  // un seul nombre sans tableau : toléré
    out.count = 1;
    out.values[0] = clamp01(val.as<float>());
    strcpy(out.keys[0], "0");
    return true;
  }

  JsonArrayConst labels = doc["label"].as<JsonArrayConst>();
  uint8_t n = 0;
  for (JsonVariantConst v : arr) {
    if (n >= SAILLANCE_MAX_KEYS) break;
    if (!v.is<float>()) return false;
    out.values[n] = clamp01(v.as<float>());
    const char* lab = labels[n] | (const char*)nullptr;
    if (lab && lab[0]) {
      strncpy(out.keys[n], lab, SAILLANCE_KEY_MAX);
      out.keys[n][SAILLANCE_KEY_MAX] = '\0';
    } else {
      snprintf(out.keys[n], sizeof(out.keys[n]), "%u", (unsigned)n);
    }
    n++;
  }
  out.count = n;
  return true;
}

// ============================================================================
//  Canaux d'envoi
// ============================================================================
void OutChannel::configure(const Topic& t, uint8_t maxHz, SendMode m) {
  topic = t;
  mode = m;
  if (maxHz < SAILLANCE_MIN_HZ || maxHz > SAILLANCE_MAX_HZ) {
    uint8_t fixed = maxHz < SAILLANCE_MIN_HZ ? SAILLANCE_MIN_HZ : SAILLANCE_MAX_HZ;
    reportIssue(Severity::Warning, ErrorCode::ApiBadConfig, "output",
                "frequence %u Hz hors limites (%d a %d) -> %u Hz", (unsigned)maxHz,
                SAILLANCE_MIN_HZ, SAILLANCE_MAX_HZ, (unsigned)fixed);
    maxHz = fixed;
  }
  this->maxHz = maxHz;
  intervalMs = (1000u + maxHz - 1u) / maxHz;  // arrondi vers le haut : jamais plus vite que demandé
}

bool OutChannel::due(uint32_t nowMs) const {
  if (mode == SendMode::Event && !dirty) return false;
  if (!sentOnce) return true;
  return (uint32_t)(nowMs - lastSentMs) >= intervalMs;
}

void OutChannel::markSent(uint32_t nowMs) {
  if (mode == SendMode::Continuous && sentOnce && (uint32_t)(nowMs - lastSentMs) < 2 * intervalMs)
    lastSentMs += intervalMs;  // cadence régulière, sans dérive
  else
    lastSentMs = nowMs;
  sentOnce = true;
  dirty = false;
}

bool FloatChannel::addKey(const char* key) {
  Check c = checkKey(key);
  if (c != Check::Ok) {
    reportIssue(Severity::Error, ErrorCode::ApiBadKey, "output",
                "cle \"%s\" invalide : %s (max %d, lettres/chiffres/_)", key ? key : "",
                checkText(c), SAILLANCE_KEY_MAX);
    return false;
  }
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(keys[i], key) == 0) {
      reportIssue(Severity::Error, ErrorCode::ApiBadKey, "output", "cle \"%s\" en double", key);
      return false;
    }
  }
  if (count >= SAILLANCE_MAX_KEYS) {
    reportIssue(Severity::Error, ErrorCode::ApiBadKey, "output", "trop de cles (max %d)",
                SAILLANCE_MAX_KEYS);
    return false;
  }
  strcpy(keys[count], key);
  values[count] = 0.0f;
  count++;
  return true;
}

void FloatChannel::set(const char* key, float v) {
  int idx = -1;
  for (uint8_t i = 0; i < count; i++) {
    if (key && strcmp(keys[i], key) == 0) {
      idx = i;
      break;
    }
  }
  if (idx < 0) {
    if (!warnedKey) {
      warnedKey = true;
      reportIssue(Severity::Warning, ErrorCode::ApiBadKey, "output",
                  "cle \"%s\" inconnue sur %s (declaree a la creation ?)", key ? key : "",
                  topic.c_str());
    }
    return;
  }
  if (isnan(v)) {
    if (!warnedRange) {
      warnedRange = true;
      reportIssue(Severity::Warning, ErrorCode::ApiBadValue, "output", "%s/%s : valeur NaN ignoree",
                  topic.c_str(), key);
    }
    return;
  }
  if (v < 0.0f || v > 1.0f) {
    if (!warnedRange) {
      warnedRange = true;
      reportIssue(Severity::Warning, ErrorCode::ApiBadValue, "output",
                  "%s : valeur %.3g hors de 0..1, ramenee dans l'intervalle", key, (double)v);
    }
    v = clamp01(v);
  }
  if (values[idx] != v) {
    values[idx] = v;
    dirty = true;
  }
}

size_t FloatChannel::build(char* buf, size_t cap, uint64_t ts) {
  JsonWriter w(buf, cap);
  w.beginObject();
  w.key("v"); w.i32(SAILLANCE_FORMAT_VERSION);
  w.key("ts"); w.u64(ts);
  w.key("seq"); w.u64(seq);
  w.key("val");
  w.beginArray();
  for (uint8_t i = 0; i < count; i++) w.num(values[i]);
  w.endArray();
  w.key("f"); w.str("float");
  w.key("label");
  w.beginArray();
  for (uint8_t i = 0; i < count; i++) w.str(keys[i]);
  w.endArray();
  w.endObject();
  size_t n = w.finish();
  if (n) seq++;
  return n;
}

void TextChannel::set(const char* s) {
  if (!s) s = "";
  if (strlen(s) > SAILLANCE_TEXT_MAX && !warnedLen) {
    warnedLen = true;
    reportIssue(Severity::Warning, ErrorCode::ApiBadValue, "output",
                "texte trop long (max %d), coupe", SAILLANCE_TEXT_MAX);
  }
  if (strncmp(text, s, SAILLANCE_TEXT_MAX) != 0) {
    strncpy(text, s, SAILLANCE_TEXT_MAX);
    text[SAILLANCE_TEXT_MAX] = '\0';
    dirty = true;
  }
}

size_t TextChannel::build(char* buf, size_t cap, uint64_t ts) {
  JsonWriter w(buf, cap);
  w.beginObject();
  w.key("v"); w.i32(SAILLANCE_FORMAT_VERSION);
  w.key("ts"); w.u64(ts);
  w.key("seq"); w.u64(seq);
  w.key("val"); w.str(text);
  w.key("f"); w.str("string");
  w.endObject();
  size_t n = w.finish();
  if (n) seq++;
  return n;
}

void InputChannel::deliver(const Message& m) {
  last = m;
  hasData = true;
  fresh = true;
  if (callback) callback(last);
}

// ============================================================================
//  Messages système
// ============================================================================
size_t buildErrorPayload(char* buf, size_t cap, uint64_t ts, uint32_t seq, const char* src,
                         const char* msg, Severity sv, ErrorCode code) {
  JsonWriter w(buf, cap);
  w.beginObject();
  w.key("v"); w.i32(SAILLANCE_FORMAT_VERSION);
  w.key("ts"); w.u64(ts);
  w.key("seq"); w.u64(seq);
  w.key("src"); w.str(src);
  w.key("msg"); w.str(msg);
  w.key("sv"); w.i32((int32_t)sv);
  w.key("t"); w.i32((int32_t)code);
  w.endObject();
  return w.finish();
}

size_t buildInfosPayload(char* buf, size_t cap, uint64_t ts, uint32_t seq, uint32_t fps,
                         uint32_t droppedFrames, uint32_t gpu, uint32_t cpu) {
  JsonWriter w(buf, cap);
  w.beginObject();
  w.key("v"); w.i32(SAILLANCE_FORMAT_VERSION);
  w.key("ts"); w.u64(ts);
  w.key("seq"); w.u64(seq);
  w.key("fps"); w.u64(fps);
  w.key("df"); w.u64(droppedFrames);
  w.key("gpu"); w.u64(gpu);
  w.key("cpu"); w.u64(cpu);
  w.key("ac"); w.i32(1);
  w.key("performMode"); w.i32(1);
  w.endObject();
  return w.finish();
}

}  // namespace saillance
