#pragma once
#include "esphome/core/component.h"
#include "esphome/components/wifi/wifi_component.h"

// include full sensor/text_sensor headers so generated main.cpp has complete types
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#if defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include "esp_wifi.h"
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
  extern "C" {
    #include "user_interface.h"  // station_config, wifi_station_*
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

  void setup() override { last_run_ = millis(); publish_current_(); }
  void loop() override;

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

  void publish_current_();
  static bool parse_bssid_(const String &str, uint8_t out[6]);
  void steer_to_bssid_(const char *ssid, const String &bssid_str, int channel);
};

}  // namespace wifi_smart_roam
}  // namespace esphome
