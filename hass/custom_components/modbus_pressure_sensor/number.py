from __future__ import annotations

from homeassistant.components.number import NumberDeviceClass, NumberEntity
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import UnitOfPressure
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddEntitiesCallback

from .const import DOMAIN, REGISTER_HIGH_LIMIT, REGISTER_LOW_LIMIT
from .sensor import ModbusPressureSensorBaseEntity


async def async_setup_entry(
    hass: HomeAssistant, entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    coordinator = hass.data[DOMAIN][entry.entry_id]
    async_add_entities([
        ModbusPressureSensorLimitNumber(
            coordinator, entry, "low_threshold", REGISTER_LOW_LIMIT
        ),
        ModbusPressureSensorLimitNumber(
            coordinator, entry, "high_threshold", REGISTER_HIGH_LIMIT
        ),
    ])


class ModbusPressureSensorLimitNumber(ModbusPressureSensorBaseEntity, NumberEntity):
    _attr_has_entity_name = True
    _attr_native_min_value = 0.0
    _attr_native_max_value = 65.535
    _attr_native_step = 0.05
    _attr_native_unit_of_measurement = UnitOfPressure.BAR
    _attr_device_class = NumberDeviceClass.PRESSURE

    def __init__(self, coordinator, entry, key, address) -> None:
        super().__init__(coordinator)
        self._key = key
        self._address = address
        self._attr_translation_key = key
        self._attr_unique_id = f"{entry.entry_id}_{key}"

    @property
    def native_value(self):
        return None if self.coordinator.data is None else self.coordinator.data[self._key]

    async def async_set_native_value(self, value: float) -> None:
        await self.coordinator.async_write_limit(self._address, value)
