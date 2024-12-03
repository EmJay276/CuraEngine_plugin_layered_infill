# CuraEngine Layered Infill Plugin

This Engine plugin extends the current infill patterns in CURA with your own infill patterns provided via `*.wtk` files

- The `*.wtk` files can be placed in any directory, and must be set Cura in the `Layered Infill Directory` setting, if
  used by commandline, this parameter is called `infill_directory`.
- Each layer can have its own pattern by providing several `*.wtk` files with the corresponding `<layer_height>` in µm
  they are applied.
- The naming pattern of the files is `<layer_height>_layered_infill` E.g. `1000_layered_infill.wkt` for the layer
  at `z = 1.0 mm`.
- Each file need to consists of a (consistent) rectangular bounding box as first polygon.
  E.g. `POLYGON ((-33758 -21651, 61856 -21651, 61856 52508, -33758 52508, -33758 -21651))`.
- The center of the bounding box is placed at the provided `Infill Center X` and `Infill Center Y` in Cura, if used by
  commandline, these parameters are called `center_x` and `center_y`. If you move the object in Cura do not forget to
  adjust these values! An automatic adjustment of these values is currently not possible.
- **`Infill Center X`, `Infill Center Y` and `Layered Infill Directory` must be set visible manually!** Just search for
  them, right-click on the name and select **Keep this setting visible**
- If no `*.wtk` file is found for one layer the closest one above is used. If no layer above is available, the closest
  one below is used.

This plugin is based on
the [CuraEngine_plugin_infill_generate](https://github.com/Ultimaker/CuraEngine_plugin_infill_generate) provided as a
template for these kind of plugins from Ultimaker

This (template) plugin consist of the following licenses:

- LGPLv3 front end Cura plugin.
- BSD-4 C++ Business logic headers for the CuraEngine plugin logic
- AGPLv3 C++ Infill generation header

## Example

<img src="/images/0_layer.png" width="200"> <img src="/images/1_layer.png" width="200"> <img src="/images/2_layer.png" width="200"> <img src="/images/3_layer.png" width="200"> <img src="/images/4_layer.png" width="200">

## Dependencies

- Cura 5.9.0 or newer

## Installation

### Cura Marketplace

Just install it from the PlugIn Marketplace

[CuraEngine Layered Infill Plugin](https://marketplace.ultimaker.com/app/cura/plugins/EmJay276/CuraEngineLayeredInfillPlugin)

### Building Manually

0. System Requirements

```
Windows
- Python 3.10 or higher
- Ninja 1.10 or higher
- VS2022 & < 17.10 (17.3.3 works)
- CMake 3.23 or higher
- nmake
```

1. Configure Conan

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

### Acknowledgement

The presented research is funded by the Deutsche Forschungsgemeinschaft (DFG, German Research Foundation) – Project No.
WA 2913/53-1 with the title "KNOTEN – Design method for the holistic optimisation of truss structures with additively
manufactured nodes while taking manufacturing and assembly restrictions into account". The authors thank the German
Research Foundation for the financial support.