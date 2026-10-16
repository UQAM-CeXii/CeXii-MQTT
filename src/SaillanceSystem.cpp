// Messages système automatiques : /error (sur événement) et /infos (toutes les 5 s).
#include "Saillance.h"

#include <esp_freertos_hooks.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace saillance {

// --- Mesure de charge CPU ----------------------------------------------------
// Chaque coeur compte combien de fois sa tâche "idle" (repos) tourne. Moins elle
// tourne, plus le processeur est occupé. Le maximum observé sert de référence (100 % libre).
static volatile uint32_t s_idle[2] = {0, 0};
static bool idleHook0() { s_idle[0]++; return true; }
static bool idleHook1() { s_idle[1]++; return true; }

static vprintf_like_t s_prevVprintf = nullptr;

static uint32_t fnv1a(const char* a, const char* b) {
  uint32_t h = 2166136261u;
  for (const char* p = a; *p; p++) h = (h ^ (uint8_t)*p) * 16777619u;
  for (const char* p = b; *p; p++) h = (h ^ (uint8_t)*p) * 16777619u;
  return h;
}

void SaillanceClass::startSystem_() {
  setIssueHook(&SaillanceClass::onIssue_);

  // 1) Capture de toutes les erreurs du système (ESP_LOGE, log_e, WiFi, MQTT, ...)
  s_prevVprintf = esp_log_set_vprintf(&SaillanceClass::logHook_);

  // 2) Mesure du CPU : calibrage pendant 100 ms de repos
  esp_register_freertos_idle_hook_for_cpu(idleHook0, 0);
#if portNUM_PROCESSORS > 1
  esp_register_freertos_idle_hook_for_cpu(idleHook1, 1);
#endif
  uint32_t c0[2] = {s_idle[0], s_idle[1]};
  delay(100);
  for (int c = 0; c < portNUM_PROCESSORS && c < 2; c++) {
    idleMaxRate_[c] = (float)(s_idle[c] - c0[c]) * 10.0f;
    idleLast_[c] = s_idle[c];
  }
  lastCpuMs_ = millis();

  // 3) Pourquoi a-t-on redémarré ? Un plantage ne peut pas être envoyé sur le coup :
  //    on le signale au démarrage suivant.
  const char* why = nullptr;
  Severity sv = Severity::Critical;
  switch (esp_reset_reason()) {
    case ESP_RST_PANIC: why = "plantage (panic)"; break;
    case ESP_RST_INT_WDT: why = "watchdog d'interruption"; break;
    case ESP_RST_TASK_WDT: why = "watchdog de tache (loop trop longue ?)"; break;
    case ESP_RST_WDT: why = "watchdog"; break;
    case ESP_RST_BROWNOUT: why = "baisse de tension (alimentation faible)"; sv = Severity::Error; break;
    default: break;
  }
  if (why) {
    char msg[64];
    snprintf(msg, sizeof(msg), "redemarrage anormal : %s", why);
    reportError("boot", msg, sv, ErrorCode::SysReset);
  }
}

// Intercepte chaque ligne de log ; si c'est une erreur ("E (123) tag: message"),
// on la met en file d'attente. L'envoi se fait plus tard, dans update().
int SaillanceClass::logHook_(const char* fmt, va_list args) {
  SaillanceClass& s = Saillance;
  if (s.errorQueue_ && !xPortInIsrContext()) {
    va_list copy;
    va_copy(copy, args);
    char line[128];
    int n = vsnprintf(line, sizeof(line), fmt, copy);
    va_end(copy);
    if (n > 0) {
      const char* p = line;
      if (p[0] == '\033') {  // couleur ANSI
        const char* m = strchr(p, 'm');
        if (m) p = m + 1;
      }
      if (p[0] == 'E' && p[1] == ' ' && p[2] == '(') {
        ErrorItem it;
        memset(&it, 0, sizeof(it));
        it.ts = s.timestampMs();
        it.sv = Severity::Error;
        it.code = ErrorCode::SysLog;
        const char* close = strchr(p, ')');
        const char* tag = close ? close + 2 : p;
        const char* colon = strchr(tag, ':');
        size_t tagLen = colon ? (size_t)(colon - tag) : 0;
        if (tagLen == 0 || tagLen >= sizeof(it.src)) {
          strcpy(it.src, "esp");
        } else {
          memcpy(it.src, tag, tagLen);
        }
        const char* msg = colon ? colon + 1 : tag;
        while (*msg == ' ') msg++;
        size_t i = 0;
        for (; msg[i] && msg[i] != '\n' && msg[i] != '\r' && msg[i] != '\033' && i < sizeof(it.msg) - 1; i++)
          it.msg[i] = msg[i];
        it.msg[i] = '\0';
        xQueueSend((QueueHandle_t)s.errorQueue_, &it, 0);
      }
    }
  }
  return s_prevVprintf ? s_prevVprintf(fmt, args) : vprintf(fmt, args);
}

void SaillanceClass::onIssue_(Severity sv, ErrorCode code, const char* src, const char* msg) {
  Saillance.reportError(src, msg, sv, code);
}

void SaillanceClass::reportError(const char* source, const char* message, Severity sv, ErrorCode code) {
  if (debug_) {
    static const char* const names[] = {"info", "attention", "ERREUR", "CRITIQUE"};
    Serial.printf("[Saillance] %s (%s) : %s\n", names[(uint8_t)sv & 3], source ? source : "?", message ? message : "");
  }
  if (!errorQueue_) return;
  ErrorItem it;
  memset(&it, 0, sizeof(it));
  it.ts = timestampMs();
  it.sv = sv;
  it.code = code;
  strncpy(it.src, source ? source : "user", sizeof(it.src) - 1);
  strncpy(it.msg, message ? message : "", sizeof(it.msg) - 1);
  xQueueSend((QueueHandle_t)errorQueue_, &it, 0);  // file pleine : on abandonne ce message
}

void SaillanceClass::publishErrors_() {
  if (!mqttConnected_ || !errorQueue_) return;
  for (int i = 0; i < 3; i++) {
    ErrorItem it;
    if (xQueueReceive((QueueHandle_t)errorQueue_, &it, 0) != pdTRUE) break;

    // Même erreur qui se répète en boucle : un seul envoi toutes les 5 s.
    uint32_t now = millis();
    uint32_t h = fnv1a(it.src, it.msg);
    if (h == lastErrHash_ && now - lastErrMs_ < 5000) continue;
    lastErrHash_ = h;
    lastErrMs_ = now;

    size_t n = buildErrorPayload(payload_, sizeof(payload_), it.ts, errorSeq_, it.src, it.msg, it.sv, it.code);
    if (n && publish_(errorTopic_, payload_, n, 1)) {
      if (errorSeq_ == 0) say("premiere erreur envoyee sur %s", errorTopic_);
      errorSeq_++;
    }
  }
}

float SaillanceClass::sampleCpu_() {
  uint32_t now = millis();
  uint32_t dt = now - lastCpuMs_;
  if (dt == 0) dt = 1;
  float busy = 0;
  int cores = 0;
  for (int c = 0; c < portNUM_PROCESSORS && c < 2; c++, cores++) {
    uint32_t cnt = s_idle[c];
    float rate = (float)(cnt - idleLast_[c]) * 1000.0f / (float)dt;
    idleLast_[c] = cnt;
    if (rate > idleMaxRate_[c]) idleMaxRate_[c] = rate;
    float b = idleMaxRate_[c] > 0 ? 1.0f - rate / idleMaxRate_[c] : 0.0f;
    busy += b < 0 ? 0 : (b > 1 ? 1 : b);
  }
  lastCpuMs_ = now;
  return cores ? busy / cores * 100.0f : 0.0f;
}

void SaillanceClass::publishInfos_(uint32_t now) {
  if (!infosDue_ && (uint32_t)(now - lastInfosMs_) < SAILLANCE_INFOS_PERIOD_MS) return;

  uint32_t elapsed = now - lastInfosMs_;
  if (elapsed == 0) elapsed = 1;
  uint32_t fps = (uint32_t)((uint64_t)frames_ * 1000ULL / elapsed);
  uint32_t cpu = (uint32_t)(sampleCpu_() + 0.5f);
  lastInfosMs_ = now;
  frames_ = 0;

  if (ESP.getFreeHeap() < SAILLANCE_LOW_HEAP_BYTES && (lowHeapMs_ == 0 || now - lowHeapMs_ > 60000)) {
    lowHeapMs_ = now ? now : 1;
    char msg[48];
    snprintf(msg, sizeof(msg), "memoire basse : %u octets libres", (unsigned)ESP.getFreeHeap());
    reportError("heap", msg, Severity::Warning, ErrorCode::SysLowHeap);
  }

  if (!mqttConnected_) return;
  // gpu : l'ESP32 n'a pas de GPU -> 0
  size_t n = buildInfosPayload(payload_, sizeof(payload_), timestampMs(), infosSeq_, fps, droppedFrames_, 0, cpu);
  if (n && publish_(infosTopic_, payload_, n, 0)) {
    if (infosSeq_ == 0) say("premier message envoye sur %s", infosTopic_);
    infosSeq_++;
    droppedFrames_ = 0;
    infosDue_ = false;
  } else {
    say("echec d'envoi sur %s", infosTopic_);
  }
}

}  // namespace saillance
