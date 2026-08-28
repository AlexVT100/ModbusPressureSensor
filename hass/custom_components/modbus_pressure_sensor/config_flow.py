from __future__ import annotations

from typing import Any
import voluptuous as vol

from homeassistant import config_entries
from homeassistant.config_entries import ConfigFlowResult, OptionsFlowWithReload
from homeassistant.helpers.selector import NumberSelector, NumberSelectorConfig
from homeassistant.core import HomeAssistant

from modbus_connection import ModbusError, ModbusTcpParams
from modbus_connection.pymodbus import ModbusConnection

from .const import (
    CONF_HOST, CONF_PORT, CONF_SCAN_INTERVAL, CONF_UNIT_ID,
    DEFAULT_PORT, DEFAULT_SCAN_INTERVAL, DEFAULT_UNIT_ID, DOMAIN,
    MODBUS_MESSAGE_SPACING, MODBUS_TIMEOUT,
)

USER_SCHEMA = vol.Schema({
    vol.Required(CONF_HOST): str,
    
    vol.Optional(CONF_PORT, default=DEFAULT_PORT): NumberSelector(
        NumberSelectorConfig(min=1, max=65535, step=1, mode="box")
    ),
    
    vol.Optional(CONF_UNIT_ID, default=DEFAULT_UNIT_ID): NumberSelector(
        NumberSelectorConfig(min=1, max=247, step=1, mode="box")
    ),
})

OPTIONS_SCHEMA = vol.Schema({
    vol.Required(CONF_SCAN_INTERVAL, default=DEFAULT_SCAN_INTERVAL): vol.All(
        vol.Coerce(int), vol.Range(min=1, max=3600)
    )
})


async def _test_connection(data: dict[str, Any]) -> None:
    connection = ModbusConnection(
        ModbusTcpParams(host=data[CONF_HOST], port=data[CONF_PORT]),
        timeout=MODBUS_TIMEOUT,
        message_spacing=MODBUS_MESSAGE_SPACING,
    )
    try:
        await connection.connect()
        unit = connection.for_unit(data[CONF_UNIT_ID])
        registers = await unit.read_input_registers(0, 2)
        if len(registers) != 2:
            raise ModbusError(f"Device returned {len(registers)} registers; expected 2" )
    finally:
        await connection.close()


class ModbusPressureSensorConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    VERSION = 1

    async def async_step_user(
            self, user_input: dict[str, Any] | None = None
    ) -> ConfigFlowResult:
        errors: dict[str, str] = {}
        if user_input is not None:
            conf_data = {
                **user_input,
                CONF_PORT: int(user_input[CONF_PORT]),
                CONF_UNIT_ID: int(user_input[CONF_UNIT_ID]),
            }

            try:
                await _test_connection(conf_data)
            except (ModbusError, OSError, TimeoutError):
                errors["base"] = "cannot_connect"
            else:
                await self.async_set_unique_id(
                    f"{user_input[CONF_HOST]}:{user_input[CONF_PORT]}:"
                    f"{user_input[CONF_UNIT_ID]}"
                )
                self._abort_if_unique_id_configured()
                return self.async_create_entry(
                    title="Modbus Pressure Sensor",
                    data=user_input,
                    options={CONF_SCAN_INTERVAL: DEFAULT_SCAN_INTERVAL},
                )

        return self.async_show_form(
            step_id="user", data_schema=USER_SCHEMA, errors=errors
        )

    @staticmethod
    def async_get_options_flow(config_entry: config_entries.ConfigEntry):
        return ModbusPressureSensorOptionsFlow()


class ModbusPressureSensorOptionsFlow(OptionsFlowWithReload):
    async def async_step_init(
            self, user_input: dict[str, Any] | None = None
    ) -> ConfigFlowResult:
        if user_input is not None:
            return self.async_create_entry(data=user_input)

        current = self.config_entry.options.get(
            CONF_SCAN_INTERVAL,
            self.config_entry.data.get(CONF_SCAN_INTERVAL, DEFAULT_SCAN_INTERVAL),
        )
        schema = self.add_suggested_values_to_schema(
            OPTIONS_SCHEMA, {CONF_SCAN_INTERVAL: current}
        )
        return self.async_show_form(step_id="init", data_schema=schema)
