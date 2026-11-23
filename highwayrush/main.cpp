#include <iostream>
#include <fstream>
#include <cmath>
#include <raylib.h>
#include <rlgl.h>

using namespace std;

// button 
bool DrawButton(float x, float y, float w, float h, const char* txt) {
    int mx = GetMouseX();
    int my = GetMouseY();
    bool hover = (mx >= x && mx <= (x + w) && my >= y && my <= (y + h));

    bool clicked = hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawRectangle((int)x, (int)y, (int)w, (int)h, hover ? LIGHTGRAY : DARKGRAY);
    DrawRectangleLines((int)x, (int)y, (int)w, (int)h, BLACK);
    DrawText(txt, (int)x + 20, (int)y + 15, 20, BLACK);

    return clicked;
}

int main(void) {
    InitWindow(1280, 720, "3D CAR RACING GAME");
    SetTargetFPS(60);

    Image bgImage = LoadImage("C:\\Users\\Maryam_Shahid\\Desktop\\highwayrush\\backgroundbutton.png");
    Texture2D bgTexture = LoadTextureFromImage(bgImage);
    UnloadImage(bgImage); 

    Model car = LoadModel("C:\\Users\\Maryam_Shahid\\Desktop\\highwayrush\\vehicle_-_ford_gt.glb");
    Vector3 carPosition = { 0.0f, 0.0f, 0.0f };
    Camera3D camera = { 0 };
    camera.position = { 0.0f, 4.0f, 6.0f };
    camera.target = carPosition;
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model obstacleCar = LoadModel("C:\\Users\\Maryam_Shahid\\Desktop\\highwayrush\\130.glb");

    // Screen states
    const int ScreenMenu       = 0;
    const int ScreenHighway    = 1;
    const int ScreenSecond     = 2;
    const int ScreenRoadTrip   = 3;
    const int ScreenHighscores = 4;
    const int ScreenGameOver   = 5;

    int currentScreen = ScreenMenu; // Start at main menu

    // Number of obstacles
    const int Obs = 10;
    Vector3 obsPositions[Obs] = { 0 };      // Positions of obstacles
    bool obsAct[Obs] = { false };          // Whether  obstacle is active
    Color obsColors[Obs] = { 0 };          // Color of obstacles
    bool obsPass[Obs] = { false };         // Whether obstacle  passed
    float Timer = 0.0f;                    // Timer for coming obstacles

    int score = 0;
    int highHighScore = 0;  // High score loaded 

    // Load saved high score 
    ifstream inFile("highscore.txt");
    if (inFile.is_open()) {
        inFile >> highHighScore;
        inFile.close();
    }

    // obstacle patterns (lane, distance, color)
    const int pLen = 24;
    int laneP[pLen] = {
        1, 0, 2, 2, 1, 0, 1, 2, 0, 0, 2, 1,
        2, 0, 1, 1, 2, 0, 2, 1, 0, 2, 1, 0
    };

    int distP[pLen] = {
        4, 1, 8, 3, 10, 2, 6, 0, 5, 9, 1, 7,
        3, 8, 0, 6, 2, 10, 4, 1, 9, 5, 7, 2
    };

    int colP[pLen] = {
        2, 5, 0, 3, 6, 1, 4, 2, 6, 0, 5, 3,
        1, 4, 6, 2, 0, 5, 3, 1, 4, 0, 2, 6
    };

    Color carColors[7] = { RED, GREEN, BLUE, YELLOW, ORANGE, PURPLE, MAGENTA };

    int nextP = 0; // Index to next obstacle pattern

    while (!WindowShouldClose()) {
        if (currentScreen == ScreenHighway) {
            if ((IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) && carPosition.x < 2.5f)  carPosition.x += 0.1f;
            if ((IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) && carPosition.x > -2.5f) carPosition.x -= 0.1f;
            if (IsKeyDown(KEY_UP))    carPosition.z -= 0.3f;
            if (IsKeyDown(KEY_DOWN))  carPosition.z += 0.3f;

            // Update camera
            camera.target = { carPosition.x, carPosition.y, carPosition.z - 2.0f };
            camera.position = { carPosition.x, carPosition.y + 3.5f, carPosition.z + 5.0f };

            Timer += 1.0f;

            // Come new obstacles 
            if (Timer > 50.0f) {
                Timer = 0.0f;

                // Find inactive slot to bring a new one
                for (int i = 0; i < Obs; i++) {
                    if (!obsAct[i]) {
                        int lane = laneP[nextP];
                        float laneX = 0.0f;
                        if (lane == 0) {
                            laneX = -2.0f;
                        } else if (lane == 1) {
                            laneX = 0.0f;
                        } else {
                            laneX = 2.0f;
                        }  

                        int distance = distP[nextP];

                        //  Coming car position is ahead of the car byfixed offset and pattern distance
                        obsPositions[i] = { laneX, 0.0f, carPosition.z - 80.0f - distance };

                        obsColors[i] = carColors[colP[nextP]];
                        obsAct[i] = true;
                        obsPass[i] = false;

                        nextP = (nextP + 1) % pLen;
                        break;
                    }
                }
            }

            // Update positions of active obstacles
            for (int i = 0; i < Obs; i++) {
                if (obsAct[i]) {

                    obsPositions[i].z += 0.9f;

                    // horizontal and depth distance to car
                    float xDiff = obsPositions[i].x - carPosition.x;
                    float xDist = (xDiff<0) ? -xDiff: xDiff;

                    float zDiff = obsPositions[i].z - carPosition.z;
                    float zDist = (zDiff<0) ? -zDiff: zDiff;

                    // Collision detection
                    if (xDist < 1.1f && zDist < 2.0f) {
                        // Save high score to file if beaten
                        if (score > highHighScore) {
                            highHighScore = score;
                            ofstream outFile("highscore.txt");
                            if (outFile.is_open()) {
                                outFile << highHighScore;
                                outFile.close();
                            }
                        }
                        // Switch to game over screen
                        currentScreen = ScreenGameOver;
                    }

                    // Award points if player has passed
                    if (!obsPass[i] && obsPositions[i].z > carPosition.z) {
                        float xDistance = ((obsPositions[i].x - carPosition.x)<0) ? -(obsPositions[i].x - carPosition.x): (obsPositions[i].x - carPosition.x);
                        if (xDistance < 3.5f) {
                            int earnedPoints = 100;
                            if (xDistance < 1.2f) {
                                earnedPoints += 50; // Bonus points for close pass
                            }
                            score += earnedPoints;
                        }
                        obsPass[i] = true;
                    }

                    // Deactivate obstacle if far behind
                    if (obsPositions[i].z > carPosition.z + 5.0f) {
                        obsAct[i] = false;
                        obsPass[i] = false;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(GRAY);

        switch (currentScreen) {

            case ScreenMenu: {

                if (bgTexture.id != 0) {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)bgTexture.width, (float)bgTexture.height };
                    Rectangle destRec = { 0.0f, 0.0f, 1280.0f, 720.0f };
                    Vector2 origin = { 0.0f, 0.0f };
                    DrawTexturePro(bgTexture, sourceRec, destRec, origin, 0.0f, WHITE);
                }

                DrawText("MAIN MENU", 560, 100, 30, WHITE);

                // Buttons for different menu options
                if (DrawButton(550, 200, 200, 40, "Highway Rush")) {
                    score = 0;
                    carPosition = { 0.0f, 0.0f, 0.0f };
                    for (int i = 0; i < Obs; i++) {
                        obsAct[i] = false;
                        obsPass[i] = false;
                    }
                    currentScreen = ScreenHighway;
                }
                if (DrawButton(550, 275, 200, 40, "Second Mode")) {
                    currentScreen = ScreenSecond;
                }
                if (DrawButton(550, 350, 200, 40, "Road Trip")) {
                    currentScreen = ScreenRoadTrip;
                }
                if (DrawButton(550, 425, 200, 40, "High Scores")) {
                    currentScreen = ScreenHighscores;
                }
                if (DrawButton(550, 500, 200, 40, "Quit")) {
                    // Save high score before quitting
                    if (score > highHighScore) {
                        highHighScore = score;
                        ofstream outFile("highscore.txt");
                        if (outFile.is_open()) {
                            outFile << highHighScore;
                            outFile.close();
                        }
                    }
                    UnloadModel(car);
                    UnloadModel(obstacleCar);
                    UnloadTexture(bgTexture);
                    CloseWindow();
                    return 0;
                }
                break;
            }

            case ScreenHighway: {
                BeginMode3D(camera);

                rlDisableBackfaceCulling();

                // Draw road and surroundings
                DrawCube({ 0.0f, -0.15f, carPosition.z }, 100.0f, 0.05f, 200.0f, DARKGREEN); // Grass
                DrawCube({ 0.0f, -0.1f, carPosition.z }, 12.0f, 0.1f, 200.0f, DARKGRAY);   // Road
                DrawCube({ 0.0f, 0.0f, carPosition.z }, 0.2f, 0.12f, 200.0f, YELLOW);     // Central lane line
                DrawCube({ -2.0f, 0.0f, carPosition.z }, 0.1f, 0.12f, 200.0f, WHITE);     // Left lane line
                DrawCube({ 2.0f, 0.0f, carPosition.z }, 0.1f, 0.12f, 200.0f, WHITE);      // Right lane line

                // Draw player car model
                DrawModelEx(car, carPosition, { 0.0f, 1.0f, 0.0f }, 180.0f, { 0.75f, 0.75f, 0.75f }, WHITE);

                // Draw all active obstacle cars
                for (int i = 0; i < Obs; i++) {
                    if (obsAct[i]) {
                        DrawModelEx(obstacleCar, obsPositions[i], { 0.0f, 1.0f, 0.0f }, 0.0f, { 1.25f, 1.25f, 1.25f }, obsColors[i]);
                    }
                }

                rlEnableBackfaceCulling(); 
                EndMode3D();


                DrawText("Highway Rush Mode", 50, 20, 20, RED);
                DrawText(TextFormat("Score: %d", score), 50, 50, 20, YELLOW);
                DrawText(TextFormat("High Score: %d", highHighScore), 50, 80, 20, GOLD);

                // Button to main menu
                if (DrawButton(50, 120, 150, 40, "Main Menu")) {
                    // Save high score 
                    if (score > highHighScore) {
                        highHighScore = score;
                        ofstream outFile("highscore.txt");
                        if (outFile.is_open()) {
                            outFile << highHighScore;
                            outFile.close();
                        }
                    }
                    currentScreen = ScreenMenu;
                }
                break;
            }

            case ScreenGameOver: {
                DrawText("GAME OVER", 520, 200, 40, RED);
                DrawText(TextFormat("Final Score: %d", score), 540, 270, 25, YELLOW);
                DrawText(TextFormat("High Score: %d", highHighScore), 535, 310, 25, GOLD);

                // Button to restart 
                if (DrawButton(540, 380, 200, 40, "Play Again")) {
                    score = 0;
                    carPosition = { 0.0f, 0.0f, 0.0f };
                    for(int i = 0; i < Obs; i++) {
                        obsAct[i] = false;
                        obsPass[i] = false;
                    }
                    currentScreen = ScreenHighway;
                }

                // Button to go back to main menu
                if (DrawButton(540, 440, 200, 40, "Main Menu")) {
                    currentScreen = ScreenMenu;
                }
                break;
            }

            case ScreenSecond: {
                DrawText("Second Game Mode", 270, 180, 30, BLUE);

                // Button to return to main menu
                if (DrawButton(50, 380, 150, 40, "Main Menu")) {
                    currentScreen = ScreenMenu;
                }
                break;
            }

            case ScreenRoadTrip: {
                DrawText("Road Trip Mode", 290, 180, 30, DARKGREEN);

                // Button to return to main menu
                if (DrawButton(50, 380, 150, 40, "Main Menu")) {
                    currentScreen = ScreenMenu;
                }
                break;
            }

            case ScreenHighscores: {
                DrawText("High Scores", 270, 180, 30, ORANGE);
                DrawText(TextFormat("Saved High Score: %d", highHighScore), 270, 240, 25, WHITE);

                // Button to return to main menu
                if (DrawButton(50, 380, 150, 40, "Main Menu")) {
                    currentScreen = ScreenMenu;
                }
                break;
            }
        }

        EndDrawing();
    }

    UnloadModel(car);
    UnloadModel(obstacleCar);
    UnloadTexture(bgTexture);
    CloseWindow();
    return 0;
}
