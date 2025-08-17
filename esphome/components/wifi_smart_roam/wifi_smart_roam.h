#pragma once

#include "esphome/core/component.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/log.h"  // ESP_LOG*

// Try to include sensor/text_sensor; fall back gracefully if unavailable
#if __has_include("esphome/components/sensor/sensor.h")
  #include "esphome/components/sensor/sensor.h"
  #define WSR_HAS_SENSOR 1
#else
  #define WSR_HAS_SENSOR 0
#endif

#if __has_include("esphome/components/text_sensor/text_sensor.h")
  #include "esphome/components/text_sensor/text_sensor.h"
  #define WSR_HAS_TEXT_SENSOR 1
#else
  #define WSR_HAS_TEXT_SENSOR 0
#endif

#if defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include "esp_wifi.h"
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
  extern "C" {
    #include "user_interface.h"   // station_config, wifi_station_*
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

#if WSR_HAS_SENSOR
  void set_current_rssi_sensor(sensor::Sensor *s) { current_rssi_sensor_ = s; }
  void set_best_rssi_sensor(sensor::Sensor *s) { best_rssi_sensor_ = s; }
#else
  void set_current_rssi_sensor(void *) {}
  void set_best_rssi_sensor(void *) {}
#endif

#if WSR_HAS_TEXT_SENSOR
  void set_current_bssid_text(text_sensor::TextSensor *t) { current_bssid_text_ = t; }
  void set_best_bssid_text(text_sensor::TextSensor *t) { best_bssid_text_ = t; }
#else
  void set_current_bssid_text(void *) {}
  void set_best_bssid_text(void *) {}
#endif

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  // Boot behavior: publish "best = current" so values aren't unknown at startup
  void setup() override { 
    // (Optional) If you also want an immediate scan, uncomment the next line:
    // last_run_ = millis() - interval_ms_;
    last_run_ = millis();
    publish_current_();

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  #if WSR_HAS_SENSOR
    if (best_rssi_sensor_) best_rssi_sensor_->publish_state(WiFi.RSSI());
  #endif
  #if WSR_HAS_TEXT_SENSOR
    if (best_bssid_text_) best_bssid_text_->publish_state(WiFi.BSSIDstr().c_str());
  #endif
#endif
  }

  void loop() override;

 protected:
  static constexpr const char *TAG = "wifi_smart_roam";

  std::string target_ssid_;
  int stronger_by_db_{6};
  int min_rssi_to_consider_{-85};
  uint32_t interval_ms_{3600 * 1000UL};
  uint32_t last_run_{0};

#if WSR_HAS_SENSOR
  sensor::Sensor *current_rssi_sensor_{nullptr};
  sensor::Sensor *best_rssi_sensor_{nullptr};
#else
  void *current_rssi_sensor_{nullptr};
  void *best_rssi_sensor_{nullptr};
#endif

#if WSR_HAS_TEXT_SENSOR
  text_sensor::TextSensor *current_bssid_text_{nullptr};
  text_sensor::TextSensor *best_bssid_text_{nullptr};
#else
  void *current_bssid_text_{nullptr};
  void *best_bssid_text_{nullptr};
#endif

  void publish_current_();
  static bool parse_bssid_(const String &str, uint8_t out[6]);
  void steer_to_bssid_(const char *ssid, const String &bssid_str, int channel);
};

}  // namespace wifi_smart_roam
}  // namespace esphome
