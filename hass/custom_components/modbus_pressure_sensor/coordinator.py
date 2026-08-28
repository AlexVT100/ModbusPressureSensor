from __future__ import annotations

from datetime import timedelta
import logging
from typing import Any

from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.exceptions import HomeAssistantError
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator, UpdateFailed

from modbus_connection import ModbusError, ModbusTcpParams
from modbus_connection.pymodbus import ModbusConnection

from .const import (
    CONF_HOST, CONF_PORT, CONF_SCAN_INTERVAL, CONF_UNIT_ID,
    DEFAULT_SCAN_INTERVAL, DOMAIN, MODBUS_MESSAGE_SPACING, MODBUS_TIMEOUT,
    PRESSURE_SCALE, REGISTER_HIGH_LIMIT, REGISTER_LOW_LIMIT,
    REGISTER_PRESSURE, STATUS_TEXT,
)

_LOGGER = logging.getLogger(__name__)


class ModbusPressureSensorCoordinator(DataUpdateCoordinator[dict[str, Any]]):
    def __init__(self, hass: HomeAssistant, entry: ConfigEntry) -> None:
        self.entry = entry
        self.port = int(entry.data[CONF_PORT])
        self.unit_id = int(entry.data[CONF_UNIT_ID])

        self.connection = ModbusConnection(
            ModbusTcpParams(host=entry.data[CONF_HOST], port=self.port),
            timeout=MODBUS_TIMEOUT,
            message_spacing=MODBUS_MESSAGE_SPACING,
        )
        self.unit = self.connection.for_unit(self.unit_id)
        self._unsubscribe_connection_lost = self.connection.on_connection_lost(self._on_connection_lost)

        interval = entry.options.get(
            CONF_SCAN_INTERVAL,
            entry.data.get(CONF_SCAN_INTERVAL, DEFAULT_SCAN_INTERVAL),
        )
        super().__init__(
            hass, _LOGGER, name=DOMAIN,
            update_interval=timedelta(seconds=interval),
        )

    async def async_connect(self) -> None:
        try:
            await self.connection.connect()
        except ModbusError as err:
            raise UpdateFailed(
                f"Unable to connect to Modbus Pressure Sensor: {err}"
            ) from err

    def _on_connection_lost(self) -> None:
        _LOGGER.warning(
            "Connection to Modbus Pressure Sensor at %s:%s was lost",
            self.entry.data[CONF_HOST],
            self.entry.data[CONF_PORT],
        )

    async def _async_update_data(self) -> dict[str, Any]:
        try:
            if not self.connection.connected:
                await self.connection.connect()

            inputs = await self.unit.read_input_registers(REGISTER_PRESSURE, 2)
            holdings = await self.unit.read_holding_registers(REGISTER_LOW_LIMIT, 2)

            if len(inputs) != 2 or len(holdings) != 2:
                raise UpdateFailed("Incomplete Modbus response")

            pressure_raw = inputs[0]
            status_raw = self._to_int16(inputs[1])

            return {
                "pressure": pressure_raw * PRESSURE_SCALE,
                "pressure_raw": pressure_raw,
                "status": STATUS_TEXT.get(status_raw, "Unknown"),
                "status_raw": status_raw,
                "low_threshold": holdings[0] * PRESSURE_SCALE,
                "high_threshold": holdings[1] * PRESSURE_SCALE,
            }
        except ModbusError as err:
            raise UpdateFailed(f"Unable to communicate with Modbus Pressure Sensor: {err}") from err

    @staticmethod
    def _to_int16(value: int) -> int:
        value &= 0xFFFF
        return value - 0x10000 if value & 0x8000 else value

    async def async_write_limit(self, address: int, value: float) -> None:
        raw_value = round(value / PRESSURE_SCALE)
        if not 0 <= raw_value <= 0xFFFF:
            raise HomeAssistantError(
                f"Pressure alert threshold {value} bar is outside the Modbus range"
            )

        try:
            if not self.connection.connected:
                await self.connection.connect()
            await self.unit.write_register(address, raw_value)
        except ModbusError as err:
            raise HomeAssistantError(
                f"Unable to write pressure alert threshold: {err}"
            ) from err

        await self.async_request_refresh()

    async def async_shutdown(self) -> None:
        if self._unsubscribe_connection_lost is not None:
            self._unsubscribe_connection_lost()
            self._unsubscribe_connection_lost = None
        await self.connection.close()
