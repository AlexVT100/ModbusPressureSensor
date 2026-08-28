from __future__ import annotations

from homeassistant.components.sensor import SensorDeviceClass, SensorEntity, SensorStateClass
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import EntityCategory, UnitOfPressure
from homeassistant.core import HomeAssistant
from homeassistant.helpers.device_registry import DeviceInfo
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN, STATUS_OPTIONS
from .coordinator import ModbusPressureSensorCoordinator


async def async_setup_entry(
    hass: HomeAssistant, entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    coordinator = hass.data[DOMAIN][entry.entry_id]
    async_add_entities([
        ModbusPressureSensorSensor(coordinator, entry),
        ModbusPressureSensorStatusSensor(coordinator, entry),
    ])


class ModbusPressureSensorBaseEntity(CoordinatorEntity[ModbusPressureSensorCoordinator]):
    @property
    def device_info(self) -> DeviceInfo:
        return DeviceInfo(
            identifiers={(DOMAIN, self.coordinator.entry.entry_id)},
            name="Modbus Pressure Sensor",
            manufacturer="Custom",
            model="ESP8266 Modbus TCP",
        )


class ModbusPressureSensorSensor(ModbusPressureSensorBaseEntity, SensorEntity):
    _attr_has_entity_name = True
    _attr_translation_key = "pressure"
    _attr_device_class = SensorDeviceClass.PRESSURE
    _attr_state_class = SensorStateClass.MEASUREMENT
    _attr_native_unit_of_measurement = UnitOfPressure.BAR
    _attr_suggested_display_precision = 2

    def __init__(self, coordinator, entry) -> None:
        super().__init__(coordinator)
        self._attr_unique_id = f"{entry.entry_id}_pressure"

    @property
    def native_value(self):
        return None if self.coordinator.data is None else self.coordinator.data["pressure"]

    @property
    def extra_state_attributes(self):
        if self.coordinator.data is None:
            return {}
        return {
            "pressure_raw": self.coordinator.data["pressure_raw"],
            "low_threshold": self.coordinator.data["low_threshold"],
            "high_threshold": self.coordinator.data["high_threshold"],
        }


class ModbusPressureSensorStatusSensor(ModbusPressureSensorBaseEntity, SensorEntity):
    _attr_has_entity_name = True
    _attr_translation_key = "status"
    _attr_device_class = SensorDeviceClass.ENUM
    _attr_entity_category = EntityCategory.DIAGNOSTIC
    _attr_options = STATUS_OPTIONS

    def __init__(self, coordinator, entry) -> None:
        super().__init__(coordinator)
        self._attr_unique_id = f"{entry.entry_id}_status"

    @property
    def native_value(self):
        if self.coordinator.data is None:
            return None
        status = self.coordinator.data["status"]
        return status if status in STATUS_OPTIONS else None

    @property
    def extra_state_attributes(self):
        if self.coordinator.data is None:
            return {}
        return {"status_raw": self.coordinator.data["status_raw"]}
