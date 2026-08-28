from typing import Final

DOMAIN: Final = "modbus_pressure_sensor"
CONF_HOST: Final = "host"
CONF_PORT: Final = "port"
CONF_UNIT_ID: Final = "unit_id"
CONF_SCAN_INTERVAL: Final = "scan_interval"

DEFAULT_PORT: Final = 502
DEFAULT_UNIT_ID: Final = 1
DEFAULT_SCAN_INTERVAL: Final = 10
MODBUS_TIMEOUT: Final = 5.0
MODBUS_MESSAGE_SPACING: Final = 0.03

REGISTER_PRESSURE: Final = 0
REGISTER_STATUS: Final = 1
REGISTER_LOW_LIMIT: Final = 0
REGISTER_HIGH_LIMIT: Final = 1
PRESSURE_SCALE: Final = 0.001

STATUS_TEXT: Final = {
    0: "normal",
    1: "pressure_too_low",
    2: "pressure_too_high",
    -1: "sensor_fault",
}
STATUS_OPTIONS: Final = list(STATUS_TEXT.values())
