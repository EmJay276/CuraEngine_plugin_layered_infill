# Copyright (c) 2024 Michael Jaeger, Marie Schmid
# CuraEngineLayeredInfill is released under the terms of the LGPLv3 or higher.

import platform

from UM.i18n import i18nCatalog

catalog = i18nCatalog("curaengine_layered_infill")

from . import constants

if platform.machine() in ["AMD64", "x86_64"] or (platform.machine() in ["arm64"] and platform.system() == "Darwin"):
    from . import CuraEngineLayeredInfill

    def getMetaData():
        return {}

    def register(app):
        return {"backend_plugin": CuraEngineLayeredInfill.CuraEngineLayeredInfill()}
else:
    from UM.Logger import Logger

    Logger.error(f"{constants.cura_plugin_name} plugin is only supported on x86_64 systems for Windows and Linux and x86_64/arm64 for macOS.")

    def getMetaData():
        return {}

    def register(app):
        return {}