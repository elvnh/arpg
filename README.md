# arpg
![](screenshot.png)

An action RPG focused on magic made in a custom game engine written in C99. It is currently under progress and far from complete.

## Building
For now, the game is only possible to compile on Linux. Windows builds are planned in the future.

If compiling with the preprocessor definition `DEBUG_BUILD=1`, hot reloading will be enabled for
both assets and game code.
This means that if recompiling, the game code will be reloaded and any changes made to source
files in the `src/game` directory will be visible immediately without restarting the application.
The same is true for assets: when an asset such as a shader or sprite is modified, the asset
will change immediately in game.

### Dependencies
- `glfw3`
- `GLEW`
- `X11`

### Compiling
```bash
mkdir build && cd build
cmake ..
cd ..
cmake --build build -j
```

### Running
```bash
./build/arpg
```

## Controls
Please be aware that the gameplay is still in a prototyping stage and any controls are subject to change.

- Aim with mouse
- Walk with, `w`, `a`, `s` and `d`
- Cast the currently selected spell with left click
- Press `i` to open the menu containing inventory, spellbook and equipment
- Hover over items in inventory to see their stat modifiers
- Click any item in inventory to equip it
- Click any equipped item to unequip it
- Click any spell in spellbook the make it the selected spell

### Debug controls
- Press `t` to open the debug overlay
- Press `o` to increase and `l` to decrease game speed
- Press `g` to enter single step mode, meaning the gameplay is paused
- To exit single step mode, either press `g` again or use the up arrow key
  to increase the game speed
- When in single step mode, press `k` to advance the game by a single frame
- Press `y` to detach the camera
- While in detached camera mode, press `y` to return to normal camera mode
- While in detached camera mode, press `shift`+`y` to reset the camera position and zoom level
