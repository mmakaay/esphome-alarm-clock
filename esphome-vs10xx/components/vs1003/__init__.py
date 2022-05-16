import esphome.codegen as cg
from esphome.components.vs10xx_base import (
    VS10XXBase,
    VS10XXPlugin,
    vs10xx_device_schema,
    register_vs10xx_component,
)

CODEOWNERS = ["@mmakaay"]
AUTO_LOAD = ["vs10xx_base"]

vs1003_ns = cg.esphome_ns.namespace("vs1003")
VS1003Component = vs1003_ns.class_("VS1003Component", VS10XXBase)

plugins = {
    "dacmono": vs1003_ns.class_("VS1003PluginDacMono", VS10XXPlugin),
    "wavfix": vs1003_ns.class_("VS1003PluginWavFix", VS10XXPlugin),
    "8khzmp3fix", vs1003_ns.class_("VS1003Plugin8kHzMp3Fix", VS10XXPlugin),
    "wma_webcast_rw": vs1003_ns.class_("VS1003WMAWebcastRewind", VS10XXPlugin),
}

CONFIG_SCHEMA = vs10xx_device_schema(VS1003Component, plugins)

async def to_code(config):
    await register_vs10xx_component(config, plugins)
