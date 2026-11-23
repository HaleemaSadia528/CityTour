3D Car Racing Game
A fast-paced, retro 3D highway racing game built with C++ and the Raylib library. Avoid oncoming traffic, weave between lanes, and score bonus points for close calls while aiming for the highest score!📋 
Features
3D Gameplay & Rendering: Utilizes Raylib's 3D engine to render player and obstacle car models with dynamic camera movement.
Dynamic Obstacle Spawning: Features procedural obstacle car placement with color variation and predefined lane patterns.
Score System & Persistence: Scores points for overtaking cars with extra bonus points for near-misses. High scores are automatically saved to highscore.txt.
Custom UI & Screens: Simple menu system including:Main MenuHighway Rush Mode (Main Game)Game Over Screen with Score SummariesHigh Scores DisplayPlaceholder modes for future extension (Second Mode, Road Trip)
🛠️ Prerequisites & RequirementsBefore building the project, ensure you have the following installed:
C++ Compiler: GCC (MinGW for Windows) or any compatible C++11 standard compiler.
Raylib: Version 4.0 or higher.Assets: Ensure the path to the required .png and .glb files match the directories in the source code or place them in your project directory:Background image: backgroundbutton.pngPlayer model: vehicle_-_ford_gt.glbObstacle model: 130.glb🎮 ControlsKey / InputActionA / Left ArrowMove car leftD / Right ArrowMove car rightUp ArrowMove car forwardDown ArrowMove car backwardLeft Mouse ClickInteract with Menu Buttons🚀 How to Build and RunUsing Command Line (MinGW / GCC)Clone or download the repository:git clone https://github.com/your-username/3d-car-racing-game.git
cd 3d-car-racing-game
Compile the program:g++ main.cpp -o CarGame.exe -lraylib -lopengl32 -lgdi32 -lwinmm
Run the executable:./CarGame.exe
Note: Update file paths in main.cpp (e.g., LoadModel(...) and LoadImage(...)) to relative paths (such as "assets/vehicle_-_ford_gt.glb") if you move assets relative to your executable!
📂 Project Structure├── main.cpp              # Core game loop, logic, and rendering
├── highscore.txt         # Auto-generated file storing the highest score
├── README.md             # Project documentation
└── assets/               # Folder for models and textures (recommended)
    ├── backgroundbutton.png
    ├── vehicle_-_ford_gt.glb
    └── 130.glb
