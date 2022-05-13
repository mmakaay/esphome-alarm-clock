import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome import pins
from esphome.components import spi
from esphome.const import (
    CONF_ID,
)

CONF_SPI_FAST_ID = "spi_fast_id"
CONF_SPI_SLOW_ID = "spi_slow_id"
CONF_DREQ_PIN = "dreq_pin"
CONF_XCS_PIN = "xcs_pin"
CONF_XDCS_PIN = "xdcs_pin"

CODEOWNERS = ["@mmakaay"]
DEPENDENCIES = ["spi"]

vs1053_ns = cg.esphome_ns.namespace("vs1053")
VS1053Component = vs1053_ns.class_("VS1053Component", cg.Component, spi.SPIDevice)
#VS1053SlowSPI = vs1053_ns.class_("VS1053SlowSPI", spi.SPIDevice)
#VS1053FastSPI = vs1053_ns.class_("VS1053FastSPI", spi.SPIDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(VS1053Component),
            #cv.GenerateID(CONF_SPI_SLOW_ID): cv.declare_id(VS1053SlowSPI),
            #cv.GenerateID(CONF_SPI_FAST_ID): cv.declare_id(VS1053FastSPI),
            cv.Required(CONF_DREQ_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_XDCS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_XCS_PIN): pins.gpio_output_pin_schema,
        }
    )  
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(False))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    #spi_slow = cg.new_Pvariable(config[CONF_SPI_SLOW_ID])
    #spi_fast = cg.new_Pvariable(config[CONF_SPI_FAST_ID])
    #await spi.register_spi_device(spi_slow, config)
    #await spi.register_spi_device(spi_fast, config)
    await spi.register_spi_device(var, config)
    dreq_pin = await cg.gpio_pin_expression(config[CONF_DREQ_PIN])
    xdcs_pin = await cg.gpio_pin_expression(config[CONF_XDCS_PIN])
    xcs_pin = await cg.gpio_pin_expression(config[CONF_XCS_PIN])
    #cg.add(var.set_slow_spi(spi_slow))
    #cg.add(var.set_fast_spi(spi_fast))
    cg.add(var.set_dreq_pin(dreq_pin))
    cg.add(var.set_xcs_pin(xcs_pin))
    cg.add(var.set_xdcs_pin(xdcs_pin))

