# CuraEngine_plugin_infill_generate
This Engine plugin extends the current infill patterns in CURA with:

- Continuous Honeycomb
- Normal Honey Comb
- Cura
- Honeycomb Fill
- ... You can create your own infill tills by adding `*.wtk` files in the `CuraEngineTiledInfill/tiles/` folder even when you already installed it in Cura...

NOTE: Please note that this plugin is Experimental and adding custom infills is not possible at the moment.
This plugin consist of the following licenses:

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
pip install conan==1.60
conan config install https://github.com/ultimaker/conan-config.git
conan profile new default --detect --force
```
2. Clone CuraEngine_plugin_infill_generate
```bash
https://github.com/Ultimaker/CuraEngine_plugin_infill_generate.git
cd CuraEngine_plugin_infill_generate
```

3. Install & Build CuraEngine (Release)
```bash
conan install . --build=missing --update -s build_type=Release
```

4. Build files
```bash
conan build .
```

[For more info](https://github.com/Ultimaker/CuraEngine/wiki/Building-CuraEngine-From-Source)
