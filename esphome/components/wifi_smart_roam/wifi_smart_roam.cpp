// Only include these if the features are enabled; matches ESPHome core style.
// This avoids include-path errors on minimal builds.
#ifdef USE_SENSOR
  #include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
  #include "esphome/components/text_sensor/text_sensor.h"
#endif

#include "wifi_smart_roam.h"

namespace esphome {
namespace wifi_smart_roam {

void WifiSmartRoam::setup() {
  last_run_ = millis();
  publish_current_();
}

void WifiSmartRoam::loop() {
  const uint32_t now = millis();
  if (now - last_run_ < interval_ms_) return;
  last_run_ = now;

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  if (WiFi.status() != WL_CONNECTED) return;

  const std::string ssid = target_ssid_.empty() ? std::string(WiFi.SSID().c_str())
                                                : target_ssid_;
  if (ssid.empty()) return;

  const int cur_rssi = WiFi.RSSI();
  const String cur_bssid = WiFi.BSSIDstr();
  publish_current_();

  const int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
  if (n <= 0) {
    ESP_LOGD(TAG, "Scan returned %d networks.", n);
    return;
  }

  int best_rssi = -127;
  String best_bssid_str;
  int best_channel = 0;

  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) != ssid.c_str()) continue;
    const String bssid_i = WiFi.BSSIDstr(i);
    if (bssid_i == cur_bssid) continue;
    const int rssi_i = WiFi.RSSI(i);
    if (rssi_i < min_rssi_to_consider_) continue;
    if (rssi_i > best_rssi) {
      best_rssi = rssi_i;
      best_bssid_str = bssid_i;
      best_channel = WiFi.channel(i);
    }
  }
  WiFi.scanDelete();

#ifdef USE_SENSOR
  if (best_rssi_sensor_) best_rssi_sensor_->publish_state(best_rssi > -127 ? best_rssi : NAN);
#endif
#ifdef USE_TEXT_SENSOR
  if (best_bssid_text_)  best_bssid_text_->publish_state(best_rssi > -127 ? best_bssid_str.c_str() : "");
#endif

  if (best_rssi <= -127) {
    ESP_LOGD(TAG, "No alternate BSSID for '%s' found.", ssid.c_str());
    return;
  }

  if (best_rssi >= cur_rssi + stronger_by_db_) {
    ESP_LOGI(TAG, "Roaming: %ddBm (%s) -> %ddBm (%s).",
             cur_rssi, cur_bssid.c_str(), best_rssi, best_bssid_str.c_str());
    steer_to_bssid_(ssid.c_str(), best_bssid_str, best_channel);
  } else {
    ESP_LOGD(TAG, "Best %ddBm not >= current %ddBm + %ddB. Staying.",
             best_rssi, cur_rssi, stronger_by_db_);
  }
#endif
}

void WifiSmartRoam::publish_current_() {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  #ifdef USE_SENSOR
    if (current_rssi_sensor_) current_rssi_sensor_->publish_state(WiFi.RSSI());
  #endif
  #ifdef USE_TEXT_SENSOR
    if (current_bssid_text_) current_bssid_text_->publish_state(WiFi.BSSIDstr().c_str());
  #endif
#endif
}

bool WifiSmartRoam::parse_bssid_(const String &str, uint8_t out[6]) {
  if (str.length() != 17) return false;  // "aa:bb:cc:dd:ee:ff"
  for (int i = 0, j = 0; i < 17 && j < 6; i += 3, j++) {
    char buf[3] = { str[i], str[i+1], 0 };
    out[j] = static_cast<uint8_t>(strtoul(buf, nullptr, 16));
    if (i + 2 < 17 && str[i + 2] != ':') return false;
  }
  return true;
}

void WifiSmartRoam::steer_to_bssid_(const char *ssid, const String &bssid_str, int channel) {
  uint8_t bssid[6]{0};
  if (!parse_bssid_(bssid_str, bssid)) return;

#if defined(ARDUINO_ARCH_ESP32)
  wifi_config_t cfg{};
  esp_wifi_get_config(WIFI_IF_STA, &cfg);
  memcpy(cfg.sta.bssid, bssid, 6);
  cfg.sta.bssid_set = 1;
  esp_wifi_set_config(WIFI_IF_STA, &cfg);
  esp_wifi_disconnect();
  delay(150);
  esp_wifi_connect();

#elif defined(ARDUINO_ARCH_ESP8266)
  station_config cfg{};
  wifi_station_get_config(&cfg);

  if (ssid && *ssid) {
    memset(cfg.ssid, 0, sizeof(cfg.ssid));
    strncpy(reinterpret_cast<char *>(cfg.ssid), ssid, sizeof(cfg.ssid) - 1);
  }
  memcpy(cfg.bssid, bssid, 6);
  cfg.bssid_set = 1;

  wifi_station_disconnect();
  wifi_station_set_config_current(&cfg);

  if (channel > 0) {
    wifi_set_channel(channel);
  }
  wifi_station_connect();
#endif
}

}  // namespace wifi_smart_roam
}  // namespace esphome
