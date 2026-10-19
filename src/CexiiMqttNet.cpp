// Réseau : WiFi ou Ethernet filaire (cartes PoE d'Olimex).
#include <ETH.h>
#include <WiFi.h>
#include <esp_arduino_version.h>

#include "CexiiMqtt.h"

namespace cexii {

bool CexiiMqttClass::startNetwork_() {
  WiFi.onEvent(&CexiiMqttClass::onNetEvent_);

  if (link_ == Link::Wifi) {
    WiFi.persistent(false);  // ne pas user la flash à chaque démarrage
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(hostname_);
    WiFi.setAutoReconnect(true);
    say("WiFi : connexion a \"%s\"...", ssid_);
    WiFi.begin(ssid_, wifiPass_);
    return true;
  }

  // Ethernet : broches de la carte Olimex ESP32-POE / ESP32-POE-ISO (puce LAN8720).
  say("Ethernet : demarrage...");
#ifdef BOARD_HAS_PSRAM
  const eth_clock_mode_t clk = ETH_CLOCK_GPIO0_OUT;  // GPIO17 est pris par la PSRAM
#else
  const eth_clock_mode_t clk = ETH_CLOCK_GPIO17_OUT;
#endif
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  bool ok = ETH.begin(ETH_PHY_LAN8720, /*addr*/ 0, /*mdc*/ 23, /*mdio*/ 18, /*power*/ 12, clk);
#else
  bool ok = ETH.begin(/*addr*/ 0, /*power*/ 12, /*mdc*/ 23, /*mdio*/ 18, ETH_PHY_LAN8720, clk);
#endif
  if (!ok) {
    reportIssue(Severity::Critical, ErrorCode::NetFailed, "ethernet", "ETH.begin a echoue");
    return false;
  }
  return true;
}

void CexiiMqttClass::onNetEvent_(arduino_event_id_t event, arduino_event_info_t info) {
  CexiiMqttClass& s = CexiiMqtt;
  const bool wifi = s.link_ == Link::Wifi;
  const bool eth = s.link_ == Link::Ethernet;

  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      if (eth) ETH.setHostname(s.hostname_);
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    case ARDUINO_EVENT_ETH_GOT_IP:
      if ((event == ARDUINO_EVENT_WIFI_STA_GOT_IP && !wifi) || (event == ARDUINO_EVENT_ETH_GOT_IP && !eth)) break;
      s.say("%s connecte, IP %s", wifi ? "WiFi" : "Ethernet",
            (wifi ? WiFi.localIP() : ETH.localIP()).toString().c_str());
      s.netUp_ = true;
      if (!s.timeStarted_) {  // horodatage réel (NTP) pour le champ "ts"
        s.timeStarted_ = true;
        configTime(0, 0, s.ntpServer_);
      }
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (!wifi) break;
      s.netUp_ = false;
      s.say("WiFi perdu (raison %d), reconnexion automatique...", (int)info.wifi_sta_disconnected.reason);
      s.reportError("wifi", "lien WiFi perdu", Severity::Warning, ErrorCode::NetLinkLost);
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      if (!eth) break;
      s.netUp_ = false;
      s.say("Ethernet debranche (cable ?)");
      s.reportError("ethernet", "lien Ethernet perdu", Severity::Warning, ErrorCode::NetLinkLost);
      break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    case ARDUINO_EVENT_ETH_LOST_IP:
#endif
      s.netUp_ = false;
      break;

    default:
      break;
  }
}

}  // namespace cexii
