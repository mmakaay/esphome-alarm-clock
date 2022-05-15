import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome import pins
from esphome.components import vs10xx_base
from esphome.const import CONF_ID

CODEOWNERS = ["@mmakaay"]
AUTO_LOAD = ["vs10xx_base"]

vs1003_ns = cg.esphome_ns.namespace("vs1003")
VS1003Component = vs1003_ns.class_("VS1003Component", vs10xx_base.VS10XXBase)

CONFIG_SCHEMA = vs10xx_base.VS10XX_CONFIG_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(VS1003Component),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await vs10xx_base.register_vs10xx_component(var, config)
