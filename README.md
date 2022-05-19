# Real-time water simulation on GPU with OpenGL

<img src=preview.jpg>

[Demo video](https://drive.google.com/file/d/1ceq8epG3ap6ilOAaniNcN8ybwiKUQOhA/view?usp=sharing)

## Build

Required GPU with OpenGL support >=4.3

Linux (Ubuntu) package dependencies: `libglfw3`, `libglfw3-dev`

Build with `./compile.sh`

## Code structure

* `./*.bmp` - image sources
* `main.cpp`, `common.cpp` - main variables, gui loop
* `textured_sphere`, `textured_wall` - underwater objects
* `water_surface` - rendering of water
* `shaders/` - vertex, fragment, compute shaders

## Features

* Surface water waves simulation
* Interactive water disturbing
* Reflection of "sky"
* Refraction of light through water, illumination of underwater surfaces
* A ball placed underwater, also affected by refraction
* Utilization of compute shaders for water surface simulation and refracted light on underwater objects

## Shortcuts

* Mouse control
* WASD movement
* Right click - disturb water surface
* Q - toggle wireframe water surface
* F1-F4 - teleport to preset scene
