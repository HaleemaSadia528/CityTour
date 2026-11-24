# 3D Car Racing Game

A fast-paced, retro 3D highway racing game built with C++ and the [Raylib](https://www.raylib.com/) library. Avoid oncoming traffic, weave between lanes, and score bonus points for close calls while aiming for the highest score!

## Features

* **3D Gameplay & Rendering:** Utilizes Raylib's 3D engine to render player and obstacle car models with dynamic camera movement.
* **Dynamic Obstacle Spawning:** Features procedural obstacle car placement with color variation and predefined lane patterns.
* **Score System & Persistence:** Scores points for overtaking cars with extra bonus points for near-misses. High scores are automatically saved to `highscore.txt`.
* **Custom UI & Screens:** Simple menu system including:
  * Main Menu
  * Highway Rush Mode (Main Game)
  * Game Over Screen with Score Summaries
  * High Scores Display
  * Placeholder modes for future extension (Second Mode, Road Trip)

---

##  Prerequisites & Requirements

Before building the project, ensure you have the following installed:

* **C++ Compiler:** GCC (MinGW for Windows) or any compatible C++11 standard compiler.
* **Raylib:** Version 4.0 or higher.
* **Assets:** Ensure the path to the required `.png` and `.glb` files match the directories in the source code or place them in your project directory:
  * Background image: `backgroundbutton.png`
  * Player model: `vehicle_-_ford_gt.glb`
  * Obstacle model: `130.glb`

---

##  Controls

| Key / Input | Action |
| :--- | :--- |
| **A** / **Left Arrow** | Move car left |
| **D** / **Right Arrow** | Move car right |
| **Up Arrow** / **W** | Move car forward |
| **Down Arrow** / **S** | Move car backward |
| **Left Mouse Click** | Interact with Menu Buttons |

---

## How to Build and Run

### Using Command Line (MinGW / GCC)

1. Clone or download the repository.
2. Compile the program:
   ```bash
   g++ main.cpp -o CarGame.exe -lraylib -lopengl32 -lgdi32 -lwinmm
