# CuraEngine_plugin_layered_infill
This Engine plugin extends the current infill patterns in CURA with  your own infill patters provided in via `*.wtk` files

- The `*.wtk` files can be placed in any directory, and must be set Cura in the `Layered Infill Directory` setting, if used by commandline, this parameter is called `infill_directory`.
- Each layer can have its own pattern by providing several `*.wtk` files with the corresponding `<layer_height>` in µm they are applied. 
- The naming pattern of the files is `<layer_height>_layered_infill` E.g. `1000_layered_infill.wkt` for the layer at `z = 1.0 mm`.
- Each file need to consists of a (consistent) rectangular bounding box as first polygon. E.g. `POLYGON ((-33758 -21651, 61856 -21651, 61856 52508, -33758 52508, -33758 -21651))`.
- The center of the bounding box is placed at the provided `Infill Center X` and `Infill Center Y` in Cura, if used by commandline, these parameters are called `center_x` and `center_y`. If you move the object in Cura do not forget to adjust these values! An automatic adjustment of these values is currently not possible.

This plugin is based on the [CuraEngine_plugin_infill_generate](https://github.com/Ultimaker/CuraEngine_plugin_infill_generate) provided as a template these kind of plugins from Ultimaker

This (template) plugin consist of the following licenses:

- LGPLv3 front end Cura plugin.
- BSD-4 C++ Business logic headers for the CuraEngine plugin logic
- AGPLv3 C++ Infill generation header

System Requirements
Windows
    Python 3.10 or higher
    Ninja 1.10 or higher
    VS2022 & < 17.10 (17.3.3 works)
    CMake 3.23 or higher
    nmake


## Installation

1. Configure Conan
   Before you start, if you use conan for other (big) projects as well, it's a good idea to either switch conan-home and/or backup your existing conan configuration(s).

That said, installing our config goes as follows:
```bash
pip install conan==1.65
conan config install https://github.com/ultimaker/conan-config.git
conan profile new default --detect --force
```
2. Clone CuraEngine_plugin_layered_infill
```bash
git clone https://github.com/EmJay276/CuraEngine_plugin_layered_infill CuraEngine_plugin_layered_infill
cd CuraEngine_plugin_layered_infill
```

3. Install & Build CuraEngine (Release)
```bash
conan install . --build=missing --update -s build_type=Release
```

4. Build executable
```bash
conan build .
```

[For more info](https://github.com/Ultimaker/CuraEngine/wiki/Building-CuraEngine-From-Source)
