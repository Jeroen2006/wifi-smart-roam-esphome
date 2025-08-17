#pragma once
#include "esphome/core/component.h"
#include "esphome/components/wifi/wifi_component.h"

#if defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include "esp_wifi.h"
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
  extern "C" {
    #include "user_interface.h"
  }
#endif

// forward declarations (avoid heavy includes here)
namespace esphome { namespace sensor { class Sensor; } }
namespace esphome { namespace text_sensor { class TextSensor; } }

namespace esphome {
namespace wifi_smart_roam {

class WifiSmartRoam : public Component {
 public:
  void set_target_ssid(const std::string &s);
  void set_stronger_by_db(int v);
  void set_min_rssi_to_consider(int v);
  void set_interval_ms(uint32_t ms);

  void set_current_rssi_sensor(sensor::Sensor *s);
  void set_best_rssi_sensor(sensor::Sensor *s);
  void set_current_bssid_text(text_sensor::TextSensor *t);
  void set_best_bssid_text(text_sensor::TextSensor *t);

  float get_setup_priority() const override;
  void setup() override;
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
