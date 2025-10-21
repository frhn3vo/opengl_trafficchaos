#include <GL/glut.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <windows.h>e
#include <conio.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")
using namespace std;

// Game state
enum GameState { MENU, PLAYING, LEVEL_COMPLETE, GAME_OVER };
GameState currentState = MENU;
enum Environment { NORMAL, DESERT, TROPICAL };
Environment currentEnvironment = NORMAL;

// Star structure for background
struct Star {
    float x, y;
    float speed;
    float brightness;
};

std::vector<Star> stars;
const int NUM_STARS = 100;

// Car and road properties
float carX = 0.0f;
float carSpeed = 0.02f;
float roadOffset = 0.0f;
float carWidth = 0.1f;
float carHeight = 0.2f;
float currentBoost = 1.0f;

// High score tracking
int highScore = 0;
int currentScore = 0;

// Environment-based spawn rates
float baseSpawnInterval = 4.0f;
float environmentSpawnMultiplier = 1.0f;
float spawnInterval = baseSpawnInterval;
float coinSpawnInterval = 2.0f;
float potholeSpawnInterval = 10.0f;

// Scenery - Trees/Cacti/Palms
struct Scenery {
    float x, y, width, height;
};
std::vector<Scenery> leftScenery, rightScenery;
float scenerySpawnInterval = 3.0f;
float timeSinceLastScenerySpawn = 0.0f;

// Obstacles and collectibles
struct Obstacle {
    float x, y, width, height;
    bool passed;
};
struct Coin {
    float x, y, radius;
};
struct Pothole {
    float x, y, width, height;
    bool active;
};

std::vector<Obstacle> obstacles;
std::vector<Coin> coins;
std::vector<Pothole> potholes;

float timeSinceLastSpawn = 0.0f;
float timeSinceLastCoin = 0.0f;
float timeSinceLastPotholeSpawn = 0.0f;

// Utility Functions
float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void playThemeSong() {
    PlaySound(TEXT("theme.wav"), NULL, SND_FILENAME | SND_SYNC | SND_LOOP);
}

void stopThemeSong() {
    PlaySound(NULL, NULL, 0);
}

void saveHighScore(int score) {
    ofstream file("highscore.txt");
    if (file.is_open()) {
        file << score;
        file.close();
    }
    else {
        cerr << "Error: Could not save high score\n";
    }
}

int loadHighScore() {
    ifstream file("highscore.txt");
    int score = 0;
    if (file.is_open()) {
        file >> score;
        file.close();
    }
    return score;
}

void initStars() {
    stars.clear();
    for (int i = 0; i < NUM_STARS; i++) {
        Star star;
        star.x = randomFloat(-1.0f, 1.0f);
        star.y = randomFloat(-1.0f, 1.0f);
        star.speed = randomFloat(0.001f, 0.003f);
        star.brightness = randomFloat(0.3f, 1.0f);
        stars.push_back(star);
    }
}

// Drawing Functions
void drawRect(float x, float y, float width, float height, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x - width / 2, y - height / 2);
    glVertex2f(x + width / 2, y - height / 2);
    glVertex2f(x + width / 2, y + height / 2);
    glVertex2f(x - width / 2, y + height / 2);
    glEnd();
}

void drawCircle(float x, float y, float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 100; ++i) {
        float angle = 2.0f * 3.1415926f * i / 100;
        glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
    }
    glEnd();
}

void drawCactus(float x, float y, float width, float height) {
    drawRect(x, y, width / 3, height, 0.0f, 0.5f, 0.0f);
    drawRect(x - width / 3, y + height * 0.3f, width / 4, height * 0.3f, 0.0f, 0.5f, 0.0f);
    drawRect(x + width / 3, y + height * 0.2f, width / 4, height * 0.4f, 0.0f, 0.5f, 0.0f);
}

void drawPalmTree(float x, float y, float width, float height) {
    drawRect(x, y, width / 4, height * 0.8f, 0.6f, 0.3f, 0.1f);
    for (int i = 0; i < 6; ++i) {
        float angle = i * 60.0f * 3.14159f / 180.0f;
        float leafLength = height * 0.6f;
        glColor3f(0.0f, 0.8f, 0.2f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x, y + height * 0.7f);
        glVertex2f(x + cos(angle) * leafLength, y + height * 0.7f + sin(angle) * leafLength);
        glVertex2f(x + cos(angle + 0.2f) * leafLength, y + height * 0.7f + sin(angle + 0.2f) * leafLength);
        glEnd();
    }
}

void drawTree(float x, float y, float width, float height) {
    drawRect(x, y, width / 3, height * 0.4f, 0.4f, 0.25f, 0.1f);
    drawRect(x, y + height * 0.4f, width, height * 0.6f, 0.0f, 0.5f, 0.0f);
}

void drawMenuBackground() {
    // Draw gradient background
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.2f); // Dark blue at bottom
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glColor3f(0.1f, 0.1f, 0.3f); // Lighter blue at top
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Draw stars
    for (auto& star : stars) {
        // Update star position
        star.y -= star.speed;
        if (star.y < -1.0f) {
            star.y = 1.0f;
            star.x = randomFloat(-1.0f, 1.0f);
            star.speed = randomFloat(0.001f, 0.003f);
        }

        // Draw star
        glColor3f(star.brightness, star.brightness, star.brightness);
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        glVertex2f(star.x, star.y);
        glEnd();
    }
}

void drawCar() {
    drawRect(carX, -0.8f, carWidth, carHeight, 1.0f, 0.0f, 0.0f);
    drawCircle(carX - carWidth / 3, -0.8f - carHeight / 3, 0.02f, 0.2f, 0.2f, 0.2f);
    drawCircle(carX + carWidth / 3, -0.8f - carHeight / 3, 0.02f, 0.2f, 0.2f, 0.2f);
}

void drawObstacles() {
    for (const auto& obs : obstacles) {
        drawRect(obs.x, obs.y, obs.width, obs.height, 1.0f, 0.3f, 0.3f);
    }
}

void drawCoins() {
    for (const auto& coin : coins) {
        drawCircle(coin.x, coin.y, coin.radius, 1.0f, 1.0f, 0.0f);
    }
}

void drawPotholes() {
    for (const auto& pothole : potholes) {
        if (pothole.active) {
            drawCircle(pothole.x, pothole.y, pothole.width / 2, 0.1f, 0.1f, 0.1f);
        }
    }
}

void drawScenery() {
    leftScenery.erase(
        std::remove_if(leftScenery.begin(), leftScenery.end(),
            [](Scenery& s) { return s.y < -1.2f; }),
        leftScenery.end());

    rightScenery.erase(
        std::remove_if(rightScenery.begin(), rightScenery.end(),
            [](Scenery& s) { return s.y < -1.2f; }),
        rightScenery.end());

    switch (currentEnvironment) {
    case DESERT:
        for (auto& s : leftScenery) {
            drawCactus(s.x, s.y, s.width, s.height);
        }
        for (auto& s : rightScenery) {
            drawCactus(s.x, s.y, s.width, s.height);
        }
        break;
    case TROPICAL:
        for (auto& s : leftScenery) {
            drawPalmTree(s.x, s.y, s.width, s.height);
        }
        for (auto& s : rightScenery) {
            drawPalmTree(s.x, s.y, s.width, s.height);
        }
        break;
    default: // NORMAL
        for (auto& s : leftScenery) {
            drawTree(s.x, s.y, s.width, s.height);
        }
        for (auto& s : rightScenery) {
            drawTree(s.x, s.y, s.width, s.height);
        }
        break;
    }
}

void drawRoad() {
    // Background
    switch (currentEnvironment) {
    case DESERT:
        glColor3f(0.9f, 0.8f, 0.5f);
        break;
    case TROPICAL:
        glColor3f(0.3f, 0.8f, 0.3f);
        break;
    default:
        glColor3f(0.2f, 0.5f, 0.2f);
        break;
    }

    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Road
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-0.4f, -1.0f);
    glVertex2f(0.4f, -1.0f);
    glVertex2f(0.4f, 1.0f);
    glVertex2f(-0.4f, 1.0f);
    glEnd();

    // Road lines
    glColor3f(1.0f, 1.0f, 1.0f);
    for (float i = -1.0f; i < 1.0f; i += 0.2f) {
        glBegin(GL_QUADS);
        glVertex2f(-0.02f, i + roadOffset);
        glVertex2f(0.02f, i + roadOffset);
        glVertex2f(0.02f, i + 0.1f + roadOffset);
        glVertex2f(-0.02f, i + 0.1f + roadOffset);
        glEnd();
    }
}

void displayText(float x, float y, const string& text) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

void updateEnvironment() {
    int scorePhase = currentScore % 600;

    if (scorePhase < 200) {
        if (currentEnvironment != NORMAL) {
            currentEnvironment = NORMAL;
            environmentSpawnMultiplier = 2.5f;
            spawnInterval = baseSpawnInterval * environmentSpawnMultiplier;
            leftScenery.clear();
            rightScenery.clear();
        }
    }
    else if (scorePhase < 400) {
        if (currentEnvironment != DESERT) {
            currentEnvironment = DESERT;
            environmentSpawnMultiplier = 1.7f;
            spawnInterval = baseSpawnInterval * environmentSpawnMultiplier;
            leftScenery.clear();
            rightScenery.clear();
        }
    }
    else {
        if (currentEnvironment != TROPICAL) {
            currentEnvironment = TROPICAL;
            environmentSpawnMultiplier = 1.5f;
            spawnInterval = baseSpawnInterval * environmentSpawnMultiplier;
            leftScenery.clear();
            rightScenery.clear();
        }
    }
}

// Function to check if an obstacle, coin, or pothole is too close to others
bool checkCollisionWithOtherObjects(float x, float y, float radius, const vector<Obstacle>& obstacles, const vector<Coin>& coins, const vector<Pothole>& potholes) {
    // Check against obstacles
    for (const auto& obs : obstacles) {
        float distance = sqrt(pow(obs.x - x, 2) + pow(obs.y - y, 2));
        if (distance < (obs.width / 2 + radius)) {
            return true; // Overlap with obstacle
        }
    }

    // Check against coins
    for (const auto& coin : coins) {
        float distance = sqrt(pow(coin.x - x, 2) + pow(coin.y - y, 2));
        if (distance < (coin.radius + radius)) {
            return true; // Overlap with coin
        }
    }

    // Check against potholes
    for (const auto& pothole : potholes) {
        float distance = sqrt(pow(pothole.x - x, 2) + pow(pothole.y - y, 2));
        if (distance < (pothole.width / 2 + radius)) {
            return true; // Overlap with pothole
        }
    }

    return false;
}

// Function to spawn obstacles, coins, or potholes without overlap
void spawnObstacleOrCoin() {
    timeSinceLastSpawn += 0.1f;
    timeSinceLastCoin += 0.1f;
    timeSinceLastPotholeSpawn += 0.1f;

    float currentSpawnThreshold = spawnInterval;
    float randomVariation = randomFloat(-0.5f, 0.5f);
    currentSpawnThreshold += randomVariation;

    if (timeSinceLastSpawn >= currentSpawnThreshold) {
        int obstacleCount = 1;

        switch (currentEnvironment) {
        case DESERT:
            obstacleCount = randomFloat(0, 1) > 0.5f ? 2 : 1;
            break;
        case TROPICAL:
            obstacleCount = randomFloat(0, 1) > 0.3f ? 2 : 1;
            break;
        default:
            obstacleCount = 1;
            break;
        }

        for (int i = 0; i < obstacleCount; i++) {
            float x = randomFloat(-0.3f, 0.3f);
            if (i > 0) {
                x = x > 0 ? max(x - 0.2f, -0.3f) : min(x + 0.2f, 0.3f);
            }
            if (!checkCollisionWithOtherObjects(x, 1.2f + (i * 0.3f), carWidth / 2, obstacles, coins, potholes)) {
                obstacles.push_back({ x, 1.2f + (i * 0.3f), carWidth, carHeight, false });
            }
        }
        timeSinceLastSpawn = 0.0f;
    }

    if (timeSinceLastCoin >= coinSpawnInterval) {
        float x = randomFloat(-0.3f, 0.3f);
        if (!checkCollisionWithOtherObjects(x, 1.2f, 0.03f, obstacles, coins, potholes)) {
            coins.push_back({ x, 1.2f, 0.03f });
        }
        timeSinceLastCoin = 0.0f;
    }

    if (timeSinceLastPotholeSpawn >= potholeSpawnInterval) {
        if ((currentEnvironment == DESERT && randomFloat(0, 1) > 0.3f) ||
            (currentEnvironment == TROPICAL && randomFloat(0, 1) > 0.5f) ||
            (currentEnvironment == NORMAL && randomFloat(0, 1) > 0.7f)) {
            float x = randomFloat(-0.3f, 0.3f);
            if (!checkCollisionWithOtherObjects(x, 1.2f, 0.05f, obstacles, coins, potholes)) {
                potholes.push_back({ x, 1.2f, 0.1f, 0.1f, true });
            }
        }
        timeSinceLastPotholeSpawn = 0.0f;
    }
}

void updateGame(int value) {
    if (currentState == PLAYING) {
        updateEnvironment();

        roadOffset -= carSpeed * currentBoost;
        if (roadOffset < -0.2f) roadOffset = 0.0f;

        for (auto& obs : obstacles) {
            obs.y -= carSpeed * currentBoost;
        }

        for (auto& pothole : potholes) {
            pothole.y -= carSpeed * currentBoost;

            if (pothole.active &&
                pothole.y - pothole.height / 2 < -0.8f + carHeight / 2 &&
                pothole.y + pothole.height / 2 > -0.8f - carHeight / 2 &&
                pothole.x - pothole.width / 2 < carX + carWidth / 2 &&
                pothole.x + pothole.width / 2 > carX - carWidth / 2) {
                currentBoost = 0.5f;
                pothole.active = false;
            }
        }

        potholes.erase(
            std::remove_if(potholes.begin(), potholes.end(),
                [](const Pothole& pothole) { return pothole.y < -1.2f; }),
            potholes.end());

        for (auto& coin : coins) {
            coin.y -= carSpeed * currentBoost;
        }

        for (auto& s : leftScenery) {
            s.y -= carSpeed * currentBoost;
        }

        for (auto& s : rightScenery) {
            s.y -= carSpeed * currentBoost;
        }

        timeSinceLastScenerySpawn += 0.1f;
        if (timeSinceLastScenerySpawn >= scenerySpawnInterval) {
            leftScenery.push_back({ -0.7f, 1.2f, 0.2f, 0.3f });
            rightScenery.push_back({ 0.7f, 1.2f, 0.2f, 0.3f });
            timeSinceLastScenerySpawn = 0.0f;
        }

        obstacles.erase(
            std::remove_if(obstacles.begin(), obstacles.end(),
                [](Obstacle& obs) { return obs.y < -1.2f; }),
            obstacles.end());
        coins.erase(
            std::remove_if(coins.begin(), coins.end(),
                [](Coin& coin) { return coin.y < -1.2f; }),
            coins.end());

        spawnObstacleOrCoin();

        // Collision detection with obstacles
        for (auto& obs : obstacles) {
            if (obs.y - obs.height / 2 < -0.8f + carHeight / 2 &&
                obs.y + obs.height / 2 > -0.8f - carHeight / 2 &&
                obs.x - obs.width / 2 < carX + carWidth / 2 &&
                obs.x + obs.width / 2 > carX - carWidth / 2) {
                currentState = GAME_OVER;
                if (currentScore > highScore) {
                    highScore = currentScore;
                    saveHighScore(highScore);
                }
                break;
            }
        }

        // Coin collection
        coins.erase(
            std::remove_if(coins.begin(), coins.end(),
                [](Coin& coin) {
                    if (coin.y - coin.radius < -0.8f + carHeight / 2 &&
                        coin.y + coin.radius > -0.8f - carHeight / 2 &&
                        coin.x - coin.radius < carX + carWidth / 2 &&
                        coin.x + coin.radius > carX - carWidth / 2) {
                        currentScore += 20;
                        return true;
                    }
                    return false;
                }),
            coins.end());

        // Score for passing obstacles
        for (auto& obs : obstacles) {
            if (obs.y + obs.height / 2 < -0.8f && !obs.passed) {
                currentScore += 10;
                obs.passed = true;
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, updateGame, 0);
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (currentState == MENU || currentState == GAME_OVER) {
        drawMenuBackground();

        if (currentState == MENU) {
            // Add a semi-transparent overlay for better text readability
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            glBegin(GL_QUADS);
            glVertex2f(-1.0f, -1.0f);
            glVertex2f(1.0f, -1.0f);
            glVertex2f(1.0f, 1.0f);
            glVertex2f(-1.0f, 1.0f);
            glEnd();

            displayText(-0.2f, 0.4f, "Welcome To Traffic Chaos");
            displayText(-0.2f, 0.2f, "Press 'G' to Start");
            displayText(-0.2f, 0.0f, "Controls:");
            displayText(-0.2f, -0.1f, "A/D - Move Left/Right");
            displayText(-0.2f, -0.2f, "W - Boost, S - Brake");
            displayText(-0.2f, -0.4f, "Press 'E' to Exit");
            displayText(-0.2f, -0.6f, "High Score: " + to_string(highScore));
        }
        else if (currentState == GAME_OVER) {
            // Add a semi-transparent overlay for better text readability
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            glBegin(GL_QUADS);
            glVertex2f(-1.0f, -1.0f);
            glVertex2f(1.0f, -1.0f);
            glVertex2f(1.0f, 1.0f);
            glVertex2f(-1.0f, 1.0f);
            glEnd();

            displayText(-0.2f, 0.4f, "Game Over!");
            displayText(-0.2f, 0.2f, "Final Score: " + to_string(currentScore));
            displayText(-0.2f, 0.0f, "High Score: " + to_string(highScore));
            displayText(-0.2f, -0.2f, "Press 'R' to Restart");
            displayText(-0.2f, -0.4f, "Press 'E' to Exit");
        }
    }
    else if (currentState == PLAYING) {
        
        drawRoad();
        drawScenery();
        drawPotholes();
        drawCar();
        drawObstacles();
        drawCoins();
        

        displayText(-0.9f, 0.9f, "Score: " + to_string(currentScore));
        displayText(0.5f, 0.9f, "High Score: " + to_string(highScore));

        string envText;
        switch (currentEnvironment) {
        case DESERT:
            envText = "Hard";
            break;
        case TROPICAL:
            envText = "Medium";
            break;
        default:
            envText = "Easy";
            break;
        }
        displayText(-0.2f, 0.9f, "Difficulty: " + envText);
    }
    glutSwapBuffers();
}

void handleInput(unsigned char key, int x, int y) {
    const float moveSpeed = 0.05f;

    switch (key) {
    case 'a': case 'A':
        carX -= moveSpeed * currentBoost;
        if (carX < -0.3f) carX = -0.3f;
        break;
    case 'd': case 'D':
        carX += moveSpeed * currentBoost;
        if (carX > 0.3f) carX = 0.3f;
        break;
    case 'w': case 'W':
        currentBoost = 1.5f;
        break;
    case 's': case 'S':
        currentBoost = 0.5f;
        break;
    case 'g': case 'G':
        if (currentState == MENU) {
            currentState = PLAYING;
            currentScore = 0;
            currentEnvironment = NORMAL;
            environmentSpawnMultiplier = 1.0f;
            spawnInterval = baseSpawnInterval * environmentSpawnMultiplier;
            leftScenery.clear();
            rightScenery.clear();
            obstacles.clear();
            coins.clear();
            potholes.clear();
        }
        break;
    case 'r': case 'R':
        if (currentState == GAME_OVER) {
            currentState = PLAYING;
            currentScore = 0;
            currentEnvironment = NORMAL;
            environmentSpawnMultiplier = 1.0f;
            spawnInterval = baseSpawnInterval * environmentSpawnMultiplier;
            carX = 0.0f;
            leftScenery.clear();
            rightScenery.clear();
            obstacles.clear();
            coins.clear();
            potholes.clear();
        }
        break;
    case 'e': case 'E':
        exit(0);
        break;
    }

    if (key != 'w' && key != 'W' && key != 's' && key != 'S') {
        currentBoost = 1.0f;
    }

    glutPostRedisplay();
}

void handleKeyUp(unsigned char key, int x, int y) {
    if (key == 'w' || key == 'W' || key == 's' || key == 'S') {
        currentBoost = 1.0f;
    }
}

void initOpenGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    srand(static_cast<unsigned>(time(0)));
    initStars();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Traffic Chaos");

    highScore = loadHighScore();
    initOpenGL();

    glutDisplayFunc(renderScene);
    glutKeyboardFunc(handleInput);
    glutKeyboardUpFunc(handleKeyUp);
    glutTimerFunc(16, updateGame, 0);
    
    glutMainLoop();

    return 0;
}