import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, cover, switch
from esphome.const import (
    CONF_CLOSE_DURATION,
    CONF_CLOSE_ENDSTOP,
    CONF_ID,
    CONF_OPEN_DURATION,
    CONF_OPEN_ENDSTOP,
)

CONF_DOOR_SWITCH = "door_switch"
CONF_SWITCH_INTERVAL = "switch_interval"

sc_cover_ns = cg.esphome_ns.namespace("sc_cover")
SingleControlCover = sc_cover_ns.class_("SingleControlCover", cover.Cover, cg.Component)

CONFIG_SCHEMA = (
    cover.cover_schema(SingleControlCover)
    .extend(
        {
            cv.Required(CONF_DOOR_SWITCH): cv.use_id(switch.Switch),
            cv.Required(CONF_SWITCH_INTERVAL): cv.positive_time_period_milliseconds,
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

    bin = await cg.get_variable(config[CONF_DOOR_SWITCH])
    cg.add(var.set_door_switch(bin))
    cg.add(var.set_switch_interval(config[CONF_SWITCH_INTERVAL]))

    bin = await cg.get_variable(config[CONF_OPEN_ENDSTOP])
    cg.add(var.set_open_endstop(bin))
    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))

    bin = await cg.get_variable(config[CONF_CLOSE_ENDSTOP])
    cg.add(var.set_close_endstop(bin))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
