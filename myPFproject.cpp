#define _CRT_SECURE_NO_WARNINGS   // lets Visual Studio accept fopen / fscanf (must stay the FIRST line)

#include "raylib.h" // for graphics, input, audio, 3D camera ....
#include "rlgl.h" //for rotation and scaling of 3d matrix transformation (like i have used functions like rlPushMatrix and rlrotatef otherwise on these functions compiler will give error)
#include <math.h> // for sinf, cosf, fabsf
#include <stdio.h> // for printf, sprintf, fopen, fscanf
#include <string.h> // for strcmp, strcpy, strlen
int SCREEN_W = 1280;                     // size of the window and updated every frame
int SCREEN_H = 720; //why updated because if we resize the window then the size of the window will change and we need to update the size of the window in our code so that we can draw on the whole window and not just on a part of it

// game states 
const int STATE_MENU = 0, STATE_PLAY = 3, STATE_PAUSE = 4, STATE_TOUR_RESULT = 9; //names of the screens in the game it is easy to write if(gameState==STATE_PLAY) than if(gameState==3) so we have given names to the states of the game 

// car physics tuning (all numbers in one place so we can tune the feel)
const float BRAKE_DECEL = 26.0f;   // m/s^2 when S is pressed and with delta time it will be multiplied with the delta time to get the final deceleration of the car and usual formula speed-=BRAKE_DECEL*GetFrameTime() is used to and the car will slow down with this deceleration when S is pressed
const float HANDBRAKE_DECEL = 14.0f; // m/s^2 when space is pressed 
const float REVERSE_ACC = 6.0f; // m/s^2 when W is pressed and car is going backwards
const float MAX_REVERSE = 9.0f; // m/s top speed in reverse
const float ROLLING_DRAG = 0.35f;   // rolling resistance between tyre and road when unpress the W(acceleration) if it's not used then car will never stop.
const float AIR_DRAG = 0.0002f; // grows with speed squared ( so it is negligible at low speed, but dominates at high speed) and car wont accelerate more than the assigned max speed.
const float TURN_GRIP = 16.0f;   // how hard the tyres can turn the car 

// car list like four cars stats in a 2D array  [car][stat]
const int NUM_CARS = 4; // number of cars in the game 
const int STAT_SPEED = 0, STAT_ACCEL = 1, STAT_GRIP = 2; //W for acceleration and S for brake
float carStats[NUM_CARS][3] = { //STAT_GRIP is used 
    { 62.0f, 12.0f, 1.00f },   // Street GT
    { 72.0f, 15.0f, 1.15f },   // Sprint R
    { 55.0f,  9.5f, 0.85f },   // Heavy Tourer
    { 66.0f, 13.0f, 1.05f }    // Electric GT
};
const char* carName[NUM_CARS] = { "Street GT", "Sprint R", "Heavy Tourer", "Electric GT" };
Color carColor[NUM_CARS] = { {200,30,40,255}, {30,100,210,255}, {240,170,20,255}, {90,200,150,255} };

// assets: model slots 0..3 = the four cars. (4, 5, 6 = bus, cockpit, wheel: not used in City Tour, but we keep the
// numbering so the slots stay the same as in the full game)
const int NUM_MODELS = 13; // total 13 models hain iss liye isse
const int CITY_MODEL = 7, CHARGER_MODEL = 8;          // the city and the charging pump
const int PUMP_MODEL = 9, GARAGE_MODEL = 10;          // the petrol pump and the workshop building
const int TIRE_MODEL = 11, BATTERY_CHARGER_MODEL = 12;   // things lying around at the workshop
const char* modelFiles[NUM_MODELS] =
{
    "resources/models/car_i30.glb",
    "resources/models/car_gallardo.glb",
    "resources/models/car_volvo.glb",
    "resources/models/car_audi.glb",
    "resources/models/bus.glb",
    "resources/models/cockpit.glb",
    "resources/models/steering_wheel.glb",
    "resources/models/city.glb",
    "resources/models/charger.glb",
    "resources/models/pump.glb",
    "resources/models/garage.glb",
    "resources/models/tire.glb",
    "resources/models/battery_charger.glb"
    "resources/models/cockpit.glb"
};
// size of the longest side of each model in metres (the code scales the GLB to this)
float modelLength[NUM_MODELS] = { 4.4f, 4.4f, 4.7f, 4.9f, 9.0f, 2.2f, 0.38f, 558.0f, 2.3f, 2.3f, 12.0f, 0.7f, 1.8f }; //we can adjust models' length through it
// if a model faces the wrong way, change its fix to 180 (or 90 / -90)
float modelTurnFix[NUM_MODELS] = { 0, 0, 180, 0, 0, 0, 0, 0, 0, 0, 180, 0, 0 }; // fix for model face means orientation
// glow / glass meshes that look like black shapes in raylib. Meshes with exactly these triangle counts are hidden.
int hideTriangles[NUM_MODELS][2] = { {154, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {4, 220}, {0, 0}, {0, 0}, {120, 0} }; //hide black things like if any headlights rendering ,it avoids black blocks of that 

//  global variables 
int   gameState = STATE_MENU; // global variable to keep track of the current state of the game like menu, play, pause, result etc. and it is used in the main loop to switch between different states of the game and it is initialized to STATE_MENU so that the game starts with the menu screen
int   selectedCar = 0; //

bool  showFps = true; // show fps on the screen or not if it is not present then fps will not be shown on the screen which is useful for debugging and testing the game and it can be toggled by pressing F1 key on the keyboard
float volume = 0.7f; // 0.0 = silent, 1.0 = full volume

Font  font; //font for the game like speedometer and other text in the game and used in drawText and drawTextCentered functions and it can be changed through loadAssets function and it is loaded from the resources/fonts/hud.ttf(truetype font file if it is present otherwise default font will be used 

Model models[NUM_MODELS]; // Model array to store the loaded models 
bool  modelLoaded[NUM_MODELS]; //check whether the model is present or not and it is used in the drawCar function to check whether the model is loaded or not and if it is not loaded then it will draw a box car instead of the model and it is initialized to false for all models and it is set to true when the model is loaded successfully in the loadAssets function and on false the model will be skipped to avoid crashing of game.
float modelScale[NUM_MODELS]; // scale ka matlab model ka size ko adjust karna like agar model ka size chota hai to usse bada karna aur agar model ka size bada hai to usse chota karna and it is used in the drawModelAt function to scale the model to the desired size and it is calculated in the measureModel function based on the modelLength array and the actual size of the model and it is initialized to 1.0f for all models and it is set to the calculated scale when the model is loaded successfully in the loadAssets function
float modelCenterX[NUM_MODELS];
float modelCenterZ[NUM_MODELS];
float modelBottom[NUM_MODELS];

Music engineSound;                       // a stream: it loops without a gap and its pitch can change
Sound crashSound, passSound, clickSound, errorSound, successSound;
bool  engineSoundLoaded = false, crashSoundLoaded = false, passSoundLoaded = false, clickSoundLoaded = false;
bool  errorSoundLoaded = false, successSoundLoaded = false;
bool  audioReady = false;

// player car
float carX = 0, carZ = 0, carAngle = 0, carSpeed = 0, steer = 0;
float distanceDriven = 0;              // total distance driven in metres used here for the City Tour to measure how far the player has driven
float straightenPower = 0;             // how strongly the car straightens itself (0 = off) means power
float maxCarAngle = 100.0f;            // maximum heading angle in radians (100 = no limit)
float speedLimitFactor = 1.0f;         // 1.0 = the car may reach its full top speed but this factor is introduced so that the City Tour can limit the top speed to make it easier to drive around the city
bool  controlsLocked = false;

// camera
Camera3D camera;
int   cameraMode = 0;                  // 0 chase, 1 bonnet, 2 cockpit
const char* cameraName[3] = { "CHASE", "BONNET", "COCKPIT" };
const float EYE_HEIGHT = 1.15f;
const float EYE_FORWARD = 0.10f;
Vector3 cameraBase = { 0, 3, 8 };      // camera position before the shake is added
float menuAngle = 0;
float gameTime = 0;                    // seconds since the run started
float shake = 0, messageTimer = 0;
char  message[64] = "";


// PART 2: small helper functions

float keepInRange(float v, float lo, float hi) { if (v < lo) return lo; if (v > hi) return hi; return v; }
float mix(float a, float b, float t) { return a + (b - a) * t; }

// moves *value towards target, at most rate*dt  (pointer parameter on purpose)
void moveToward(float* value, float target, float rate, float dt)
{
    float diff = target - *value;
    float step = rate * dt;
    if (diff > step) *value += step;
    else if (diff < -step) *value -= step;
    else *value = target;
}

// repeatable pseudo random number in 0..1 from an integer (so scenery never needs storage)
float randomFromNumber(int n)
{
    unsigned int x = (unsigned int)n;
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x ^= x >> 4;
    x *= 0x27d4eb2du;
    x ^= x >> 15;
    return (float)(x & 0xFFFFFF) / 16777215.0f;
}

void playClickSound() { if (clickSoundLoaded) PlaySound(clickSound); }
void playErrorSound() { if (errorSoundLoaded) PlaySound(errorSound); }
void playSuccessSound() { if (successSoundLoaded) PlaySound(successSound); }

// turns an angle into the range -PI .. PI
float fixAngle(float a)
{
    while (a > PI)  a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

// time as minutes:seconds
const char* timeText(float t)
{
    if (t <= 0.0f) return "-:--.--";
    int m = (int)(t / 60.0f);
    return TextFormat("%d:%05.2f", m, t - m * 60.0f);
}


// PART 3: asset loading (every file is optional, the game falls back to shapes)

void findResources()
{
    if (DirectoryExists("resources")) return;
    ChangeDirectory(GetApplicationDirectory());
    if (DirectoryExists("resources")) return;
    ChangeDirectory("..");                       // exe may be in x64/Debug
    if (DirectoryExists("resources")) return;
    ChangeDirectory("..");
}

// switches off the meshes we do not want to draw (they are skipped by drawing and by measuring)
void hideBadMeshes(int i)
{
    for (int k = 0; k < models[i].meshCount; k++)
        for (int h = 0; h < 2; h++)
        {
            if (hideTriangles[i][h] > 0 && models[i].meshes[k].triangleCount == hideTriangles[i][h])
            {
                models[i].meshes[k].triangleCount = 0;
                models[i].meshes[k].vertexCount = 0;
            }
        }
}

// measures a loaded model and stores the scale / centre so we can place it exactly
void measureModel(int i)
{
    float minX = 1.0e30f, minY = 1.0e30f, minZ = 1.0e30f;
    float maxX = -1.0e30f, maxY = -1.0e30f, maxZ = -1.0e30f;
    for (int k = 0; k < models[i].meshCount; k++)
    {
        Mesh* m = &models[i].meshes[k];
        if (m->vertices == NULL) continue;
        for (int v = 0; v < m->vertexCount; v++)
        {
            float x = m->vertices[v * 3], y = m->vertices[v * 3 + 1], z = m->vertices[v * 3 + 2];
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
            if (z < minZ) minZ = z;
            if (z > maxZ) maxZ = z;
        }
    }
    float big = maxX - minX;
    if (maxY - minY > big) big = maxY - minY;
    if (maxZ - minZ > big) big = maxZ - minZ;
    if (big < 0.0001f) { modelLoaded[i] = false; return; }
    modelScale[i] = modelLength[i] / big;
    modelCenterX[i] = (minX + maxX) * 0.5f;
    modelCenterZ[i] = (minZ + maxZ) * 0.5f;
    modelBottom[i] = minY;
}

void prepareCityModel();                  // defined in the City Tour part
void setupCityData();

void loadAssets()
{
    // font
    font = GetFontDefault();
    if (FileExists("resources/fonts/hud.ttf"))
    {
        font = LoadFontEx("resources/fonts/hud.ttf", 64, NULL, 0);
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }
    // models
    for (int i = 0; i < NUM_MODELS; i++)
    {
        modelLoaded[i] = false;
        if (FileExists(modelFiles[i]))
        {
            models[i] = LoadModel(modelFiles[i]);
            if (models[i].meshCount > 0)
            {
                modelLoaded[i] = true;
                if (i == CITY_MODEL) prepareCityModel();
                else { hideBadMeshes(i); measureModel(i); }
            }
        }
    }
    // sounds
    audioReady = IsAudioDeviceReady();
    if (audioReady)
    {
        if (FileExists("resources/sounds/engine.wav")) { engineSound = LoadMusicStream("resources/sounds/engine.wav"); engineSoundLoaded = true; }
        if (FileExists("resources/sounds/crash.wav")) { crashSound = LoadSound("resources/sounds/crash.wav");  crashSoundLoaded = true; }
        if (FileExists("resources/sounds/pass.wav")) { passSound = LoadSound("resources/sounds/pass.wav");   passSoundLoaded = true; }
        if (FileExists("resources/sounds/click.wav")) { clickSound = LoadSound("resources/sounds/click.wav");  clickSoundLoaded = true; }
        if (FileExists("resources/sounds/error.wav")) { errorSound = LoadSound("resources/sounds/error.wav");  errorSoundLoaded = true; }
        if (FileExists("resources/sounds/success.wav")) { successSound = LoadSound("resources/sounds/success.wav"); successSoundLoaded = true; }
        SetMasterVolume(volume);
    }
    setupCityData();                       // building boxes and places of the city
}

void unloadAssets()
{
    for (int i = 0; i < NUM_MODELS; i++) if (modelLoaded[i]) UnloadModel(models[i]);
    if (engineSoundLoaded) UnloadMusicStream(engineSound);
    if (crashSoundLoaded)  UnloadSound(crashSound);
    if (passSoundLoaded)   UnloadSound(passSound);
    if (clickSoundLoaded)  UnloadSound(clickSound);
    if (errorSoundLoaded)  UnloadSound(errorSound);
    if (successSoundLoaded) UnloadSound(successSound);
}


// PART 4: 2D user interface helpers

void drawText(const char* t, float x, float y, float size, Color c)
{
    DrawTextEx(font, t, Vector2{ x, y }, size, size / 10.0f, c);
}

void drawTextCentered(const char* t, float cx, float y, float size, Color c)
{
    Vector2 s = MeasureTextEx(font, t, size, size / 10.0f);
    DrawTextEx(font, t, Vector2{ cx - s.x / 2.0f, y }, size, size / 10.0f, c);
}

// button: returns true on the frame it is clicked
bool button(const char* label, float x, float y, float w, float h)
{
    Rectangle r = { x, y, w, h };
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);
    Color bg = hover ? Color{ 230, 60, 50, 255 } : Color{ 28, 32, 44, 235 };
    Color edge = hover ? Color{ 255, 190, 170, 255 } : Color{ 90, 100, 130, 255 };
    DrawRectangleRounded(r, 0.25f, 8, bg);
    DrawRectangleLinesEx(r, 2, edge);
    drawTextCentered(label, x + w / 2.0f, y + h / 2.0f - 12.0f, 26, WHITE);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { playClickSound(); return true; }
    return false;
}

void drawStatBar(const char* label, float x, float y, float frac)
{
    drawText(label, x, y, 20, Color{ 200, 205, 220, 255 });
    DrawRectangle((int)x + 110, (int)y + 5, 160, 10, Color{ 50, 56, 74, 255 });
    DrawRectangle((int)x + 110, (int)y + 5, (int)(160 * keepInRange(frac, 0, 1)), 10, Color{ 230, 60, 50, 255 });
}

void drawSky()
{
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, Color{ 70, 130, 210, 255 }, Color{ 205, 225, 245, 255 });
}


// PART 5: drawing cars (GLB model if we have it, otherwise simple boxes)
 

// draws model i with its bottom-centre at (x,y,z) and rotated yawDeg around Y
void drawModelAt(int i, float x, float y, float z, float yawDeg, Color tint)
{
    float s = modelScale[i];
    float a = yawDeg * DEG2RAD;
    float ca = cosf(a), sa = sinf(a);
    // model-space point we want to land on (x,y,z): its bottom centre, scaled
    float px = modelCenterX[i] * s, py = modelBottom[i] * s, pz = modelCenterZ[i] * s;
    // rotate it around Y the same way DrawModelEx will
    float rx = px * ca + pz * sa;
    float rz = -px * sa + pz * ca;
    Vector3 pos = { x - rx, y - py, z - rz };
    DrawModelEx(models[i], pos, Vector3{ 0, 1, 0 }, yawDeg, Vector3{ s, s, s }, tint);
}

void drawBoxCar(float x, float z, float headingRad, Color body, float len)
{
    float half = len * 0.5f;
    bool big = (len > 7.0f);                         // truck / bus
    float wx = big ? 1.2f : 0.95f;
    Color dark = { 15, 15, 15, 255 };
    rlPushMatrix();
    rlTranslatef(x, 0, z);
    rlRotatef(-headingRad * RAD2DEG, 0, 1, 0);
    if (big)
    {
        DrawCube(Vector3{ 0, 1.6f, 0.8f }, 2.4f, 2.6f, len - 2.2f, body);
        DrawCube(Vector3{ 0, 1.3f, -half + 1.1f }, 2.3f, 2.0f, 2.0f, Color{ 40, 45, 55, 255 });
    }
    else
    {
        DrawCube(Vector3{ 0, 0.55f, 0 }, 1.9f, 0.6f, len * 0.95f, body);
        DrawCube(Vector3{ 0, 1.05f, len * 0.05f }, 1.6f, 0.5f, len * 0.5f, Color{ 25, 30, 40, 255 });
    }
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            DrawCube(Vector3{ sx * wx, 0.35f, sz * (half - 0.9f) }, 0.25f, 0.7f, 0.7f, dark);
    float ly = big ? 1.0f : 0.65f;
    DrawCube(Vector3{ -0.65f, ly, half }, 0.3f, 0.15f, 0.05f, RED);
    DrawCube(Vector3{ 0.65f, ly, half }, 0.3f, 0.15f, 0.05f, RED);
    DrawCube(Vector3{ -0.65f, ly, -half }, 0.3f, 0.15f, 0.05f, Color{ 255, 240, 170, 255 });
    DrawCube(Vector3{ 0.65f, ly, -half }, 0.3f, 0.15f, 0.05f, Color{ 255, 240, 170, 255 });
    rlPopMatrix();
}

// heading 0 = facing -Z. glTF models face +Z, so they get an extra 180 degrees.
void drawCar(int modelIdx, float x, float z, float headingRad, Color body)
{
    if (modelLoaded[modelIdx])
        drawModelAt(modelIdx, x, 0.0f, z, 180.0f - headingRad * RAD2DEG + modelTurnFix[modelIdx], WHITE);
    else
        drawBoxCar(x, z, headingRad, body, modelLength[modelIdx]);
}

// PART 6: car physics

void resetCar()
{
    carX = 0;
    carZ = 0; carAngle = 0; carSpeed = 0; steer = 0; distanceDriven = 0;
}

void updateCar(float dt)
{
    float maxS = carStats[selectedCar][STAT_SPEED] * speedLimitFactor;     // City Tour uses a lower top speed
    float acc = carStats[selectedCar][STAT_ACCEL];
    float grip = carStats[selectedCar][STAT_GRIP];

    // ---- input ----
    float thr = 0, brk = 0, st = 0;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    thr = 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  brk = 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) st += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  st -= 1;
    bool hb = IsKeyDown(KEY_SPACE);
    if (controlsLocked) { thr = 0; brk = 0; st = 0; hb = false; }   // during a breakdown

    // ---- speed (acceleration shrinks as we get near top speed) ----
    float a = 0;
    if (thr > 0) a += acc * keepInRange(1.0f - carSpeed / maxS, 0.0f, 1.0f);
    if (brk > 0) { if (carSpeed > 0.5f) a -= BRAKE_DECEL; else a -= REVERSE_ACC; }
    if (hb && carSpeed > 0) a -= HANDBRAKE_DECEL;
    float resist = ROLLING_DRAG + AIR_DRAG * carSpeed * carSpeed;
    if (carSpeed > 0.0f) a -= resist;
    else if (carSpeed < 0.0f) a += resist;
    carSpeed += a * dt;
    if (carSpeed < -MAX_REVERSE) carSpeed = -MAX_REVERSE;
    if (thr == 0 && brk == 0 && fabsf(carSpeed) < 0.2f) carSpeed = 0;

    // ---- steering (smoothed input, turn rate limited by tyre grip) ----
    moveToward(&steer, st, (st == 0.0f) ? 8.0f : 5.0f, dt);
    float sp = fabsf(carSpeed);
    float lat = TURN_GRIP * grip * (hb ? 1.5f : 1.0f);
    float maxYaw = lat / (sp < 1.0f ? 1.0f : sp);
    if (maxYaw > 1.8f) maxYaw = 1.8f;
    float lowScale = keepInRange(sp / 3.0f, 0.0f, 1.0f);          // no spinning on the spot
    float dir = (carSpeed >= 0.0f) ? 1.0f : -1.0f;
    carAngle += steer * maxYaw * lowScale * dir * dt;
    if (st == 0.0f && !hb) carAngle -= carAngle * straightenPower * dt;
    carAngle = keepInRange(carAngle, -maxCarAngle, maxCarAngle);

    // ---- move ----
    carX += sinf(carAngle) * carSpeed * dt;
    carZ -= cosf(carAngle) * carSpeed * dt;
    distanceDriven += fabsf(carSpeed) * dt;
}


// PART 7: cameras and engine sound

void moveCamera(float dt, bool snap)
{
    float fx = sinf(carAngle), fz = -cosf(carAngle);      // forward
    float rx = cosf(carAngle), rz = sinf(carAngle);       // right
    float maxS = carStats[selectedCar][STAT_SPEED];
    float speedFrac = keepInRange(fabsf(carSpeed) / maxS, 0.0f, 1.0f);

    Vector3 pos, target;
    if (cameraMode == 0)                                    // chase
    {
        float dist = 7.5f + speedFrac * 2.0f;
        Vector3 want = { carX - fx * dist, 3.0f + speedFrac * 0.6f, carZ - fz * dist };
        float k = snap ? 1.0f : 1.0f - expf(-6.0f * dt);
        pos.x = mix(cameraBase.x, want.x, k);
        pos.y = mix(cameraBase.y, want.y, k);
        pos.z = mix(cameraBase.z, want.z, k);
        target = Vector3{ carX + fx * 8.0f, 1.3f, carZ + fz * 8.0f };
    }
    else if (cameraMode == 1)                               // bonnet
    {
        pos = Vector3{ carX + fx * 0.9f, 1.15f, carZ + fz * 0.9f };
        target = Vector3{ pos.x + fx * 25.0f, 1.0f, pos.z + fz * 25.0f };
    }
    else                                                  // cockpit
    {
        pos = Vector3{ carX + fx * EYE_FORWARD, EYE_HEIGHT, carZ + fz * EYE_FORWARD };
        target = Vector3{ pos.x + fx * 25.0f, EYE_HEIGHT - 0.1f, pos.z + fz * 25.0f };
    }
    cameraBase = pos;
    camera.position = pos;
    camera.target = target;
    if (shake > 0.0f)                                    // wall-hit shake
    {
        camera.position.x += sinf(gameTime * 83.0f) * shake * 0.25f;
        camera.position.y += sinf(gameTime * 97.0f + 1.0f) * shake * 0.20f;
    }
    // lean into the corner a little
    float tilt = steer * 0.035f * speedFrac;
    camera.up = Vector3{ rx * tilt, 1.0f, rz * tilt };
    // wider view at speed gives a feeling of speed
    camera.fovy = 58.0f + 20.0f * speedFrac;
}

void setupCamera()
{
    camera.position = Vector3{ 0, 3, 8 };
    camera.target = Vector3{ 0, 1, 0 };
    camera.up = Vector3{ 0, 1, 0 };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    cameraBase = camera.position;
}

void playEngine()
{
    if (!engineSoundLoaded) return;
    float f = keepInRange(fabsf(carSpeed) / carStats[selectedCar][STAT_SPEED], 0.0f, 1.0f);
    if (!IsMusicStreamPlaying(engineSound)) PlayMusicStream(engineSound);
    UpdateMusicStream(engineSound);                     // must be called every frame to keep the sound going
    SetMusicPitch(engineSound, 0.7f + f * 1.2f);
    SetMusicVolume(engineSound, 0.35f + 0.4f * f);
}

void stopEngine() { if (engineSoundLoaded) StopMusicStream(engineSound); }


// PART 8: HUD (speedometer, gear, camera label)
void drawSpeedometer()
{
    float cx = SCREEN_W - 170.0f, cy = SCREEN_H - 150.0f, r = 112.0f;
    float kmh = fabsf(carSpeed) * 3.6f;
    float frac = keepInRange(kmh / 280.0f, 0.0f, 1.0f);

    DrawCircle((int)cx, (int)cy, r + 14, Fade(BLACK, 0.55f));
    DrawRing(Vector2{ cx, cy }, r - 10, r, 135.0f, 405.0f, 64, Color{ 60, 66, 86, 255 });
    Color ring = (frac < 0.75f) ? Color{ 80, 200, 255, 255 } : Color{ 255, 90, 60, 255 };
    DrawRing(Vector2{ cx, cy }, r - 10, r, 135.0f, 135.0f + 270.0f * frac, 64, ring);

    float ang = (135.0f + 270.0f * frac) * DEG2RAD;
    DrawLineEx(Vector2{ cx, cy }, Vector2{ cx + cosf(ang) * (r - 18), cy + sinf(ang) * (r - 18) }, 4, Color{ 255, 90, 60, 255 });
    DrawCircle((int)cx, (int)cy, 8, WHITE);

    drawTextCentered(TextFormat("%d", (int)(kmh + 0.5f)), cx, cy + 20, 44, WHITE);
    drawTextCentered("km/h", cx, cy + 62, 20, Color{ 190, 195, 210, 255 });

    // gear is simulated from the speed (6 gears)
    const char* gear;
    if (carSpeed < -0.5f) gear = "R";
    else if (carSpeed < 0.5f) gear = "N";
    else
    {
        int g = 1 + (int)(carSpeed / (carStats[selectedCar][STAT_SPEED] / 6.0f));
        if (g > 6) g = 6;
        gear = TextFormat("%d", g);
    }
    drawTextCentered(gear, cx, cy - 62, 40, Color{ 255, 210, 80, 255 });
}

void drawBasicHud()
{
    drawSpeedometer();
    DrawRectangle(20, SCREEN_H - 70, 330, 50, Fade(BLACK, 0.5f));
    drawText(TextFormat("Camera: %s  (C)", cameraName[cameraMode]), 34, SCREEN_H - 58, 24, WHITE);
    if (showFps) DrawFPS(20, 20);
}


// PART 9: City Tour data (made from city.glb)

const float CITY_WIDTH = 558.0f;                      // the city is scaled to this width (metres)
const float CITY_UNITS_WIDTH = 55800.0f, CITY_UNITS_DEPTH = 23400.0f;   // size of city.glb in its own units
const float DRAW_DISTANCE = 210.0f;                   // buildings further away than this are not drawn
const float REACH = 22.0f;                            // how close we must get to a landmark
const float TANK_SIZE = 30.0f;
const float FUEL_PRICE = 120.0f;                      // money per litre
const float TOW_COST = 500.0f, REPAIR_COST = 800.0f, LANDMARK_BONUS = 400.0f;
const float CAR_RADIUS = 1.2f;

const int CITY_HIDE_COUNT = 175;
// glow meshes that look wrong in raylib: mesh number and the number of triangles it must have
int cityHideIndex[CITY_HIDE_COUNT] = {
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    58, 68, 81, 82, 84, 85, 89, 132, 134, 135, 138, 154,
    160, 161, 162, 166, 167, 169, 170, 183, 185, 197, 198, 202,
    203, 204, 238, 244, 249, 258, 266, 275, 284, 288, 302, 310,
    319, 328, 329, 330, 331, 332, 364, 372, 403, 458, 459, 460,
    461, 462, 463, 464, 465, 466, 467, 468, 469, 487, 489, 491,
    493, 495, 497, 499, 501, 505, 510, 519, 527, 536, 545, 549,
    563, 571, 580, 589, 590, 591, 592, 593, 597, 602, 611, 619,
    628, 637, 641, 655, 663, 672, 681, 682, 683, 684, 685, 689,
    694, 703, 711, 720, 729, 733, 747, 755, 764, 773, 774, 775,
    776, 777, 781, 786, 795, 803, 817, 825, 834, 843, 844, 845,
    846, 850, 859, 868, 872, 886, 894, 903, 912, 913, 914, 915,
    919, 924, 933, 941, 950, 959, 963, 977, 982, 983, 984, 1062,
    1113, 1126, 1129, 1136, 1174, 1177, 1215, 1218, 1256, 1259, 1272, 1282,
    1295, 1296, 1298, 1299, 1303, 1347, 1350,
};
int cityHideTriangles[CITY_HIDE_COUNT] = {
    60, 6, 6, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 48, 10, 2, 2, 2, 22, 2,
    8, 8, 2, 8, 8, 2, 8, 2, 8, 8, 28, 28, 28, 28, 36, 20, 20, 60, 6, 6,
    4, 4, 2, 2, 2, 2, 2, 2, 2, 28, 28, 28, 28, 28, 28, 28, 28, 2, 8, 8,
    2, 8, 8, 2, 8, 2, 8, 8, 28, 28, 28, 28, 2, 8, 8, 2, 8, 8, 2, 8,
    2, 8, 8, 28, 28, 28, 28, 2, 8, 8, 2, 8, 8, 2, 8, 2, 8, 8, 28, 28,
    28, 28, 2, 8, 8, 2, 8, 2, 8, 8, 28, 28, 28, 2, 8, 8, 2, 8, 2, 8,
    8, 28, 28, 28, 2, 8, 8, 2, 8, 8, 2, 8, 28, 28, 28, 2, 2, 4, 4, 20,
    4, 4, 22, 36, 22, 36, 2, 2, 2, 2, 2, 2, 2, 4, 4,
};
// building boxes in model units, measured from the corner of the city: minX, minZ, maxX, maxZ
const int CITY_BOX_COUNT = 38;
int cityBoxData[CITY_BOX_COUNT][4] = {
    { 3700, 12750, 5400, 14250 },
    { 3680, 12850, 8916, 14263 },
    { 7200, 12750, 8900, 14250 },
    { 10774, 12539, 12618, 14599 },
    { 12619, 12615, 14377, 14451 },
    { 16331, 12136, 18015, 12594 },
    { 16168, 12591, 18021, 14433 },
    { 16235, 14400, 17928, 14713 },
    { 17966, 5390, 19809, 7450 },
    { 18100, 9150, 19800, 10729 },
    { 17988, 10695, 19813, 12595 },
    { 17949, 12573, 19839, 14451 },
    { 18080, 9250, 23316, 10663 },
    { 19802, 12623, 21407, 14432 },
    { 19869, 14398, 21436, 14619 },
    { 21600, 9150, 23300, 10650 },
    { 20960, 12600, 23440, 14438 },
    { 25350, 1900, 26850, 3600 },
    { 25337, 1880, 26750, 7116 },
    { 25350, 5400, 26850, 7100 },
    { 25215, 9023, 27051, 10781 },
    { 25191, 10779, 27033, 12632 },
    { 25200, 12600, 27000, 14400 },
    { 27000, 10872, 27313, 12565 },
    { 28820, 1823, 30407, 3632 },
    { 28869, 3598, 30436, 3819 },
    { 28614, 5374, 30674, 7218 },
    { 28776, 9018, 30624, 10843 },
    { 29960, 1800, 32440, 3638 },
    { 30619, 5415, 32377, 7251 },
    { 32380, 10752, 34240, 12601 },
    { 32453, 12700, 34200, 14300 },
    { 34200, 12700, 36000, 14330 },
    { 36100, 7253, 37700, 9000 },
    { 36070, 9000, 37700, 10800 },
    { 36070, 9000, 37700, 12600 },
    { 34200, 12700, 37800, 14330 },
    { 41262, 12577, 45020, 14434 },
};
// the six places of the tour, in metres: minX, minZ, maxX, maxZ
const int LANDMARK_COUNT = 6;
const char* landmarkName[LANDMARK_COUNT] = { "Apartments", "Pawn Shop", "Grand Hotel", "Shopping Mall", "Fish Factory", "Clinic" };
float landmarkBox[LANDMARK_COUNT][4] = {
    { 36.8f, 127.5f, 89.2f, 142.6f },
    { 107.7f, 125.4f, 143.8f, 146.0f },
    { 161.7f, 91.5f, 234.4f, 147.1f },
    { 251.9f, 90.2f, 306.2f, 144.0f },
    { 323.8f, 72.5f, 378.0f, 143.3f },
    { 412.6f, 125.8f, 450.2f, 144.3f },
};

// the city in the game
bool   cityOk = false;                                // the model has the size we expect
float  cityScale = 0.01f, cityMinX = 0, cityMinZ = 0;
float  cityWidth = 558.0f, cityDepth = 234.0f;        // metres
int    hiddenMeshes = 0;
float* meshX = NULL;  float* meshZ = NULL;  float* meshRadius = NULL;    // where each mesh is (for drawing only what is near)
int    boxCount = 0;
float* boxMinX = NULL;  float* boxMinZ = NULL;  float* boxMaxX = NULL;  float* boxMaxZ = NULL;
float  landBox[LANDMARK_COUNT][4];
float  stationX = 480, stationZ = 196, workshopX = 80, workshopZ = 196;   // the two places in the yard in the south

// tour state
float fuel = 12.0f, money = 3000.0f;
int   tourNext = 0, fault = -1, faultCount = 0;
float tourTime = 0, faultTimer = 0, bestTourTime = 0;
bool  newTourRecord = false;
const char* faultName[5] = { "Flat battery", "Engine overheating", "Low engine oil", "Flat tyre", "Out of fuel" };

// PART 10: City Tour logic

// distance from a point to a rectangle (0 if the point is inside)
float distToBox(float px, float pz, float x0, float z0, float x1, float z1)
{
    float dx = 0.0f, dz = 0.0f;
    if (px < x0) dx = x0 - px; else if (px > x1) dx = px - x1;
    if (pz < z0) dz = z0 - pz; else if (pz > z1) dz = pz - z1;
    return sqrtf(dx * dx + dz * dz);
}

void freeCity()
{
    delete[] meshX; delete[] meshZ; delete[] meshRadius;
    delete[] boxMinX; delete[] boxMinZ; delete[] boxMaxX; delete[] boxMaxZ;
    meshX = meshZ = meshRadius = NULL;
    boxMinX = boxMinZ = boxMaxX = boxMaxZ = NULL;
    boxCount = 0;
}

// called after city.glb is loaded: hide the glow meshes, measure the model, remember where every mesh is
void prepareCityModel()
{
    Model* m = &models[CITY_MODEL];
    hiddenMeshes = 0;
    for (int h = 0; h < CITY_HIDE_COUNT; h++)
    {
        int k = cityHideIndex[h];
        if (k < m->meshCount && m->meshes[k].triangleCount == cityHideTriangles[h])
        {
            m->meshes[k].triangleCount = 0;
            m->meshes[k].vertexCount = 0;
            hiddenMeshes++;
        }
    }
    float minX = 1.0e30f, minZ = 1.0e30f, maxX = -1.0e30f, maxZ = -1.0e30f;
    for (int k = 0; k < m->meshCount; k++)
        for (int v = 0; v < m->meshes[k].vertexCount; v++)
        {
            float x = m->meshes[k].vertices[v * 3], z = m->meshes[k].vertices[v * 3 + 2];
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (z < minZ) minZ = z;
            if (z > maxZ) maxZ = z;
        }
    float w = maxX - minX, d = maxZ - minZ;
    cityOk = (fabsf(w - CITY_UNITS_WIDTH) < 560.0f && fabsf(d - CITY_UNITS_DEPTH) < 240.0f);
    cityScale = CITY_WIDTH / w;
    cityMinX = minX; cityMinZ = minZ;

    meshX = new float[m->meshCount];
    meshZ = new float[m->meshCount];
    meshRadius = new float[m->meshCount];
    for (int k = 0; k < m->meshCount; k++)
    {
        meshRadius[k] = -1.0f;                         // -1 = do not draw
        if (m->meshes[k].vertexCount == 0) continue;
        float x0 = 1.0e30f, z0 = 1.0e30f, x1 = -1.0e30f, z1 = -1.0e30f;
        for (int v = 0; v < m->meshes[k].vertexCount; v++)
        {
            float x = m->meshes[k].vertices[v * 3], z = m->meshes[k].vertices[v * 3 + 2];
            if (x < x0) x0 = x;
            if (x > x1) x1 = x;
            if (z < z0) z0 = z;
            if (z > z1) z1 = z;
        }
        meshX[k] = ((x0 + x1) * 0.5f - minX) * cityScale;
        meshZ[k] = ((z0 + z1) * 0.5f - minZ) * cityScale;
        float rx = (x1 - x0) * 0.5f, rz = (z1 - z0) * 0.5f;
        meshRadius[k] = sqrtf(rx * rx + rz * rz) * cityScale;
    }
}

// builds the building boxes, landmarks and the two special places (works with or without the model)
void setupCityData()
{
    float ratio = cityScale / 0.01f;                   // the tables were made for a scale of 0.01
    cityWidth = 55800.0f * cityScale;
    cityDepth = 23400.0f * cityScale;
    delete[] boxMinX; delete[] boxMinZ; delete[] boxMaxX; delete[] boxMaxZ;
    int n = CITY_BOX_COUNT + 4;                        // + the garage walls, the stack of tyres and the battery charger
    boxMinX = new float[n]; boxMinZ = new float[n]; boxMaxX = new float[n]; boxMaxZ = new float[n];
    boxCount = 0;
    if (cityOk || !modelLoaded[CITY_MODEL])            // wrong model size: no boxes, we would block the wrong places
    {
        for (int b = 0; b < CITY_BOX_COUNT; b++)
        {
            boxMinX[boxCount] = cityBoxData[b][0] * cityScale;
            boxMinZ[boxCount] = cityBoxData[b][1] * cityScale;
            boxMaxX[boxCount] = cityBoxData[b][2] * cityScale;
            boxMaxZ[boxCount] = cityBoxData[b][3] * cityScale;
            boxCount++;
        }
    }
    for (int l = 0; l < LANDMARK_COUNT; l++)
        for (int c = 0; c < 4; c++) landBox[l][c] = landmarkBox[l][c] * ratio;
    stationX = 480.0f * ratio; stationZ = 196.0f * ratio;
    workshopX = 80.0f * ratio; workshopZ = 196.0f * ratio;
    // the garage has two solid walls (north and west). The south and the east side are open, the car can drive in
    float gz = workshopZ - 12.0f;
    boxMinX[boxCount] = workshopX - 5.1f; boxMaxX[boxCount] = workshopX + 5.1f;
    boxMinZ[boxCount] = gz - 6.1f;        boxMaxZ[boxCount] = gz - 4.7f;
    boxCount++;                                                       // north wall (door and lamps)
    boxMinX[boxCount] = workshopX - 5.1f; boxMaxX[boxCount] = workshopX - 3.7f;
    boxMinZ[boxCount] = gz - 6.1f;        boxMaxZ[boxCount] = gz + 6.1f;
    boxCount++;                                                       // west wall (cabinet)
    if (modelLoaded[TIRE_MODEL])                                      // the stack of tyres next to the garage
    {
        boxMinX[boxCount] = workshopX + 7.9f; boxMaxX[boxCount] = workshopX + 9.1f;
        boxMinZ[boxCount] = workshopZ - 10.1f; boxMaxZ[boxCount] = workshopZ - 8.9f;
        boxCount++;
    }
    if (modelLoaded[BATTERY_CHARGER_MODEL])                           // the battery charger inside the garage
    {
        boxMinX[boxCount] = workshopX + 1.9f; boxMaxX[boxCount] = workshopX + 3.7f;
        boxMinZ[boxCount] = gz - 4.3f;        boxMaxZ[boxCount] = gz - 2.9f;
        boxCount++;
    }
}

// ---- file handling: best tour time ----
void loadBestTour()
{
    bestTourTime = 0.0f;
    FILE* f = fopen("citytour.txt", "r");
    if (f == NULL) return;
    float t;
    if (fscanf(f, "%f", &t) == 1 && t > 0.0f && t < 36000.0f) bestTourTime = t;
    fclose(f);
}

void saveBestTour()
{
    FILE* f = fopen("citytour.txt", "w");
    if (f == NULL) return;
    fprintf(f, "%.2f\n", bestTourTime);
    fclose(f);
}

bool inStation() { return fabsf(carX - stationX) < 12.0f && fabsf(carZ - stationZ) < 5.0f; }
bool inWorkshop() { return fabsf(carX - workshopX) < 7.0f && carZ > workshopZ - 17.0f && carZ < workshopZ + 5.0f; }   // inside the garage or in front of it

void showMessage(const char* text, float seconds)
{
    snprintf(message, sizeof(message), "%s", text);
    messageTimer = seconds;
}

void startCityTour()
{
    resetCar();
    carX = cityWidth * 0.5f; carZ = cityDepth - 30.0f;
    carAngle = -PI / 2.0f;                             // facing west
    straightenPower = 0.0f; maxCarAngle = 100.0f;
    speedLimitFactor = 0.40f;                          // city speed: the car is slower than on the highway
    controlsLocked = false; shake = 0.0f;
    fuel = 12.0f; money = 3000.0f; tourNext = 0; fault = -1; faultCount = 0;
    tourTime = 0.0f; faultTimer = 80.0f + GetRandomValue(0, 50); newTourRecord = false;
    messageTimer = 0.0f; message[0] = 0; gameTime = 0.0f;
    cameraMode = 0;
    setupCamera();
    moveCamera(0.0f, true);
    if (!cityOk && modelLoaded[CITY_MODEL]) showMessage("city.glb has a different size: collisions are off", 6.0f);
    gameState = STATE_PLAY;
}

// the car is a few circles; if a circle is inside a building box we push the car out
bool pushOutOfBoxes()
{
    bool hit = false;
    for (int c = -1; c <= 1; c++)
    {
        float cx = carX + sinf(carAngle) * c * 1.3f;
        float cz = carZ - cosf(carAngle) * c * 1.3f;
        for (int b = 0; b < boxCount; b++)
        {
            float px = keepInRange(cx, boxMinX[b], boxMaxX[b]);
            float pz = keepInRange(cz, boxMinZ[b], boxMaxZ[b]);
            float dx = cx - px, dz = cz - pz;
            float d2 = dx * dx + dz * dz;
            if (d2 >= CAR_RADIUS * CAR_RADIUS) continue;
            hit = true;
            if (d2 > 0.000001f)
            {
                float d = sqrtf(d2);
                float push = CAR_RADIUS - d;
                carX += dx / d * push;
                carZ += dz / d * push;
            }
            else                                       // the circle centre is inside the box: leave by the nearest side
            {
                float left = cx - boxMinX[b], right = boxMaxX[b] - cx, top = cz - boxMinZ[b], bottom = boxMaxZ[b] - cz;
                float m = left;
                if (right < m) m = right;
                if (top < m) m = top;
                if (bottom < m) m = bottom;
                if (m == left) carX -= left + CAR_RADIUS;
                else if (m == right) carX += right + CAR_RADIUS;
                else if (m == top) carZ -= top + CAR_RADIUS;
                else carZ += bottom + CAR_RADIUS;
            }
            cx = carX + sinf(carAngle) * c * 1.3f;
            cz = carZ - cosf(carAngle) * c * 1.3f;
        }
    }
    return hit;
}

void startFault(int which)
{
    fault = which;
    faultCount++;
    controlsLocked = true;
    stopEngine();
    playErrorSound();
    showMessage(TextFormat("BREAKDOWN: %s", faultName[which]), 4.0f);
}

void endTour()
{
    newTourRecord = false;
    if (bestTourTime <= 0.0f || tourTime < bestTourTime) { bestTourTime = tourTime; newTourRecord = true; saveBestTour(); }
    stopEngine();
    playSuccessSound();
    gameState = STATE_TOUR_RESULT;
}

void updateCityTour(float dt)
{
    tourTime += dt;
    if (messageTimer > 0.0f) messageTimer -= dt;
    if (shake > 0.0f) shake = keepInRange(shake - 0.8f * dt, 0.0f, 1.0f);

    // moving and the city
    float speedBefore = fabsf(carSpeed);
    if (pushOutOfBoxes())
    {
        carSpeed *= expf(-6.0f * dt);                  // scraping a wall slows us down
        if (speedBefore > 8.0f && shake < 0.4f)
        {
            shake = 0.4f;
            if (crashSoundLoaded) PlaySound(crashSound);
        }
    }
    carX = keepInRange(carX, 4.0f, cityWidth - 4.0f);
    carZ = keepInRange(carZ, 4.0f, cityDepth - 4.0f);

    // fuel: the faster we drive, the more we use per metre
    float v = fabsf(carSpeed);
    fuel -= (0.004f + 0.0004f * v) * v * dt;
    if (fuel <= 0.0f) { fuel = 0.0f; if (fault < 0) startFault(4); }

    // random breakdown
    faultTimer -= dt;
    if (faultTimer <= 0.0f && fault < 0 && faultCount < 3 && v > 3.0f)
    {
        startFault(GetRandomValue(0, 3));
        faultTimer = 120.0f + GetRandomValue(0, 60);
    }
    if (controlsLocked) carSpeed *= expf(-2.5f * dt);

    // petrol station: stand still inside the yellow area and hold F
    if (inStation() && fabsf(carSpeed) < 2.0f && IsKeyDown(KEY_F) && fuel < TANK_SIZE)
    {
        float litres = 8.0f * dt;
        if (money >= litres * FUEL_PRICE)
        {
            fuel += litres;
            money -= litres * FUEL_PRICE;
            if (fuel > TANK_SIZE) fuel = TANK_SIZE;
            if (fault == 4 && fuel > 1.0f) { fault = -1; controlsLocked = false; showMessage("Tank filled, you can drive again", 3.0f); }
        }
        else showMessage("Not enough money for fuel", 1.0f);
    }

    // workshop: ENTER, the mechanic repairs the car for money
    if (inWorkshop() && fabsf(carSpeed) < 2.0f && IsKeyPressed(KEY_ENTER))
    {
        if (fault >= 0)
        {
            money -= REPAIR_COST;
            if (money < 0.0f) money = 0.0f;
            if (fault == 4) fuel += 5.0f;
            fault = -1; controlsLocked = false;
            showMessage(TextFormat("Repaired for Rs %d", (int)REPAIR_COST), 3.0f);
        }
        else showMessage("Nothing to repair", 2.0f);
    }

    // tow truck: brings us to the workshop
    if (fault >= 0 && !inWorkshop() && IsKeyPressed(KEY_T))
    {
        money -= TOW_COST;
        if (money < 0.0f) money = 0.0f;
        carX = workshopX; carZ = workshopZ;
        carAngle = 0.0f; carSpeed = 0.0f; steer = 0.0f;
        moveCamera(0.0f, true);
        showMessage("The tow truck brought you to the workshop. Press ENTER to repair", 4.0f);
    }

    // landmarks, one after the other
    if (tourNext < LANDMARK_COUNT)
    {
        float d = distToBox(carX, carZ, landBox[tourNext][0], landBox[tourNext][1], landBox[tourNext][2], landBox[tourNext][3]);
        if (d < REACH)
        {
            money += LANDMARK_BONUS;
            showMessage(TextFormat("%s reached!  +Rs %d", landmarkName[tourNext], (int)LANDMARK_BONUS), 3.0f);
            if (passSoundLoaded) PlaySound(passSound);
            tourNext++;
            if (tourNext >= LANDMARK_COUNT) endTour();
        }
    }
}

// PART 11: drawing the city
void drawCityModel()
{
    Model* m = &models[CITY_MODEL];
    // scale the city and move its corner to (0,0,0)   (order: m0 m4 m8 m12, m1 m5 m9 m13, m2 m6 m10 m14, m3 m7 m11 m15)
    Matrix t = { cityScale, 0, 0, -cityMinX * cityScale,
                 0, cityScale, 0, 0,
                 0, 0, cityScale, -cityMinZ * cityScale,
                 0, 0, 0, 1 };
    for (int k = 0; k < m->meshCount; k++)
    {
        if (meshRadius[k] < 0.0f) continue;
        float dx = meshX[k] - carX, dz = meshZ[k] - carZ;
        float r = DRAW_DISTANCE + meshRadius[k];
        if (dx * dx + dz * dz > r * r) continue;
        DrawMesh(m->meshes[k], m->materials[m->meshMaterial[k]], t);
    }
}

// used when city.glb is missing: a grey ground and plain boxes
void drawSimpleCity()
{
    DrawCube(Vector3{ cityWidth / 2.0f, -0.1f, cityDepth / 2.0f }, cityWidth, 0.2f, cityDepth, Color{ 110, 112, 118, 255 });
    for (int b = 0; b < boxCount; b++)
    {
        float w = boxMaxX[b] - boxMinX[b], d = boxMaxZ[b] - boxMinZ[b];
        float h = 10.0f + 12.0f * randomFromNumber(b * 17);
        unsigned char g = (unsigned char)(150 + 70 * randomFromNumber(b * 5 + 1));
        DrawCube(Vector3{ boxMinX[b] + w / 2.0f, h / 2.0f, boxMinZ[b] + d / 2.0f }, w, h, d, Color{ g, (unsigned char)(g - 20), (unsigned char)(g - 40), 255 });
    }
}

void drawStation()
{
    float x = stationX, z = stationZ;
    DrawCube(Vector3{ x, 0.14f, z - 3.0f }, 26.0f, 0.04f, 16.0f, Color{ 70, 72, 78, 255 });          // the ground
    DrawCube(Vector3{ x, 0.16f, z }, 24.0f, 0.02f, 10.0f, Color{ 245, 200, 40, 255 });               // yellow area: stop here
    DrawCube(Vector3{ x, 0.17f, z }, 23.0f, 0.02f, 9.0f, Color{ 70, 72, 78, 255 });
    DrawCube(Vector3{ x, 5.6f, z - 4.0f }, 26.0f, 0.6f, 10.0f, Color{ 230, 230, 235, 255 });         // roof
    DrawCube(Vector3{ x, 5.6f, z - 4.0f }, 26.2f, 0.5f, 1.0f, Color{ 40, 160, 90, 255 });
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            DrawCylinder(Vector3{ x + sx * 11.0f, 0, z - 4.0f + sz * 3.5f }, 0.3f, 0.3f, 5.4f, 8, Color{ 190, 190, 195, 255 });
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    for (int p = -1; p <= 1; p++)                                                                    // three pumps
    {
        float px = x + p * 8.0f, pz = z - 4.0f;
        if (modelLoaded[PUMP_MODEL]) drawModelAt(PUMP_MODEL, px, 0.2f, pz, modelTurnFix[PUMP_MODEL], WHITE);
        else if (modelLoaded[CHARGER_MODEL]) drawModelAt(CHARGER_MODEL, px, 0.2f, pz, 180.0f + modelTurnFix[CHARGER_MODEL], WHITE);
        else DrawCube(Vector3{ px, 0.95f, pz }, 0.8f, 1.8f, 0.6f, Color{ 40, 160, 90, 255 });
    }
    if (modelLoaded[PUMP_MODEL] && modelLoaded[CHARGER_MODEL])                                       // an electric charger next to them
        drawModelAt(CHARGER_MODEL, x + 14.0f, 0.2f, z - 4.0f, 180.0f + modelTurnFix[CHARGER_MODEL], WHITE);
    rlEnableBackfaceCulling();
    DrawCube(Vector3{ x + 15.0f, 4.5f, z - 8.0f }, 0.6f, 9.0f, 0.6f, Color{ 90, 94, 102, 255 });     // sign post
    DrawCube(Vector3{ x + 15.0f, 8.6f, z - 8.0f }, 3.4f, 2.0f, 0.5f, Color{ 40, 160, 90, 255 });
}

void drawWorkshop()
{
    float x = workshopX, z = workshopZ;
    float gz = z - 12.0f;                                                                          // middle of the garage building
    DrawCube(Vector3{ x, 0.14f, z - 6.0f }, 26.0f, 0.04f, 26.0f, Color{ 70, 72, 78, 255 });          // ground
    DrawCube(Vector3{ x, 0.16f, z - 0.5f }, 14.0f, 0.02f, 11.0f, Color{ 60, 130, 230, 255 });        // blue area: repairs are possible here
    DrawCube(Vector3{ x, 0.17f, z - 0.5f }, 13.0f, 0.02f, 10.0f, Color{ 70, 72, 78, 255 });
    if (modelLoaded[GARAGE_MODEL])
    {
        rlDrawRenderBatchActive();                                                                  // draw what is waiting first
        rlDisableBackfaceCulling();                                                                 // the walls of this model are one-sided
        drawModelAt(GARAGE_MODEL, x, 0.0f, gz, modelTurnFix[GARAGE_MODEL], WHITE);                  // the garage: open to the south
        rlEnableBackfaceCulling();
    }
    else                                                                                            // simple garage: a back wall, a west wall and a roof
    {
        DrawCube(Vector3{ x, 2.5f, gz - 5.4f }, 10.2f, 5.0f, 0.8f, Color{ 205, 205, 210, 255 });
        DrawCube(Vector3{ x - 4.8f, 2.5f, gz }, 0.8f, 5.0f, 12.0f, Color{ 190, 190, 196, 255 });
        DrawCube(Vector3{ x, 5.2f, gz }, 11.0f, 0.4f, 12.4f, Color{ 60, 130, 230, 255 });
        DrawCube(Vector3{ x + 1.0f, 1.8f, gz - 4.95f }, 5.0f, 3.6f, 0.15f, Color{ 50, 54, 64, 255 });
    }
    if (modelLoaded[TIRE_MODEL])                                                                    // a stack of old tyres
        for (int i = 0; i < 3; i++) drawModelAt(TIRE_MODEL, x + 8.5f, 0.2f + i * 0.23f, z - 9.5f, i * 40.0f, WHITE);
    if (modelLoaded[BATTERY_CHARGER_MODEL])                                                         // battery charger inside the garage
    {
        rlDrawRenderBatchActive();
        rlDisableBackfaceCulling();
        drawModelAt(BATTERY_CHARGER_MODEL, x + 2.8f, 0.0f, gz - 3.6f, modelTurnFix[BATTERY_CHARGER_MODEL], WHITE);
        rlEnableBackfaceCulling();
    }
    DrawCube(Vector3{ x + 8.5f, 3.0f, z + 3.0f }, 0.3f, 6.0f, 0.3f, Color{ 90, 94, 102, 255 });       // sign post
    DrawCube(Vector3{ x + 8.5f, 6.4f, z + 3.0f }, 2.6f, 1.6f, 0.3f, Color{ 245, 200, 40, 255 });
}

void drawTargetBeam()
{
    if (tourNext >= LANDMARK_COUNT) return;
    float cx = (landBox[tourNext][0] + landBox[tourNext][2]) * 0.5f;
    float cz = (landBox[tourNext][1] + landBox[tourNext][3]) * 0.5f;
    if ((carX - cx) * (carX - cx) + (carZ - cz) * (carZ - cz) < 20.0f * 20.0f) return;      // too close: it would be in the way
    DrawCylinder(Vector3{ cx, 0, cz }, 0.5f, 0.5f, 45.0f, 8, Color{ 255, 200, 60, 255 });
    DrawCylinder(Vector3{ cx, 45.0f, cz }, 3.0f, 0.0f, 5.0f, 8, Color{ 255, 90, 60, 255 });         // arrow pointing down
}

// a tall coloured pole at each special place: green = petrol station, blue = workshop.
// It is only drawn when we are far away (over 35 m): close up it would stand in front of the camera.
void drawPlaceBeams()
{
    float ds = sqrtf((carX - stationX) * (carX - stationX) + (carZ - stationZ) * (carZ - stationZ));
    float dw = sqrtf((carX - workshopX) * (carX - workshopX) + (carZ - workshopZ) * (carZ - workshopZ));
    if (ds > 35.0f) DrawCylinder(Vector3{ stationX + 9.0f, 0, stationZ + 3.0f }, 0.3f, 0.3f, 30.0f, 8, Color{ 40, 200, 110, 255 });
    if (dw > 35.0f) DrawCylinder(Vector3{ workshopX + 9.0f, 0, workshopZ + 3.0f }, 0.3f, 0.3f, 30.0f, 8, Color{ 70, 140, 235, 255 });
}

void drawCityWorld()
{
    BeginMode3D(camera);
    DrawPlane(Vector3{ carX, -0.3f, carZ }, Vector2{ 1600, 1600 }, Color{ 96, 150, 70, 255 });
    if (modelLoaded[CITY_MODEL]) drawCityModel(); else drawSimpleCity();
    drawStation();
    drawWorkshop();
    drawPlaceBeams();
    drawTargetBeam();
    drawCar(selectedCar, carX, carZ, carAngle, carColor[selectedCar]);
    EndMode3D();
}


// PART 12: City Tour HUD

void drawCityMinimap(float x, float y, float w)
{
    float sc = w / cityWidth, h = cityDepth * sc;
    DrawRectangle((int)x - 4, (int)y - 4, (int)w + 8, (int)h + 8, Fade(BLACK, 0.55f));
    DrawRectangle((int)x, (int)y, (int)w, (int)h, Color{ 80, 84, 92, 255 });
    for (int b = 0; b < boxCount; b++)
        DrawRectangle((int)(x + boxMinX[b] * sc), (int)(y + boxMinZ[b] * sc), (int)((boxMaxX[b] - boxMinX[b]) * sc) + 1, (int)((boxMaxZ[b] - boxMinZ[b]) * sc) + 1, Color{ 150, 154, 164, 255 });
    DrawRectangle((int)(x + stationX * sc) - 5, (int)(y + stationZ * sc) - 4, 10, 8, Color{ 40, 190, 100, 255 });
    DrawRectangle((int)(x + workshopX * sc) - 5, (int)(y + workshopZ * sc) - 4, 10, 8, Color{ 60, 130, 230, 255 });
    drawText("F", x + stationX * sc - 4, y + stationZ * sc - 22, 16, Color{ 40, 220, 110, 255 });     // fuel
    drawText("W", x + workshopX * sc - 6, y + workshopZ * sc - 22, 16, Color{ 120, 180, 255, 255 });   // workshop
    for (int l = 0; l < LANDMARK_COUNT; l++)
    {
        float cx = x + (landBox[l][0] + landBox[l][2]) * 0.5f * sc, cz = y + (landBox[l][1] + landBox[l][3]) * 0.5f * sc;
        if (l < tourNext) DrawCircle((int)cx, (int)cz, 3, Color{ 90, 230, 120, 255 });
        else if (l == tourNext) DrawCircle((int)cx, (int)cz, 6, Color{ 255, 90, 60, 255 });
        else DrawCircle((int)cx, (int)cz, 3, WHITE);
    }
    DrawCircle((int)(x + carX * sc), (int)(y + carZ * sc), 5, Color{ 255, 220, 40, 255 });
}

// an arrow on the screen that points to a place (0 = straight ahead)
void drawArrow(float tx, float tz, float cx, float cy, Color c, const char* label)
{
    float turn = fixAngle(atan2f(tx - carX, -(tz - carZ)) - carAngle);
    float dx = sinf(turn), dy = -cosf(turn);
    Vector2 tip = { cx + dx * 34.0f, cy + dy * 34.0f }, tail = { cx - dx * 30.0f, cy - dy * 30.0f };
    DrawLineEx(tail, tip, 7.0f, c);
    DrawLineEx(tip, Vector2{ tip.x - sinf(turn + 0.5f) * 18.0f, tip.y + cosf(turn + 0.5f) * 18.0f }, 7.0f, c);
    DrawLineEx(tip, Vector2{ tip.x - sinf(turn - 0.5f) * 18.0f, tip.y + cosf(turn - 0.5f) * 18.0f }, 7.0f, c);
    if (label != NULL)
    {
        float d = sqrtf((tx - carX) * (tx - carX) + (tz - carZ) * (tz - carZ));
        drawTextCentered(TextFormat("%s  %d m", label, (int)d), cx, cy + 48.0f, 20, c);
    }
}

void drawArrowToTarget()
{
    if (tourNext >= LANDMARK_COUNT) return;
    float tx = (landBox[tourNext][0] + landBox[tourNext][2]) * 0.5f;
    float tz = (landBox[tourNext][1] + landBox[tourNext][3]) * 0.5f;
    drawArrow(tx, tz, SCREEN_W / 2.0f, 190.0f, Color{ 255, 200, 60, 255 }, NULL);
}

void drawCityInfo()
{
    float cx = SCREEN_W / 2.0f;
    // fuel and money (left)
    DrawRectangle(20, 56, 250, 104, Fade(BLACK, 0.5f));
    drawText("FUEL", 32, 62, 20, Color{ 190, 195, 210, 255 });
    bool low = (fuel < 5.0f);
    Color fc = low ? ((int)(tourTime * 3) % 2 == 0 ? Color{ 255, 70, 60, 255 } : Color{ 120, 40, 40, 255 }) : Color{ 90, 200, 255, 255 };
    DrawRectangle(32, 88, 226, 16, Fade(WHITE, 0.2f));
    DrawRectangle(32, 88, (int)(226 * fuel / TANK_SIZE), 16, fc);
    drawText(TextFormat("%.1f L", fuel), 150, 62, 20, fc);
    drawText(TextFormat("Money  Rs %d", (int)money), 32, 112, 24, WHITE);
    drawText(TextFormat("Time  %s", timeText(tourTime)), 32, 136, 20, Color{ 190, 195, 210, 255 });

    // next place (top centre)
    DrawRectangle((int)cx - 190, 12, 380, 78, Fade(BLACK, 0.5f));
    if (tourNext < LANDMARK_COUNT)
    {
        float d = distToBox(carX, carZ, landBox[tourNext][0], landBox[tourNext][1], landBox[tourNext][2], landBox[tourNext][3]);
        drawTextCentered(TextFormat("Place %d / %d", tourNext + 1, LANDMARK_COUNT), cx, 14, 20, Color{ 190, 195, 210, 255 });
        drawTextCentered(landmarkName[tourNext], cx, 36, 36, Color{ 255, 210, 80, 255 });
        drawTextCentered(TextFormat("%d m", (int)d), cx, 70, 18, WHITE);
    }
    drawArrowToTarget();
    if (fuel < 8.0f && fault < 0)                                    // low fuel: show the way to the petrol station
        drawArrow(stationX, stationZ, cx + 170.0f, 190.0f, Color{ 40, 220, 110, 255 }, "FUEL");
    drawCityMinimap(SCREEN_W - 296.0f, 14.0f, 280.0f);
    if (modelLoaded[CITY_MODEL] && (hiddenMeshes != CITY_HIDE_COUNT || !cityOk))     // small help text if city.glb is not the expected file
        drawText(TextFormat("city.glb: %d of %d glow meshes hidden, size ok: %s", hiddenMeshes, CITY_HIDE_COUNT, cityOk ? "yes" : "no"), 20, SCREEN_H - 100, 18, Color{ 255, 160, 120, 255 });
    if (!modelLoaded[CITY_MODEL])
        drawText("city.glb not found: plain boxes are shown", 20, SCREEN_H - 100, 18, Color{ 255, 160, 120, 255 });

    // hints
    float by = SCREEN_H - 110.0f;
    if (fault >= 0)
    {
        DrawRectangle((int)cx - 370, (int)by - 90, 740, 160, Fade(Color{ 120, 20, 20, 255 }, 0.82f));
        drawTextCentered(TextFormat("BREAKDOWN: %s", faultName[fault]), cx, by - 84, 36, WHITE);
        if (inWorkshop()) drawTextCentered(TextFormat("Press ENTER: the mechanic repairs the car  (Rs %d)", (int)REPAIR_COST), cx, by - 30, 28, Color{ 255, 230, 120, 255 });
        else drawTextCentered(TextFormat("Press T: tow truck to the workshop  (Rs %d)", (int)TOW_COST), cx, by - 30, 28, Color{ 255, 230, 120, 255 });
        if (!inWorkshop()) drawTextCentered("At the workshop (blue area) press ENTER to repair", cx, by + 8, 22, WHITE);
    }
    else if (inStation())
        drawTextCentered(TextFormat("Stand still and hold F to refuel  (Rs %d per litre)", (int)FUEL_PRICE), cx, by, 28, Color{ 255, 220, 90, 255 });
    else if (inWorkshop())
        drawTextCentered(TextFormat("Press ENTER to use the workshop  (repair costs Rs %d)", (int)REPAIR_COST), cx, by, 28, Color{ 120, 180, 255, 255 });
    if (messageTimer > 0.0f && fault < 0)
        drawTextCentered(message, cx, 260.0f - (2.0f - keepInRange(messageTimer, 0.0f, 2.0f)) * 10.0f, 40, Fade(Color{ 255, 220, 90, 255 }, keepInRange(messageTimer / 0.5f, 0.0f, 1.0f)));
}


// PART 13: screens (menu, play, pause, result) and main
// switches between a window and full screen at the size of the monitor (also on the F11 key)
void toggleFullscreen()
{
    if (IsWindowFullscreen())
    {
        ToggleFullscreen();
        SetWindowSize(1280, 720);
    }
    else
    {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    }
    SCREEN_W = GetScreenWidth();
    SCREEN_H = GetScreenHeight();
}

void menuScreen(float dt, bool* quit)
{
    menuAngle += 25.0f * dt;
    if (IsKeyPressed(KEY_RIGHT)) { selectedCar = (selectedCar + 1) % NUM_CARS; playClickSound(); }
    if (IsKeyPressed(KEY_LEFT)) { selectedCar = (selectedCar + NUM_CARS - 1) % NUM_CARS; playClickSound(); }

    // dark showroom background
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, Color{ 12, 14, 22, 255 }, Color{ 34, 40, 58, 255 });
    Camera3D mc = { Vector3{ -2.2f, 2.3f, 7.5f }, Vector3{ -2.2f, 0.9f, 0.0f }, Vector3{ 0, 1, 0 }, 45.0f, CAMERA_PERSPECTIVE };
    BeginMode3D(mc);
    DrawPlane(Vector3{ 0, -0.1f, 0 }, Vector2{ 60, 60 }, Color{ 18, 20, 30, 255 });
    DrawCylinder(Vector3{ 0, -0.1f, 0 }, 3.3f, 3.3f, 0.04f, 48, Color{ 230, 60, 50, 255 });
    DrawCylinder(Vector3{ 0, -0.1f, 0 }, 3.2f, 3.2f, 0.1f, 48, Color{ 30, 34, 46, 255 });
    drawCar(selectedCar, 0.0f, 0.0f, -menuAngle * DEG2RAD, carColor[selectedCar]);
    EndMode3D();

    // title
    drawText("CITY TOUR", 60, 40, 64, WHITE);
    drawText("Drive. Explore.", 64, 108, 26, Color{ 230, 60, 50, 255 });

    // buttons
    float bx = 60, by = 170, bw = 330, bh = 54, gap = 12;
    if (button("Start City Tour", bx, by + 0 * (bh + gap), bw, bh) || IsKeyPressed(KEY_ENTER)) startCityTour();
    if (button("Quit", bx, by + 1 * (bh + gap), bw, bh)) *quit = true;

    // choosing the car
    float px = 80, py = 330;
    DrawRectangle((int)px - 20, (int)py - 20, 470, 160, Fade(BLACK, 0.45f));
    if (button("<", px, py - 4, 46, 46)) selectedCar = (selectedCar + NUM_CARS - 1) % NUM_CARS;
    if (button(">", px + 404, py - 4, 46, 46)) selectedCar = (selectedCar + 1) % NUM_CARS;
    drawTextCentered(carName[selectedCar], px + 225, py + 2, 34, WHITE);
    drawStatBar("Speed", px + 70, py + 52, carStats[selectedCar][STAT_SPEED] / 80.0f);
    drawStatBar("Accel", px + 70, py + 76, carStats[selectedCar][STAT_ACCEL] / 20.0f);
    drawStatBar("Handling", px + 70, py + 100, carStats[selectedCar][STAT_GRIP] / 1.3f);

    if (showFps) DrawFPS(SCREEN_W - 100, 14);
}

void playScreen(float dt)
{
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) { gameState = STATE_PAUSE; stopEngine(); return; }
    if (IsKeyPressed(KEY_C)) cameraMode = (cameraMode + 1) % 3;
    gameTime += dt;
    updateCar(dt);
    updateCityTour(dt);
    moveCamera(dt, false);
    if (!controlsLocked) playEngine();
    drawSky();
    drawCityWorld();
    drawBasicHud();
    drawCityInfo();
}

void tourResultScreen()
{
    drawSky();
    drawCityWorld();
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.7f));
    float cx = SCREEN_W / 2.0f;
    drawTextCentered("CITY TOUR COMPLETE", cx, 110, 70, Color{ 255, 210, 80, 255 });
    drawTextCentered(TextFormat("Time: %s", timeText(tourTime)), cx, 220, 52, WHITE);
    drawTextCentered(TextFormat("Breakdowns: %d     Money left: Rs %d     Fuel left: %.1f L", faultCount, (int)money, fuel), cx, 295, 26, Color{ 200, 205, 220, 255 });
    if (newTourRecord) drawTextCentered("NEW BEST TIME!", cx, 340, 40, Color{ 90, 230, 120, 255 });
    else drawTextCentered(TextFormat("Best time: %s", timeText(bestTourTime)), cx, 345, 30, Color{ 200, 205, 220, 255 });
    float bx = cx - 165;
    if (button("Drive again  (Enter)", bx, 430, 330, 54) || IsKeyPressed(KEY_ENTER)) startCityTour();
    if (button("Main menu", bx, 500, 330, 54)) gameState = STATE_MENU;
}

void pauseScreen()
{
    drawSky();
    drawCityWorld();
    drawBasicHud();
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.55f));
    drawTextCentered("PAUSED", SCREEN_W / 2.0f, 180, 72, WHITE);
    float bx = SCREEN_W / 2.0f - 165;
    if (button("Resume", bx, 290, 330, 54) || IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) gameState = STATE_PLAY;
    if (button("Restart", bx, 360, 330, 54)) startCityTour();
    if (button("Main menu", bx, 430, 330, 54)) gameState = STATE_MENU;
}

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT); //| FLAG_WINDOW_RESIZABLE means that the window can be resized, but it is not needed here why not needed here as the game is designed to be played in full screen and | symbol is used to combine multiple flags together and flag means a setting or option that can be turned on or off or roman urdu me is ka matlab hai ke ye game me kuch settings ya options hain jo on ya off ki ja sakti hain jese ke MSAA 4X aur VSync. MSAA 4X is a setting that improves the visual quality of the game by reducing jagged edges on objects, while VSync is a setting that synchronizes the game's frame rate with the monitor's refresh rate to prevent screen tearing. The FLAG_WINDOW_RESIZABLE option allows the player to resize the game window, but it is not necessary for this game as it is designed to be played in full screen mode.
    InitWindow(SCREEN_W, SCREEN_H, "City Tour");
    SetExitKey(KEY_NULL);                 // Esc must not close the window
    SetTargetFPS(60);
    toggleFullscreen();                   // the game starts in full screen (F11 switches back)
    findResources(); 
    InitAudioDevice();
    loadAssets();
    loadBestTour();
    setupCamera();

    bool quit = false;
    while (!quit && !WindowShouldClose())
    {
        float dt = GetFrameTime(); //dt means delta time and get frame time gives time of every frame
        if (dt > 0.05f) dt = 0.05f;       // avoid huge jumps if the window is dragged
        SCREEN_W = GetScreenWidth();      // follow the real size of the window
        SCREEN_H = GetScreenHeight();
        if (IsKeyPressed(KEY_F11)) toggleFullscreen();

        BeginDrawing();
        ClearBackground(BLACK);
        if (gameState == STATE_MENU)        menuScreen(dt, &quit);
        else if (gameState == STATE_PLAY)        playScreen(dt);
        else if (gameState == STATE_PAUSE)       pauseScreen();
        else if (gameState == STATE_TOUR_RESULT) tourResultScreen();
        EndDrawing();
    }

    freeCity();
    unloadAssets();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}