# Graphics Engine

An interactive C++20/OpenGL renderer with editable materials, post-processing,
scene save/load, repeatable GPU benchmarks, OBJ/glTF model importing, and a
world-coordinate grid. Its windowed editor uses a professional graphite shell
with a menu bar, workspace toolbar, dock-style panels, and a dedicated viewport
header. The Content Browser opens as a responsive bottom dock, keeping the
scene visible while assets and editor tools are browsed.

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
- `File`, `Edit`, `View`, and `Window` menus expose scene, history, display, content-browser, and panel commands with shortcuts and live enabled/checked states.
- Toolbar actions: create a cube, quick-save/load the scene, toggle the grid, open the Content Browser, or start the GPU benchmark.
- The viewport toolbar cycles material debug views and post-processing effects, switches between `Move` and `Rotate`, and can keep snapping enabled without holding `Ctrl`.
- The interface uses antialiased platform TrueType text when available and automatically falls back to its embedded bitmap font.
- Inspector quick-edit buttons move, rotate, scale, snap, or reset the selected object and participate in undo/redo history.
- Scene buttons duplicate or delete the selected object with the same undo/redo behavior as the keyboard shortcuts.
- Scene and Inspector can collapse into narrow rails; their state is restored on the next launch.
- Drag the separators beside Scene or Inspector to resize the panels; their widths are restored on the next launch.
- Buttons and Scene rows show hover feedback, and Content Browser entries can be opened with the mouse or keyboard; long lists follow the selection and show their visible range.
- The title and status bar show `*` / `UNSAVED` after scene edits and return to a saved state after save, load, or undoing back to the saved content.
- Hold the right mouse button and use the mouse / `W/A/S/D`, `Shift`, `Ctrl`: look and move the editor camera.
- `Alt+Enter`: toggle between the remembered windowed layout and fullscreen.
- Window position is preserved across monitors; fullscreen uses the display containing the window.
- `V`: toggle VSync; GPU benchmark mode temporarily disables it.
- Mouse and wheel: look around and change field of view.
- Left mouse button: select the nearest object under the cursor inside the viewport.
- Drag a colored `X`, `Y`, or `Z` move handle or rotation ring to transform the selected object with one undoable history transaction; hold `Ctrl` to snap movement to `0.5` units or rotation to `15` degrees, or press `Esc` / RMB to cancel and restore the starting transform.
- `Insert` / `C`: create and select a cube on the nearest free grid position.
- `F`: frame the selected object in the camera view.
- Arrow keys and `PageUp` / `PageDown`: move the selected object on `X/Z` and `Y`.
- `W` / `R`: select the Move / Rotate gizmo; `Q` / `E` still provide quick rotation around `Y`.
- `-` / `=`: scale the selected object.
- `End` / `Home`: snap the selected position/rotation to editor steps or reset the transform.
- `F8`: toggle the docked Content Browser for textures, models, object movement, and lighting tools.
- `Tab` / `Shift+Tab`: select the next / previous scene object.
- `Ctrl+D`: duplicate the selected object with a small position offset.
- `Delete` / `X`: remove the selected object.
- `Ctrl+Z` / `Ctrl+Y`: undo / redo scene edits without moving the camera.
- `Ctrl+S` or `F11`: save the quick scene; `F12`: load it.
- Quick-load is blocked while the current scene is unsaved; save it or undo back to the saved state first.
- `G`: toggle the coordinate grid.
- `1`-`6`: select a post-processing effect.
- `F1`-`F4`, `F6`: select a material debug view.
- `F5`: reload shaders.
- `F7`: toggle the repeatable GPU benchmark.
- `F9`: toggle GPU information.
- `Esc`: close the menu or request application exit.
- Closing an edited scene with `Esc`, `Alt+F4`, or the title-bar `X` opens a keyboard-and-mouse confirmation: `Enter` / `Ctrl+S` saves and exits, `D` discards and exits, and `Esc` cancels.

The importer accepts `.obj`, `.gltf`, and `.glb`. External material textures
inside `assets` are supported; embedded image textures currently use the
material color fallback.
