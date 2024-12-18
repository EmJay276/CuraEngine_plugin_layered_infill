# Copyright (c) 2024 Michael Jaeger, Marie Schmid
# CuraEngineLayeredInfill is released under the terms of the LGPLv3 or higher.

import os
import platform
import stat
import sys
from pathlib import Path

from UM.Logger import Logger
from UM.Settings.ContainerRegistry import ContainerRegistry
from UM.Settings.DefinitionContainer import DefinitionContainer
from UM.i18n import i18nCatalog
from cura.BackendPlugin import BackendPlugin

catalog = i18nCatalog("cura")


class CuraEngineLayeredInfill(BackendPlugin):
    def __init__(self):
        super().__init__()
        self.definition_file_paths = [Path(__file__).parent.joinpath("infill_settings.def.json").as_posix()]
        if not self.isDebug():
            if not self.binaryPath().exists():
                Logger.error(f"Could not find CuraEngineLayeredInfill binary at {self.binaryPath().as_posix()}")
            if platform.system() != "Windows" and self.binaryPath().exists():
                st = os.stat(self.binaryPath())
                os.chmod(self.binaryPath(), st.st_mode | stat.S_IEXEC)
            self._plugin_command = [self.binaryPath().as_posix(), "--tiles_path", Path(__file__).parent.as_posix()]

        self._supported_slots = [200]  # ModifyPostprocess SlotID
        ContainerRegistry.getInstance().containerLoadComplete.connect(self._on_container_load_complete)

    def _on_container_load_complete(self, container_id) -> None:
        if not ContainerRegistry.getInstance().isLoaded(container_id):
            # skip containers that could not be loaded, or subsequent findContainers() will cause an infinite loop
            return
        try:
            container = ContainerRegistry.getInstance().findContainers(id=container_id)[0]
        except IndexError:
            # the container no longer exists
            return
        if not isinstance(container, DefinitionContainer):
            # skip containers that are not definitions
            return
        if container.getMetaDataEntry("type") == "extruder":
            # skip extruder definitions
            return

        for definition in container.findDefinitions(key="infill_pattern"):
            definition.extend_category('layered_infill', 'Layered Infill', plugin_id=self.getPluginId(),
                                       plugin_version=self.getVersion())

    def getPort(self):
        return super().getPort() if not self.isDebug() else int(os.environ["CURAENGINE_INFILL_GENERATE_PORT"])

    def isDebug(self):
        return not hasattr(sys, "frozen") and os.environ.get("CURAENGINE_INFILL_GENERATE_PORT", None) is not None

    def start(self):
        if not self.isDebug():
            super().start()

    def binaryPath(self) -> Path:
        ext = ".exe" if platform.system() == "Windows" else ""

        machine = platform.machine()
        if machine == "AMD64":
            machine = "x86_64"
        return Path(__file__).parent.joinpath(machine, platform.system(), f"curaengine_plugin_layered_infill{ext}").resolve()
