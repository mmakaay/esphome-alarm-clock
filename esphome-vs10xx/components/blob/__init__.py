import os
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.core import CORE, HexInt
from esphome.const import CONF_ID, CONF_FILE, CONF_RAW_DATA_ID


CODEOWNERS = ["@mmakaay"]
MULTI_CONF = True

blob_ns = cg.esphome_ns.namespace("blob")
Blob = blob_ns.class_("Blob")


def validate_file(value):
    path = CORE.relative_config_path(value);
    if not os.path.exists(path):
        raise cv.Invalid("File does not exist")
    try:
        with (open(path, "r")):
            pass
    except OSError as err:
      raise cv.Invalid(f"File cannot be read: {str(err)}"); 
    return path


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Blob),
        cv.GenerateID(CONF_RAW_DATA_ID): cv.declare_id(cg.uint8),
        cv.Required(CONF_FILE): cv.All(cv.string, validate_file),
    }
)


async def to_code(config):
    path = CORE.relative_config_path(config[CONF_FILE])  
    with open(path, "rb") as fh:
        rhs = list(map(HexInt, fh.read()))
    prog_arr = cg.progmem_array(config[CONF_RAW_DATA_ID], rhs)
    cg.new_Pvariable(config[CONF_ID], prog_arr, len(rhs))
