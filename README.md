# Graphics Engine

An interactive C++20/OpenGL renderer with editable materials, post-processing,
scene save/load, repeatable GPU benchmarks, OBJ/glTF model importing, and a
world-coordinate grid.

## Build

Requirements: Windows, MSVC with the C++ workload, CMake 3.31+, Ninja, and
[vcpkg](https://github.com/microsoft/vcpkg). Set `VCPKG_ROOT` before using the
checked-in presets.

```powershell
$env:VCPKG_ROOT = "C:\dev\vcpkg"
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev
```

For an optimized distributable ZIP:

```powershell
cmake --preset release
cmake --build --preset release --parallel
ctest --preset release
cpack --preset release
```

The package is written to `out/build/release/packages` and contains the
executable, assets, third-party DLLs, and the MSVC runtime libraries.

## Controls

- `W/A/S/D`, `Shift`, `Ctrl`: move the camera.
- Mouse and wheel: look around and change field of view.
- `F8`: open the texture, model, object movement, and lighting menu.
- `Tab` / `Shift+Tab`: select the next / previous scene object.
- `Ctrl+D`: duplicate the selected object with a small position offset.
- `Delete`: remove the selected object.
- `Ctrl+Z` / `Ctrl+Y`: undo / redo scene edits without moving the camera.
- `G`: toggle the coordinate grid.
- `1`-`6`: select a post-processing effect.
- `F1`-`F4`, `F6`: select a material debug view.
- `F5`: reload shaders.
- `F7`: toggle the repeatable GPU benchmark.
- `F9`: toggle GPU information.
- `F11` / `F12`: save / load the quick scene.
- `Esc`: close the menu or application.

The importer accepts `.obj`, `.gltf`, and `.glb`. External material textures
inside `assets` are supported; embedded image textures currently use the
material color fallback.
