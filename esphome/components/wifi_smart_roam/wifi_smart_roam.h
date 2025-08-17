#pragma once
#include "esphome/core/component.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#if defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include "esp_wifi.h"
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
  extern "C" {
    #include "user_interface.h"
  }
#endif

namespace esphome {
namespace wifi_smart_roam {

class WifiSmartRoam : public Component {
 public:
  void set_target_ssid(const std::string &s) { target_ssid_ = s; }
  void set_stronger_by_db(int v) { stronger_by_db_ = v; }
  void set_min_rssi_to_consider(int v) { min_rssi_to_consider_ = v; }
  void set_interval_ms(uint32_t ms) { interval_ms_ = ms; }

  void set_current_rssi_sensor(sensor::Sensor *s) { current_rssi_sensor_ = s; }
  void set_best_rssi_sensor(sensor::Sensor *s) { best_rssi_sensor_ = s; }
  void set_current_bssid_text(text_sensor::TextSensor *t) { current_bssid_text_ = t; }
  void set_best_bssid_text(text_sensor::TextSensor *t) { best_bssid_text_ = t; }

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void setup() override {
    last_run_ = millis();
    publish_current_();
  }

  void loop() override {
    const uint32_t now = millis();
    if (now - last_run_ < interval_ms_) return;
    last_run_ = now;

    if (WiFi.status() != WL_CONNECTED) return;

    const std::string ssid = target_ssid_.empty() ? std::string(WiFi.SSID().c_str())
                                                  : target_ssid_;
    if (ssid.empty()) return;

    const int cur_rssi = WiFi.RSSI();
    const String cur_bssid = WiFi.BSSIDstr();

    publish_current_();

    // Active scan, include hidden to catch multi-AP deployments that don't beacon name identically
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

    if (best_rssi_sensor_) best_rssi_sensor_->publish_state(best_rssi > -127 ? best_rssi : NAN);
    if (best_bssid_text_)  best_bssid_text_->publish_state(best_rssi > -127 ? best_bssid_str.c_str() : "");

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
  }

 protected:
  static constexpr const char *TAG = "wifi_smart_roam";

  std::string target_ssid_;
  int stronger_by_db_{6};
  int min_rssi_to_consider_{-85};
  uint32_t interval_ms_{3600 * 1000UL};
  uint32_t last_run_{0};

  sensor::Sensor *current_rssi_sensor_{nullptr};
  sensor::Sensor *best_rssi_sensor_{nullptr};
  text_sensor::TextSensor *current_bssid_text_{nullptr};
  text_sensor::TextSensor *best_bssid_text_{nullptr};

  void publish_current_() {
    if (current_rssi_sensor_) current_rssi_sensor_->publish_state(WiFi.RSSI());
    if (current_bssid_text_) current_bssid_text_->publish_state(WiFi.BSSIDstr().c_str());
  }

  // Parse "aa:bb:cc:dd:ee:ff" to 6 bytes
  static bool parse_bssid_(const String &str, uint8_t out[6]) {
    if (str.length() != 17) return false;
    for (int i = 0, j = 0; i < 17 && j < 6; i += 3, j++) {
      char buf[3] = { str[i], str[i+1], 0 };
      out[j] = static_cast<uint8_t>(strtoul(buf, nullptr, 16));
      if (i + 2 < 17 && str[i + 2] != ':') return false;
    }
    return true;
  }

  void steer_to_bssid_(const char *ssid, const String &bssid_str, int channel) {
    uint8_t bssid[6]{0};
    if (!parse_bssid_(bssid_str, bssid)) return;

#if defined(ARDUINO_ARCH_ESP32)
    wifi_config_t cfg{};
    esp_wifi_get_config(WIFI_IF_STA, &cfg);
    memcpy(cfg.sta.bssid, bssid, 6);
    cfg.sta.bssid_set = 1;
    // keep credentials as-is; ESPHome already configured them
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_wifi_disconnect();
    delay(150);
    esp_wifi_connect();

#elif defined(ARDUINO_ARCH_ESP8266)
    // Read current SDK station config (includes SSID & password set by ESPHome)
    station_config cfg{};
    wifi_station_get_config(&cfg);

    // Ensure SSID remains the same (optional; otherwise keep current)
    if (ssid && *ssid) {
      memset(cfg.ssid, 0, sizeof(cfg.ssid));
      strncpy(reinterpret_cast<char *>(cfg.ssid), ssid, sizeof(cfg.ssid) - 1);
    }

    // Pin target BSSID
    memcpy(cfg.bssid, bssid, 6);
    cfg.bssid_set = 1;  // lock association to this BSSID

    // Apply current config in RAM only
    wifi_station_disconnect();
    wifi_station_set_config_current(&cfg);

    // Hint channel if known (optional optimization)
    if (channel > 0) {
      wifi_set_channel(channel);
    }

    // Reconnect — SDK will honor bssid_set
    wifi_station_connect();
#endif
  }
};

}  // namespace wifi_smart_roam
}  // namespace esphome
