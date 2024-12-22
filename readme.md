# CSCI420 Programming Assignment 1: Height Fields Using Shaders

**Assignment Link:** [Height Fields Using Shaders](https://odedstein.com/teaching/hs-2024-csci-420/assign1/)

**Demo Video:** [Youtube Link](https://youtu.be/-l9-UUwLa6g)

**Final Grade:** 110 out of 100

## Introduction

This program renders a height field using OpenGL shaders, allowing interactive visualization and manipulation of terrain data derived from heightmap images. It supports various rendering modes, smooth shading, and user controls for transformation and camera adjustments.

## Features

- **Multiple Rendering Modes:**
  - Points
  - Lines
  - Solid Triangles
  - Smoothed Height Field
  - Wireframe Overlay

- **Interactive Transformations:**
  - Rotation
  - Translation
  - Scaling

- **Camera Controls:**
  - Zoom In and Zoom Out
  - Auto-Rotate Feature

- **Color Options:**
  - Grayscale based on heightmap
  - Custom coloring using an external color image

- **Screenshot Functionality:**
  - Take a single screenshot
  - Capture a sequence of 300 screenshots over 20 seconds

## Controls

### Keyboard Commands

- **Rendering Modes:**
  - Press **`1`**: Render the height field as **points**.
  - Press **`2`**: Render the height field as **lines**.
  - Press **`3`**: Render the height field as **solid triangles**.
  - Press **`4`**: Render a **smoothed height field**.
  - Press **`5`**: Render **wireframe overlay** on top of solid triangles.

- **Scaling and Elevation:**
  - Press **`+`** or **`=`**: Increase scale, making the terrain **taller**.
  - Press **`-`** or **`_`**: Decrease scale, making the terrain **flatter**.
  - Press **`9`**: Increase exponent, enhancing **elevation contrast**.
  - Press **`0`**: Decrease exponent, reducing elevation contrast.

- **Camera Controls:**
  - Press **`o`**: **Zoom in** the camera.
  - Press **`p`**: **Zoom out** the camera.
  - Press **`r`**: Toggle **auto-rotation** of the terrain.

- **Coloring:**
  - Press **`c`**: Toggle **color mode** using an external color image.

- **Screenshot Functions:**
  - Press **`x`**: Take a **screenshot** and save it as `screenshot.jpg`.
  - Press **`s`**: Start capturing **300 screenshots** over **20 seconds**.
    - Screenshots are saved in the `screenshot` directory, named from `000.jpg` to `299.jpg`.

### Mouse Controls

- **Rotation:**
  - **Left Mouse Button**: Rotate about the **X** and **Y** axes.
  - **Middle Mouse Button**: Rotate about the **Z** axis.

- **Scaling:**
  - **Left Mouse Button** + **`SHIFT`**: Scale along the **X** and **Y** axes.
  - **Middle Mouse Button** + **`SHIFT`**: Scale along the **Z** axis.

- **Translation:**
  - **Left Mouse Button** + **`t`** key: Translate along the **X** and **Y** axes.
  - **Middle Mouse Button** + **`t`** key: Translate along the **Z** axis.
  - *(Note: The `CTRL` key may not work as expected on macOS; thus, the `t` key is used for translation.)*

## Usage

To run the program, use the following command:
```bash
./hw1 <heightmap image file>
```

Examples:
```bash
./hw1 heightmap/spiral.jpg
./hw1 heightmap/GrandTeton-128.jpg
./hw1 heightmap/GrandTeton-256.jpg
./hw1 heightmap/OhioPyle-256.jpg
./hw1 heightmap/OhioPyle-512.jpg
```

## Additional Implementations

- **Element Arrays with `glDrawElements`:**
  - Used for rendering lines, triangles, and smoothed height fields efficiently.
  - Implemented custom `ebo.h` and `ebo.cpp` helper classes.

- **Coloring with External Image:**
  - Vertices are colored based on an external image.
  - The color image does not need to match the heightmap's dimensions; rescaling is handled.
  - Still support the height-based coloring after applying colors from extermal image.

- **Wireframe Overlay:**
  - Renders a wireframe overlay on top of solid triangles.
  - Utilizes `glPolygonOffset` to avoid z-buffer fighting.

- **Camera Zooming:**
  - Implemented zoom in and zoom out functionality for the camera.
  - Adjusted camera position to allow closer or farther viewing of the terrain.

- **Automated Screenshot Capture:**
  - Captures 300 screenshots over 20 seconds.
  - Useful for creating animations or analyzing terrain over time.
  - Screenshots are saved in the `screenshot` directory, named from `000.jpg` to `299.jpg`.

- **Automated Rotation:**
  - Automatically rotate the terrain after press 'r'.
  - Able to turn off auto rotate when press again. 

## Sample Images
![mode 1 with color](<hw1/sample images/screenshot2.jpg>)
![mode 3 with color](<hw1/sample images/screenshot1.jpg>)