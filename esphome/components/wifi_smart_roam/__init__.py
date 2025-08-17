import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INTERVAL
from esphome.components import sensor, text_sensor

CODEOWNERS = ["@you"]
AUTO_LOAD = ["wifi"]
MULTI_CONF = False

wifi_smart_roam_ns = cg.esphome_ns.namespace("wifi_smart_roam")
WifiSmartRoam = wifi_smart_roam_ns.class_("WifiSmartRoam", cg.Component)

CONF_TARGET_SSID = "target_ssid"
CONF_STRONGER_BY_DB = "stronger_by_db"
CONF_MIN_RSSI = "min_rssi_to_consider"
CONF_CURRENT_RSSI = "current_rssi"
CONF_BEST_RSSI = "best_rssi"
CONF_CURRENT_BSSID = "current_bssid"
CONF_BEST_BSSID = "best_bssid"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(WifiSmartRoam),
    cv.Optional(CONF_TARGET_SSID, default=""): cv.string,
    cv.Optional(CONF_STRONGER_BY_DB, default=6): cv.int_,
    cv.Optional(CONF_MIN_RSSI, default=-85): cv.int_,
    cv.Optional(CONF_INTERVAL, default="1h"): cv.positive_time_period,
    cv.Optional(CONF_CURRENT_RSSI): sensor.sensor_schema(
        unit_of_measurement="dBm",
        accuracy_decimals=0,
        icon="mdi:wifi"
    ),
    cv.Optional(CONF_BEST_RSSI): sensor.sensor_schema(
        unit_of_measurement="dBm",
        accuracy_decimals=0,
        icon="mdi:access-point"
    ),
    cv.Optional(CONF_CURRENT_BSSID): text_sensor.text_sensor_schema(
        icon="mdi:router-wireless"
    ),
    cv.Optional(CONF_BEST_BSSID): text_sensor.text_sensor_schema(
        icon="mdi:router-wireless"
    ),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_target_ssid(config[CONF_TARGET_SSID]))
    cg.add(var.set_stronger_by_db(config[CONF_STRONGER_BY_DB]))
    cg.add(var.set_min_rssi_to_consider(config[CONF_MIN_RSSI]))

    # TimePeriod → milliseconds
    interval_ms = config[CONF_INTERVAL].total_milliseconds
    cg.add(var.set_interval_ms(interval_ms))

    if CONF_CURRENT_RSSI in config:
        s = await sensor.new_sensor(config[CONF_CURRENT_RSSI])
        cg.add(var.set_current_rssi_sensor(s))
    if CONF_BEST_RSSI in config:
        s = await sensor.new_sensor(config[CONF_BEST_RSSI])
        cg.add(var.set_best_rssi_sensor(s))
    if CONF_CURRENT_BSSID in config:
        ts = await text_sensor.new_text_sensor(config[CONF_CURRENT_BSSID])
        cg.add(var.set_current_bssid_text(ts))
    if CONF_BEST_BSSID in config:
        ts = await text_sensor.new_text_sensor(config[CONF_BEST_BSSID])
        cg.add(var.set_best_bssid_text(ts))
