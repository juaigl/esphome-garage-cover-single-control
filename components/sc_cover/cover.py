import esphome.codegen as cg
from esphome.components import binary_sensor, button, cover
import esphome.config_validation as cv
from esphome.const import (
    CONF_CLOSE_DURATION,
    CONF_CLOSE_ENDSTOP,
    CONF_ID,
    CONF_OPEN_DURATION,
    CONF_OPEN_ENDSTOP,
)

CONF_DOOR_ACTIVATE_BUTTON = "door_activate_button"
CONF_BUTTON_PRESS_INTERVAL = "button_press_interval"

sc_cover_ns = cg.esphome_ns.namespace("sc_cover")
SingleControlCover = sc_cover_ns.class_("SingleControlCover", cover.Cover, cg.Component)

CONFIG_SCHEMA = (
    cover.cover_schema(SingleControlCover)
    .extend(
        {
            cv.Required(CONF_DOOR_ACTIVATE_BUTTON): cv.use_id(button.Button),
            cv.Required(
                CONF_BUTTON_PRESS_INTERVAL
            ): cv.positive_time_period_milliseconds,
            cv.Required(CONF_OPEN_ENDSTOP): cv.use_id(binary_sensor.BinarySensor),
            cv.Required(CONF_OPEN_DURATION): cv.positive_time_period_milliseconds,
            cv.Required(CONF_CLOSE_ENDSTOP): cv.use_id(binary_sensor.BinarySensor),
            cv.Required(CONF_CLOSE_DURATION): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    bin = await cg.get_variable(config[CONF_DOOR_ACTIVATE_BUTTON])
    cg.add(var.set_door_activate_button(bin))
    cg.add(var.set_button_press_interval(config[CONF_BUTTON_PRESS_INTERVAL]))

    bin = await cg.get_variable(config[CONF_OPEN_ENDSTOP])
    cg.add(var.set_open_endstop(bin))
    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))

    bin = await cg.get_variable(config[CONF_CLOSE_ENDSTOP])
    cg.add(var.set_close_endstop(bin))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
