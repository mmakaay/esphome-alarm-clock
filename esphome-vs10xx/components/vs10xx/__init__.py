import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome import pins
from esphome.components import spi
from esphome.components import blob
from esphome.const import CONF_ID, CONF_RESET_PIN, CONF_TYPE, CONF_DELTA, CONF_DIRECTION

CONF_HAL_ID = "hal_id"
CONF_SPI_FAST_ID = "spi_fast_id"
CONF_SPI_SLOW_ID = "spi_slow_id"
CONF_DREQ_PIN = "dreq_pin"
CONF_XCS_PIN = "xcs_pin"
CONF_XDCS_PIN = "xdcs_pin"
CONF_PLUGINS = "plugins"
CONF_VOLUME = "volume"
CONF_LEFT = "left"
CONF_RIGHT = "right"
CONF_BLOB_ID = "blob_id"

CODEOWNERS = ["@mmakaay"]
DEPENDENCIES = ["spi"]

vs10xx_ns = cg.esphome_ns.namespace("vs10xx")

# Device classes
VS10XX = vs10xx_ns.class_("VS10XX", cg.Component)
VS10XXSlowSPI = vs10xx_ns.class_("VS10XXSlowSPI", cg.Component, spi.SPIDevice)
VS10XXFastSPI = vs10xx_ns.class_("VS10XXFastSPI", cg.Component, spi.SPIDevice)
VS10XXHAL = vs10xx_ns.class_("VS10XXHAL", cg.Component)
VS10XXHALChipset = vs10xx_ns.class_("VS10XXHALChipset")
VS1003Chipset = vs10xx_ns.class_("VS1003Chipset", VS10XXHALChipset)
VS1053Chipset = vs10xx_ns.class_("VS1053Chipset", VS10XXHALChipset)
VS10XXPlugin = vs10xx_ns.class_("VS10XXPlugin")

# Actions
ChangeVolumeAction = vs10xx_ns.class_(
    "ChangeVolumeAction", automation.Action, cg.Parented.template(VS10XX)
)
SetVolumeAction = vs10xx_ns.class_(
    "SetVolumeAction", automation.Action, cg.Parented.template(VS10XX)
)
PlayAction = vs10xx_ns.class_(
    "PlayAction", automation.Action, cg.Parented.template(VS10XX)
)
TurnOffOutputAction = vs10xx_ns.class_(
    "TurnOffOutputAction", automation.Action, cg.Parented.template(VS10XX)
)


# A mapping of known device types and their HAL ipmlementation classes.
TYPES = {
  "VS1003": VS1003Chipset,
  "VS1053": VS1053Chipset,
}

# Available plugins for the known device types.
def _plugin(className):
    return vs10xx_ns.class_(className, VS10XXPlugin)
PLUGINS = {
    "VS1003": {
        "DACMONO": _plugin("PluginVS1003DacMono"),
        "WAVFIX": _plugin("PluginVS1003WavFix"),
        "8KHZMP3fix": _plugin("PluginVS10038kHzMp3Fix"),
        "WMAREW4": _plugin("PluginVS1003WMAWebcastRewind"),
    },
    "VS1053": {},
}


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(VS10XX),
            cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
            cv.GenerateID(CONF_SPI_SLOW_ID): cv.declare_id(VS10XXSlowSPI),
            cv.GenerateID(CONF_SPI_FAST_ID): cv.declare_id(VS10XXFastSPI),
            cv.GenerateID(CONF_HAL_ID): cv.declare_id(VS10XXHAL),
            cv.Required(CONF_DREQ_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_XDCS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_XCS_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_PLUGINS): cv.ensure_list(cv.string),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(False))
)

def final_validate(config):
    valid_plugins = PLUGINS[config[CONF_TYPE]]
    for plugin in config[CONF_PLUGINS]:
        if plugin.upper() not in valid_plugins:
            raise cv.Invalid(f"Invalid plugin for type '{config[CONF_TYPE]}': {plugin} (valid plugins are: {', '.join(valid_plugins)})")


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    type_ = config[CONF_TYPE]
    plugins = PLUGINS[type_];
    cg.add_define(f"USE_{type_}");

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    chipset_class = TYPES[type_]
    chipset_id = cv.declare_id(chipset_class)(f"chipset_{type_}")
    chipset = cg.new_Pvariable(chipset_id)

    hal = cg.new_Pvariable(config[CONF_HAL_ID], chipset)
    await cg.register_component(hal, config)
    cg.add(var.set_hal(hal))

    spi_slow = cg.new_Pvariable(config[CONF_SPI_SLOW_ID])
    await spi.register_spi_device(spi_slow, config)
    cg.add(hal.set_slow_spi(spi_slow))

    spi_fast = cg.new_Pvariable(config[CONF_SPI_FAST_ID])
    await spi.register_spi_device(spi_fast, config)
    cg.add(hal.set_fast_spi(spi_fast))

    dreq_pin = await cg.gpio_pin_expression(config[CONF_DREQ_PIN])
    cg.add(hal.set_dreq_pin(dreq_pin))

    xcs_pin = await cg.gpio_pin_expression(config[CONF_XCS_PIN])
    cg.add(hal.set_xcs_pin(xcs_pin))

    xdcs_pin = await cg.gpio_pin_expression(config[CONF_XDCS_PIN])
    cg.add(hal.set_xdcs_pin(xdcs_pin))

    if CONF_RESET_PIN in config:
        reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(hal.set_reset_pin(reset_pin))

    if CONF_PLUGINS in config:
        for name in map(str.upper, config[CONF_PLUGINS]):
            plugin_class = plugins[name]
            plugin_id = cv.declare_id(plugin_class)(f"plugin_{name}")
            plugin = cg.new_Pvariable(plugin_id)
            cg.add(var.add_plugin(plugin))


@automation.register_action(
    "vs10xx.play",
    PlayAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(VS10XX),
            cv.GenerateID(CONF_BLOB_ID): cv.use_id(blob.Blob),
        },
        key=CONF_BLOB_ID,
    ),
)
async def vs10xx_play_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    blob_var = await cg.get_variable(config[CONF_BLOB_ID])
    cg.add(var.set_blob(blob_var))
    return var


@automation.register_action(
    "vs10xx.volume_up",
    ChangeVolumeAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(VS10XX),
            cv.Optional(CONF_DELTA, default=0.1): cv.percentage,
            cv.Optional(CONF_DIRECTION, default=+1): +1,
        },
        key=CONF_DELTA,
    ),
)
@automation.register_action(
    "vs10xx.volume_down",
    ChangeVolumeAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(VS10XX),
            cv.Optional(CONF_DELTA, default=0.1): cv.percentage,
            cv.Optional(CONF_DIRECTION, default=-1): -1
        },
        key=CONF_DELTA,
    ),
)
async def vs10xx_volume_up_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_delta(config[CONF_DIRECTION] * config[CONF_DELTA]))
    return var


@automation.register_action(
    "vs10xx.set_volume",
    SetVolumeAction,
    cv.Any(
        cv.maybe_simple_value(
            {
                cv.GenerateID(): cv.use_id(VS10XX),
                cv.Required(CONF_VOLUME): cv.templatable(cv.percentage),
            },
            key=CONF_VOLUME,
        ),
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(VS10XX),
                cv.Optional(CONF_LEFT): cv.templatable(cv.percentage),
                cv.Optional(CONF_RIGHT): cv.templatable(cv.percentage),
            }
        ),
    )
) 
async def vs10xx_set_volume_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    if CONF_VOLUME in config:
        vol = await cg.templatable(config[CONF_VOLUME], args, float)
        cg.add(var.set_left(vol))
        cg.add(var.set_right(vol))
    else:
        left_vol = await cg.templatable(config[CONF_LEFT], args, float)
        cg.add(var.set_left(left_vol))
        right_vol = await cg.templatable(config[CONF_RIGHT], args, float)
        cg.add(var.set_right(right_vol))
    return var


SIMPLE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(VS10XX),
    }
)

@automation.register_action("vs10xx.turn_off_output", TurnOffOutputAction, SIMPLE_SCHEMA)
async def vs10xx_set_volume_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


