# Flappy Bird OpenGL Game

A 3D Flappy Bird clone developed in C++ using OpenGL 3.3. This project demonstrates basic game engine concepts including 3D rendering, model loading (GLTF), collision detection, and UI systems.

## Features

*   **3D Graphics:** Built with modern OpenGL (Core Profile).
*   **Model Loading:** Custom GLTF loader to import 3D assets (Bird model).
*   **Text Rendering:** Uses `stb_truetype` for rendering score and UI text.
*   **UI System:** Interactive buttons for Start and Restart.
*   **Game Loop:** Physics-based movement, pipe generation, and collision detection.

## Controls

*   **Spacebar:** Jump / Flap wings.
*   **Mouse Click:** Interact with "Start" and "Restart" buttons.
*   **R Key:** Restart the game (when on the Game Over screen).

## Dependencies

The project uses the following libraries (included in `Libraries/`):
*   **GLFW:** Window management and input.
*   **GLAD:** OpenGL function pointer loading.
*   **GLM:** Mathematics library for graphics software.
*   **stb_image & stb_truetype:** Image loading and font rendering.
*   **nlohmann/json:** JSON parsing for GLTF files.

## Build Instructions

This project is configured for MinGW (g++) on Windows.

To build the project, run the following command in the terminal:

```bash
g++ -fdiagnostics-color=always -g "main.cpp" "Libraries/src/glad.c" -o "main.exe" "-ILibraries/include" "-LLibraries/lib" -lglfw3dll -lopengl32 -lgdi32 -luser32 -lkernel32
```

Ensure `glfw3.dll` is in the same directory as the executable.

## Assets

*   Bird Model: GLTF format.
*   Textures: PNG format for background and pipes.
*   Font: Arial (loaded from system fonts).
