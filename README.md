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

- The editor toolbar and Scene object rows are clickable; the 3D camera and wheel only react inside the central viewport.
- Toolbar actions: create a cube, quick-save/load the scene, toggle the grid, open Assets, or start the GPU benchmark.
- Inspector quick-edit buttons move, rotate, scale, snap, or reset the selected object and participate in undo/redo history.
- Scene buttons duplicate or delete the selected object with the same undo/redo behavior as the keyboard shortcuts.
- Scene and Inspector can collapse into narrow rails; their state is restored on the next launch.
- Buttons and Scene rows show hover feedback, and Assets menu entries can be opened with the mouse or keyboard.
- Hold the right mouse button and use the mouse / `W/A/S/D`, `Shift`, `Ctrl`: look and move the editor camera.
- `Alt+Enter`: toggle between the remembered windowed layout and fullscreen.
- Window position is preserved across monitors; fullscreen uses the display containing the window.
- `V`: toggle VSync; GPU benchmark mode temporarily disables it.
- Mouse and wheel: look around and change field of view.
- Left mouse button: select the nearest object under the cursor inside the viewport.
- Drag the colored `X`, `Y`, or `Z` gizmo handle to move the selected object with one undoable history transaction; hold `Ctrl` to snap the active axis to the `0.5` grid, or press `Esc` / RMB to cancel and restore the starting position.
- `Insert` / `C`: create and select a cube on the nearest free grid position.
- `F`: frame the selected object in the camera view.
- Arrow keys and `PageUp` / `PageDown`: move the selected object on `X/Z` and `Y`.
- `Q` / `E` and `-` / `=`: rotate around `Y` and scale the selected object.
- `End` / `Home`: snap the selected position/rotation to editor steps or reset the transform.
- `F8`: open the texture, model, object movement, and lighting menu.
- `Tab` / `Shift+Tab`: select the next / previous scene object.
- `Ctrl+D`: duplicate the selected object with a small position offset.
- `Delete` / `X`: remove the selected object.
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
