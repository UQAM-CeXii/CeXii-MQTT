// Client MQTT : on s'appuie sur esp-mqtt (inclus dans le framework Arduino-ESP32),
// qui sait parler MQTT par-dessus WebSocket (ws://).
#include <esp_arduino_version.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <mqtt_client.h>

#include "Saillance.h"

namespace saillance {

bool SaillanceClass::startMqtt_() {
  if (!client_) {
    esp_mqtt_client_config_t cfg = {};
    const char* pass = mqttPass_[0] ? mqttPass_ : nullptr;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    cfg.broker.address.uri = uri_;
    cfg.credentials.client_id = username_;
    cfg.credentials.username = username_;
    cfg.credentials.authentication.password = pass;
    cfg.session.keepalive = 15;
    cfg.buffer.size = 1024;
    cfg.network.reconnect_timeout_ms = 3000;
    cfg.task.stack_size = 8192;
#else
    cfg.uri = uri_;
    cfg.client_id = username_;
    cfg.username = username_;
    cfg.password = pass;
    cfg.keepalive = 15;
    cfg.buffer_size = 1024;
    cfg.reconnect_timeout_ms = 3000;
    cfg.task_stack = 8192;
#endif
    client_ = esp_mqtt_client_init(&cfg);
    if (!client_) {
      reportIssue(Severity::Critical, ErrorCode::MqttError, "mqtt", "esp_mqtt_client_init a echoue");
      return false;
    }
    esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, &SaillanceClass::onMqttEvent_, this);
    say("MQTT : connexion a %s en tant que \"%s\"...", uri_, username_);
    esp_mqtt_client_start(client_);
  } else {
    esp_mqtt_client_start(client_);  // redémarrage après un lien perdu
  }
  mqttStarted_ = true;
  return true;
}

void SaillanceClass::stopMqtt_() {
  if (client_) esp_mqtt_client_stop(client_);
  mqttStarted_ = false;
  mqttConnected_ = false;
}

// Démarre / arrête MQTT selon l'état du réseau (appelé depuis update()).
void SaillanceClass::syncMqtt_() {
  if (netUp_ && !mqttStarted_)
    startMqtt_();
  else if (!netUp_ && mqttStarted_)
    stopMqtt_();

  if (!mqttConnected_)
    for (InputChannel* in : inputs_) in->subscribed = false;
}

bool SaillanceClass::publish_(const char* topic, const char* data, size_t len, int qos) {
  if (!client_ || !mqttConnected_) return false;
  // Envoi direct. Surtout pas esp_mqtt_client_enqueue(store=true) : la file interne d'esp-mqtt ne
  // libère qu'environ 1 message par seconde et jette les messages après 30 s.
  return esp_mqtt_client_publish(client_, topic, data, (int)len, qos, 0) >= 0;
}

void SaillanceClass::subscribeAll_() {
  for (InputChannel* in : inputs_) {
    if (in->subscribed) continue;
    if (esp_mqtt_client_subscribe(client_, in->topic.c_str(), 0) >= 0) {
      in->subscribed = true;
      say("abonne a %s", in->topic.c_str());
    }
  }
}

// Les messages reçus arrivent dans la tâche MQTT ; on les passe à loop() par une file,
// pour que les fonctions de l'utilisateur s'exécutent toujours dans loop().
void SaillanceClass::dispatchIncoming_() {
  if (!incomingQueue_) return;
  Incoming it;
  for (int budget = 8; budget > 0 && xQueueReceive((QueueHandle_t)incomingQueue_, &it, 0) == pdTRUE; budget--) {
    Message m;
    if (!parseMessage(it.payload, it.len, m)) {
      reportIssue(Severity::Warning, ErrorCode::ApiBadMessage, "input", "message illisible sur %s", it.topic);
      continue;
    }
    for (InputChannel* in : inputs_)
      if (strcmp(in->topic.c_str(), it.topic) == 0) in->deliver(m);
  }
}

void SaillanceClass::onMqttEvent_(void*, const char*, int32_t id, void* data) {
  SaillanceClass& s = Saillance;
  esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)data;
  static bool failureShown = false;

  switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED:
      s.mqttConnected_ = true;
      s.infosDue_ = true;
      failureShown = false;
      s.say("MQTT connecte");
      break;

    case MQTT_EVENT_DISCONNECTED:
      if (s.mqttConnected_) s.say("MQTT deconnecte, nouvelle tentative...");
      s.mqttConnected_ = false;
      break;

    case MQTT_EVENT_DATA: {
      if (ev->current_data_offset != 0) break;  // suite d'un gros message déjà rejeté
      if (ev->topic_len <= 0 || ev->topic_len > SAILLANCE_TOPIC_MAX || ev->total_data_len > SAILLANCE_PAYLOAD_MAX) {
        s.reportError("input", "message recu trop gros, ignore", Severity::Warning, ErrorCode::ApiBadMessage);
        break;
      }
      Incoming it;
      it.len = (uint16_t)ev->data_len;
      memcpy(it.topic, ev->topic, ev->topic_len);
      it.topic[ev->topic_len] = '\0';
      memcpy(it.payload, ev->data, ev->data_len);
      it.payload[ev->data_len] = '\0';
      if (s.incomingQueue_) xQueueSend((QueueHandle_t)s.incomingQueue_, &it, 0);
      break;
    }

    case MQTT_EVENT_ERROR:
      if (ev->error_handle && ev->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
        char msg[96];
        snprintf(msg, sizeof(msg), "connexion refusee par le broker (code %d) : username/mot de passe ?",
                 (int)ev->error_handle->connect_return_code);
        s.reportError("mqtt", msg, Severity::Error, ErrorCode::MqttRefused);
      } else if (!failureShown) {
        failureShown = true;
        s.say("broker injoignable (%s), on reessaie en boucle", s.uri_);
      }
      break;

    default:
      break;
  }
}

}  // namespace saillance
