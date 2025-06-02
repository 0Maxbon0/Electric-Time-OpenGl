#include <GL/glut.h>
#include <iostream>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif
//Made by :-
//Abdallah Salah Elsaid
//Maxim Mamdouh Salib
using namespace std;

string getHighScoreFilePath() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        string dir = string(path) + "//game";
        CreateDirectory(dir.c_str(), NULL);
        return dir + "\\highscore.txt";
    }
#endif
    return "highscore.txt";
}

enum Character { BMO, REPO };
Character selectedCharacter = BMO;

// Background layer structure for parallax scrolling effect
struct BackgroundLayer {
    float offset;
    float speed;
    int color;
};

// Star structure for night sky
struct Star {
    float x, y;
    float size;
    float brightness;
    float twinkleSpeed;
    float twinklePhase;
};

// Rain drop structure
struct Rain {
    float x, y;
    float speed;
    float length;
};

// Rocket obstacle structure
struct Rocket {
    float x, y;
    float speed;
    float width, height;
};

// Power-up structure for health restoration
struct PowerUp {
    float x, y;
    float speed;
    float size;
    bool active;
};

// Game object collections
vector<Rain> Rains;
vector<Rocket> Rockets;
vector<PowerUp> powerUps;
vector<BackgroundLayer> backgroundLayers;
vector<Star> stars;

// Sky color themes (night, day, sunset)
const float skyColors[3][3] = {
    {0.0f, 0.0f, 0.3f},
    {0.4f, 0.7f, 1.0f},
    {1.0f, 0.6f, 0.4f}
};
int currentSkyTheme = 0;

// Player health tracking
int hitCount = 0;
int maxHits = 3;

// Global scale factor for consistent sizing
const float scale = 0.25f;

// BMO character dimensions
const float bmoWidth = 0.8f * scale;
const float bmoHeight = 1.2f * scale;
const float bmoHalfWidth = bmoWidth / 2.0f;
const float bmoHalfHeight = bmoHeight / 2.0f;

// REPO character dimensions
const float REPOWidth = 0.8f * scale;
const float REPOHeight = 1.2f * scale;
const float REPOHalfWidth = REPOWidth / 2.0f;
const float REPOHalfHeight = REPOHeight / 2.0f;

// Character limb dimensions
const float armWidth = 0.1f * scale;
const float armHeight = 0.05f * scale;
const float legWidth = 0.05f * scale;
const float legHeight = 0.15f * scale;

// Screen boundaries in normalized coordinates
const float screenLeft = -2.0f;
const float screenRight = 2.0f;
const float screenBottom = -1.0f;
const float screenTop = 1.0f;

// Character position and movement properties
float charX = 0.0f;
float charY = 0.0f;
float moveSpeed = 0.1f;

int gameTime = 0;
int lastTime = 0;

// Game state management
enum GameState { START, SELECT, PLAYING, GAME_OVER, PAUSED };
GameState gameState = START;

const float gravity = 0.002f;
float verticalVelocity = 0.0f;
bool isOnGround = false;
const float groundHeight = 0.3f;

int score = 0;
int highScore = 0;

bool showMsg = false;
int msgStart = 0;
const int msgDur = 2000;
bool newRecMsg = false;
int newRecStart = 0;
const int newRecDur = 2000;
bool recBroken = false;

// Character animation variables
float legAngle = 0.0f;
float legSwingSpeed = 0.2f;
bool isMoving = false;

bool isInvincible = false;
int invincibleStartTime = 0;
const int invincibleDuration = 2000;
int flashTimer = 0;
const int flashInterval = 100;

void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c; ++c) {
        glutBitmapCharacter(font, *c);
    }
}

void drawCircle(float cx, float cy, float r, int segments = 100) {
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.1415926f * i / segments;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

void drawHalfCircle(float cx, float cy, float r, int segments = 100) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float theta = 3.1415926f + 3.1415926f * float(i) / float(segments);
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

void drawRectangle(float x, float y, float width, float height) {
    glBegin(GL_POLYGON);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawTriangle(float x, float y, float size, bool upward = true) {
    glBegin(GL_TRIANGLES);
    if (upward) {
        glVertex2f(x, y);
        glVertex2f(x + size, y);
        glVertex2f(x + size / 2, y + size);
    } else {
        glVertex2f(x, y);
        glVertex2f(x + size, y);
        glVertex2f(x + size / 2, y - size);
    }
    glEnd();
}

// Draw the battery-shaped health indicator
void drawBatteryHealthBar() {
    float startX = screenLeft + 0.1f;
    float startY = screenTop - 0.25f;
    float width = 0.6f;
    float height = 0.2f;
    float segmentWidth = width / maxHits;

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(startX, startY);
    glVertex2f(startX + width, startY);
    glVertex2f(startX + width, startY + height);
    glVertex2f(startX, startY + height);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(startX + width, startY + height/4);
    glVertex2f(startX + width + 0.05f, startY + height/4);
    glVertex2f(startX + width + 0.05f, startY + 3*height/4);
    glVertex2f(startX + width, startY + 3*height/4);
    glEnd();

    for (int i = 0; i < maxHits - hitCount; i++) {
        if (maxHits - hitCount > (maxHits/3)*2) {
            glColor3f(0.0f, 1.0f, 0.0f);
        } else if (maxHits - hitCount > (maxHits/3)){
            glColor3f(1.0f, 1.0f, 0.0f);
        } else {
            glColor3f(1.0f, 0.0f, 0.0f);
        }

        glBegin(GL_POLYGON);
        glVertex2f(startX + i*segmentWidth, startY);
        glVertex2f(startX + (i+1)*segmentWidth, startY);
        glVertex2f(startX + (i+1)*segmentWidth, startY + height);
        glVertex2f(startX + i*segmentWidth, startY + height);
        glEnd();
    }
}

// Draw a plus sign (+)
void drawPlus(float x, float y, float size) {
    float thickness = size / 3.0f;
    float half = size / 2.0f;

    drawRectangle(x + size/2 - thickness/2, y, thickness, size);
    drawRectangle(x, y + size/2 - thickness/2, size, thickness);
}

// Draw the BMO character
void drawBMO() {
    if (!isInvincible || (int)(glutGet(GLUT_ELAPSED_TIME) / flashInterval % 2 == 0)) {
    glColor3f(0.2f, 0.8f, 0.6f);
    drawRectangle(charX - bmoHalfWidth, charY - bmoHalfHeight, bmoWidth, bmoHeight);

    glPushMatrix();
    glTranslatef(charX - 0.1f * scale, charY - bmoHalfHeight, 0.0f);
    glRotatef(sinf(legAngle) * 15.0f, 0, 0, 1);
    glBegin(GL_POLYGON);
        glVertex2f(-legWidth/2, 0);
        glVertex2f(legWidth/2, 0);
        glVertex2f(legWidth/2, -legHeight);
        glVertex2f(-legWidth/2, -legHeight);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(charX + 0.1f * scale, charY - bmoHalfHeight, 0.0f);
    glRotatef(-sinf(legAngle) * 15.0f, 0, 0, 1);
    glBegin(GL_POLYGON);
        glVertex2f(-legWidth/2, 0);
        glVertex2f(legWidth/2, 0);
        glVertex2f(legWidth/2, -legHeight);
        glVertex2f(-legWidth/2, -legHeight);
    glEnd();
    glPopMatrix();

    glBegin(GL_POLYGON);
        glVertex2f(charX - bmoHalfWidth, charY - bmoHalfHeight + 0.5f * scale);
        glVertex2f(charX - bmoHalfWidth, charY - bmoHalfHeight + 0.4f * scale);
        glVertex2f(charX - bmoHalfWidth - 0.2f * scale, charY - bmoHalfHeight + 0.25f * scale);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex2f(charX + bmoHalfWidth, charY - bmoHalfHeight + 0.5f * scale);
        glVertex2f(charX + bmoHalfWidth, charY - bmoHalfHeight + 0.4f * scale);
        glVertex2f(charX + bmoHalfWidth + 0.2f * scale, charY - bmoHalfHeight + 0.25f * scale);
    glEnd();

    glColor3f(0.6f, 1.0f, 0.8f);
    drawRectangle(charX - 0.25f * scale, charY + 0.2f * scale, 0.5f * scale, 0.3f * scale);

    glColor3f(0.0f, 0.0f, 0.0f);
    drawCircle(charX - 0.15f * scale, charY + 0.35f * scale, 0.025f * scale);
    drawCircle(charX + 0.15f * scale, charY + 0.35f * scale, 0.025f * scale);

    glColor3f(1.0f, 0.0f, 0.6f);
    drawHalfCircle(charX, charY + 0.28f * scale, 0.05f * scale);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawRectangle(charX - 0.020f * scale, charY + 0.28f * scale, 0.040f * scale, -0.015f * scale);

    glColor3f(1.0f, 0.0f, 0.5f);
    drawCircle(charX + 0.2f * scale, charY - 0.2f * scale, 0.04f * scale);

    glColor3f(0.0f, 0.6f, 1.0f);
    drawTriangle(charX + 0.08f * scale, charY - 0.12f * scale, 0.08f * scale);

    glColor3f(0.0f, 1.0f, 0.4f);
    drawCircle(charX + 0.22f * scale, charY - 0.05f * scale, 0.035f * scale);

    glColor3f(1.0f, 1.0f, 0.0f);
    drawPlus(charX - 0.15f * scale, charY - 0.15f * scale, 0.15f * scale);

    glColor3f(0.4f, 0.4f, 0.4f);
    drawRectangle(charX - 0.23f * scale, charY - 0.28f * scale, 0.08f * scale, 0.03f * scale);
    drawRectangle(charX - 0.14f * scale, charY - 0.28f * scale, 0.08f * scale, 0.03f * scale);
    }
}
//Draw REPO character//Draw REPO character
void drawREPO() {
    if (!isInvincible || (int)(glutGet(GLUT_ELAPSED_TIME) / flashInterval % 2 == 0)) {
    glColor3f(0.6f, 0.85f, 0.2f);

    glBegin(GL_POLYGON);
        glVertex2f(charX - REPOHalfWidth, charY - REPOHalfHeight);
        glVertex2f(charX + REPOHalfWidth, charY - REPOHalfHeight);
        glVertex2f(charX + REPOHalfWidth, charY - REPOHalfHeight + 0.8f * scale);
        glVertex2f(charX - REPOHalfWidth, charY - REPOHalfHeight + 0.8f * scale);
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(charX, charY - REPOHalfHeight + 0.8f * scale);
    for (int i = 0; i <= 100; i++) {
        float theta = 3.1415926f * float(i) / float(100);
        float x = REPOHalfWidth * cosf(theta);
        float y = REPOHalfWidth * sinf(theta);
        glVertex2f(charX + x, charY - REPOHalfHeight + 0.8f * scale + y);
    }
    glEnd();

    glBegin(GL_POLYGON);
        glVertex2f(charX - REPOHalfWidth, charY - REPOHalfHeight + 0.5f * scale);
        glVertex2f(charX - REPOHalfWidth, charY - REPOHalfHeight + 0.4f * scale);
        glVertex2f(charX - REPOHalfWidth - 0.2f * scale, charY - REPOHalfHeight + 0.25f * scale);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex2f(charX + REPOHalfWidth, charY - REPOHalfHeight + 0.5f * scale);
        glVertex2f(charX + REPOHalfWidth, charY - REPOHalfHeight + 0.4f * scale);
        glVertex2f(charX + REPOHalfWidth + 0.2f * scale, charY - REPOHalfHeight + 0.25f * scale);
    glEnd();

    glPushMatrix();
        glTranslatef(charX - 0.1f * scale, charY - REPOHalfHeight, 0.0f);
        glRotatef(sinf(legAngle) * 15.0f, 0, 0, 1);
        glBegin(GL_POLYGON);
        glVertex2f(-0.05f * scale, 0);
        glVertex2f(0.05f * scale, 0);
        glVertex2f(0, -0.3f * scale);
    glEnd();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(charX + 0.1f * scale, charY - REPOHalfHeight, 0.0f);
        glRotatef(-sinf(legAngle) * 15.0f, 0, 0, 1);
        glBegin(GL_POLYGON);
        glVertex2f(-0.05f * scale, 0);
        glVertex2f(0.05f * scale, 0);
        glVertex2f(0, -0.3f * scale);
    glEnd();
    glPopMatrix();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(charX - 0.07f * scale, charY - REPOHalfHeight + 0.8f * scale, 0.04f * scale, 30);
    drawCircle(charX + 0.07f * scale, charY - REPOHalfHeight + 0.8f * scale, 0.04f * scale, 30);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawCircle(charX - 0.07f * scale, charY - REPOHalfHeight + 0.8f * scale, 0.02f * scale, 30);
    drawCircle(charX + 0.07f * scale, charY - REPOHalfHeight + 0.8f * scale, 0.02f * scale, 30);
    }
}

void drawBackground() {
    glBegin(GL_QUADS);
    glColor3f(skyColors[currentSkyTheme][0], skyColors[currentSkyTheme][1], skyColors[currentSkyTheme][2]);
    glVertex2f(screenLeft, screenBottom + 0.3f);
    glVertex2f(screenRight, screenBottom + 0.3f);
    glVertex2f(screenRight, screenTop);
    glVertex2f(screenLeft, screenTop);
    glEnd();

    if (currentSkyTheme == 0) {
        for (const auto& star : stars) {
            float brightness = star.brightness * (0.7f + 0.3f * sinf(star.twinklePhase));
            glColor3f(brightness, brightness, brightness);
            drawCircle(star.x, star.y, star.size);
        }
    }

    if (currentSkyTheme == 0) {
        glColor3f(0.9f, 0.9f, 0.8f);
        drawCircle(screenRight - 0.3f, screenTop - 0.2f, 0.1f);
        glColor3f(0.0f, 0.0f, 0.3f);
        drawCircle(screenRight - 0.33f, screenTop - 0.22f, 0.03f);
    } else if (currentSkyTheme == 1) {
        glColor3f(1.0f, 1.0f, 0.0f);
        drawCircle(screenRight - 0.33f, screenTop - 0.2f, 0.1f);
    } else {
        glColor3f(1.0f, 0.4f, 0.0f);
        drawCircle(screenRight - 0.5f, screenBottom + 0.5f, 0.15f);
    }

    for (int i = 0; i < backgroundLayers.size(); i++) {
        const auto& layer = backgroundLayers[i];

        if (i == 0) {
            if (currentSkyTheme == 0)
                glColor3f(0.2f, 0.2f, 0.3f);
            else if (currentSkyTheme == 1)
                glColor3f(0.3f, 0.4f, 0.5f);
            else
                glColor3f(0.5f, 0.3f, 0.4f);
        } else if (i == 1) {
            if (currentSkyTheme == 0)
                glColor3f(0.1f, 0.1f, 0.2f);
            else if (currentSkyTheme == 1)
                glColor3f(0.2f, 0.5f, 0.3f);
            else
                glColor3f(0.4f, 0.2f, 0.3f);
        } else {
            if (currentSkyTheme == 0)
                glColor3f(0.0f, 0.1f, 0.0f);
            else if (currentSkyTheme == 1)
                glColor3f(0.0f, 0.4f, 0.0f);
            else
                glColor3f(0.2f, 0.1f, 0.0f);
        }

        float height = 0.3f + i * 0.1f;
        float baseY = screenBottom + 0.3f;

        for (float x = -2.0f - layer.offset; x < 2.0f; x += 0.2f) {
            float peakHeight = height * (0.5f + 0.5f * sinf(x * 3.0f + i * 2.0f));

            glBegin(GL_TRIANGLES);
            glVertex2f(x, baseY);
            glVertex2f(x + 0.2f, baseY);
            glVertex2f(x + 0.1f, baseY + peakHeight);
            glEnd();
        }
    }
}

void initBackground() {
    for (int i = 0; i < 3; i++) {
        BackgroundLayer layer;
        layer.offset = 0.0f;
        layer.speed = 0.01f * (i + 1);
        layer.color = i;
        backgroundLayers.push_back(layer);
    }

    for (int i = 0; i < 50; i++) {
        Star star;
        star.x = screenLeft + static_cast<float>(rand()) / RAND_MAX * (screenRight - screenLeft);
        star.y = screenBottom + 0.3f + static_cast<float>(rand()) / RAND_MAX * (screenTop - screenBottom - 0.3f);
        star.size = 0.002f + static_cast<float>(rand()) / RAND_MAX * 0.006f;
        star.brightness = 0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f;
        star.twinkleSpeed = 0.001f + static_cast<float>(rand()) / RAND_MAX * 0.005f;
        star.twinklePhase = static_cast<float>(rand()) / RAND_MAX * 6.28f;
        stars.push_back(star);
    }
}

void updateBackground() {
    for (auto& layer : backgroundLayers) {
        layer.offset += layer.speed;
        if (layer.offset > 4.0f) {
            layer.offset -= 4.0f;
        }
    }

    for (auto& star : stars) {
        star.twinklePhase += star.twinkleSpeed;
        if (star.twinklePhase > 6.28f) {
            star.twinklePhase -= 6.28f;
        }
    }

    if (score % 9 < 3) {
        currentSkyTheme = 0;
    } else if (score % 9 < 6) {
        currentSkyTheme = 1;
    } else {
        currentSkyTheme = 2;
    }
}

void spawnRain() {
    Rain dot;
    dot.x = screenLeft + static_cast<float>(rand()) / RAND_MAX * (screenRight - screenLeft);
    dot.y = screenTop + 0.1f;
    dot.speed = 0.01f + static_cast<float>(rand()) / RAND_MAX * 0.02f;
    dot.length = 0.1f + static_cast<float>(rand()) / RAND_MAX * 0.1f;
    Rains.push_back(dot);
}

void updateRains() {
    for (auto& dot : Rains) {
        dot.y -= dot.speed;
    }

    for (auto it = Rains.begin(); it != Rains.end();) {
        bool hit = false;
        if (selectedCharacter == BMO) {
            hit = abs(it->x - charX - armWidth) < bmoHalfWidth && abs(it->y - charY) < bmoHalfHeight + 0.18;
        } else {
            hit = abs(it->x - charX) < REPOHalfWidth && abs(it->y - charY) < REPOHalfHeight + 0.18;
        }

        if (it->y < groundHeight - 0.82f || hit) {
           if (!isInvincible && hit) {
                hitCount++;
           if (hitCount >= maxHits) {
                gameState = GAME_OVER;
           }
            isInvincible = true;
            invincibleStartTime = glutGet(GLUT_ELAPSED_TIME);
}
            it = Rains.erase(it);
        } else {
            ++it;
        }
    }
}

void spawnRocket() {
    Rocket projectile;
    projectile.x = screenLeft - 0.1f;
    projectile.y = screenBottom + groundHeight + 0.1f;
    projectile.speed = 0.03f + static_cast<float>(rand()) / RAND_MAX * 0.02f;
    projectile.width = 0.2f;
    projectile.height = 0.1f;
    Rockets.push_back(projectile);
}

void updateRockets() {
    for (auto & projectile : Rockets) {
       projectile.x += projectile.speed;
    }

    for (auto it = Rockets.begin(); it != Rockets.end();) {
        bool hit = false;
        if (selectedCharacter == BMO) {
            hit = (charX + bmoHalfWidth > it->x - it->width/2) && (charX - bmoHalfWidth < it->x + it->width/2) && (charY - bmoHalfHeight < it->y + it->height) && (charY + bmoHalfHeight > it->y);
        } else {
            hit = (charX + REPOHalfWidth > it->x - it->width/2) &&(charX - REPOHalfWidth < it->x + it->width/2) &&(charY - REPOHalfHeight < it->y + it->height) &&(charY + REPOHalfHeight > it->y);
        }

        bool offScreen = it->x - it->width/2 > screenRight;

        if (hit || offScreen) {
           if (!isInvincible && hit) {
            hitCount++;
            if (hitCount >= maxHits) {
            gameState = GAME_OVER;
            }
            isInvincible = true;
            invincibleStartTime = glutGet(GLUT_ELAPSED_TIME);
            }
            it = Rockets.erase(it);
        } else {
            ++it;
        }
    }
}

void spawnPowerUp() {
    if (rand() % 500 < 1) {
        PowerUp pu;
        pu.x = screenLeft + static_cast<float>(rand()) / RAND_MAX * (screenRight - screenLeft);
        pu.y = screenTop + 0.1f;
        pu.speed = 0.02f ;
        pu.size = 0.4f * scale;
        pu.active = true;
        powerUps.push_back(pu);
    }
}

void updatePowerUps() {
    for (auto& pu : powerUps) {
        if (pu.active) {
            pu.y -= pu.speed;

            bool collected = false;
            if (selectedCharacter == BMO) {
                collected = (charX + bmoHalfWidth > pu.x - pu.size/2) && (charX - bmoHalfWidth < pu.x + pu.size/2) && (charY + bmoHalfHeight > pu.y - pu.size/2) && (charY - bmoHalfHeight < pu.y + pu.size/2);
            } else {
                collected = (charX + REPOHalfWidth > pu.x - pu.size/2) && (charX - REPOHalfWidth < pu.x + pu.size/2) && (charY + REPOHalfHeight > pu.y - pu.size/2) && (charY - REPOHalfHeight < pu.y + pu.size/2);
            }

            if (collected) {
                pu.active = false;
                if (hitCount > 0) {
                    hitCount--;
                }
            }
            if (pu.y < groundHeight - 0.95f) {
                pu.active = false;
            }
        }
    }

    powerUps.erase(remove_if(powerUps.begin(), powerUps.end(),
                            [](const PowerUp& pu) { return !pu.active; }),
                  powerUps.end());
}

void updateInvincibility() {
    if (isInvincible) {
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - invincibleStartTime >= invincibleDuration) {
            isInvincible = false;
        }
    }
}

void GamePage(int) {
    if (gameState == PLAYING) {

        if (isMoving) {
            legAngle += legSwingSpeed;
        }

        //gravity
        if (!isOnGround) {
            verticalVelocity -= gravity;
            charY += verticalVelocity;
        }

        //ground collision
        float charBottom = 0;
        if (selectedCharacter == BMO) {
            charBottom = charY - bmoHalfHeight - 0.3f * scale;
        } else {
            charBottom = charY - REPOHalfHeight - 0.3f * scale;
        }

        if (charBottom <= screenBottom + groundHeight) {
            if (selectedCharacter == BMO) {
                charY = screenBottom + groundHeight + bmoHalfHeight + legHeight;
            } else {
                charY = screenBottom + groundHeight + REPOHalfHeight + 0.3f * scale;
            }
            verticalVelocity = 0.0f;
            isOnGround = true;
        } else {
            isOnGround = false;
        }

        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - lastTime >= 1000) {
            gameTime++;
            lastTime = currentTime;
        }

        updateBackground();

        updateInvincibility();

        //Rain spawn rate
        if (rand() % 10 < 1) {
            spawnRain();
        }

        //lePosa spawn rate
        if (rand() % 500 < 1) {
            spawnRocket();
        }

        updateRockets();

        spawnPowerUp();
        updatePowerUps();
    }
    glutPostRedisplay();
    glutTimerFunc(16, GamePage, 0);
}

void loadHighScore() {
    string filePath = getHighScoreFilePath();
    ifstream file(filePath.c_str());

    if (file.is_open()) {
        if (!(file >> highScore)) {
            highScore = 0;
        }
        file.close();
    } else {
        highScore = 0;
    }
}

void saveHighScore() {
    string filePath = getHighScoreFilePath();
    ofstream file(filePath.c_str());

    if (file.is_open()) {
        file << highScore;
        file.close();
    } else {
        cerr << "Error: Could not save high score to " << filePath << endl;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameState == START) {
        glColor3f(1.0f, 1.0f, 1.0f);
        char heartsText[32];
        sprintf(heartsText, "YOU HAVE %d HITS", maxHits);
        drawText(-0.47f, 0.3f, heartsText);
        drawText(-0.62f, 0.2f,"AVOID RAIN AND ROCKETS!!");
        drawText(-0.48f, 0.1f,"Press ENTER to Start");
        drawText(-0.82f, -0.8f,"Go Right To Up Your Score AND DONT STOP.");

    } else if (gameState == SELECT) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(-0.4f, 0.8f, "SELECT YOUR CHARACTER");
        drawText(-0.7f, 0.5f, "Press 1 for BMO");
        drawText(0.3f, 0.5f, "Press 2 for REPO");

        glPushMatrix();
        glTranslatef(1.72f, 0.08f, 0.0f);
        glScalef(1.5f, 1.5f, 1.0f);
        drawBMO();
        glPopMatrix();

        glPushMatrix();
        glTranslatef(2.72f, 0.1f, 0.0f);
        glScalef(1.5f, 1.5f, 1.0f);
        drawREPO();
        glPopMatrix();

    } else if (gameState == GAME_OVER) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(-0.45f, 0.1f, "Game Over!");
        drawText(-0.5f, -0.1f, "Press R to Restart");
    } else if (gameState == PLAYING){
        // Draw background first
        drawBackground();

        //ground
        glColor3f(0.0f, 0.5f, 0.2f);
        drawRectangle(screenLeft, screenBottom, screenRight - screenLeft, 0.3f);

        // Draw character
        if (selectedCharacter == BMO) {
            drawBMO();
        } else {
            drawREPO();
        }

        //Rain
        glColor3f(0.0f, 0.0f, 2.0f);
        for (const auto& dot : Rains) {
        glBegin(GL_LINES);
            glVertex2f(dot.x, dot.y);
            glVertex2f(dot.x, dot.y - dot.length);
        glEnd();

        glBegin(GL_TRIANGLES);
            glVertex2f(dot.x - 0.0001f, dot.y - dot.length);
            glVertex2f(dot.x + 0.0001f, dot.y - dot.length);
            glVertex2f(dot.x, dot.y - dot.length - 0.02f);
        glEnd();
        }

        //lePosa
        glColor3f(1.0f, 0.0f, 0.0f);
        for (const auto& projectile : Rockets) {
        glBegin(GL_POLYGON);
            glVertex2f(projectile.x - projectile.width/2, projectile.y);
            glVertex2f(projectile.x - projectile.width/2, projectile.y + projectile.height);
            glVertex2f(projectile.x + projectile.width/2 - 0.05f, projectile.y + projectile.height);
            glVertex2f(projectile.x + projectile.width/2, projectile.y + projectile.height/2);
            glVertex2f(projectile.x + projectile.width/2 - 0.05f, projectile.y);
        glEnd();
        }

        //score stuff
        glColor3f(1.0f, 1.0f, 1.0f);
        char scoreText[32];
        sprintf(scoreText, "Score: %d", score);
        drawText(screenRight - 0.7f, screenTop - 0.2f, scoreText);

        sprintf(scoreText, "High: %d", highScore);
        drawText(screenRight - 0.7f, screenTop - 0.3f, scoreText);
        updateRains();
        drawBatteryHealthBar();

        //timer
        glColor3f(1.0f, 1.0f, 1.0f);
        char buffer[32];
        sprintf(buffer, "Time: %d", gameTime);
        drawText(screenRight - 0.7f, screenTop - 0.1f, buffer);


        if (newRecMsg) {
            int currentTime = glutGet(GLUT_ELAPSED_TIME);
            if (currentTime - newRecStart < newRecDur) {
                glColor3f(0.0f, 1.0f, 0.0f);
                drawText(-0.3f, screenTop - 0.4f, "NEW RECORD!");
            } else {
                newRecMsg = false;
            }
        }

        if (showMsg) {
            int currentTime = glutGet(GLUT_ELAPSED_TIME);
            if (currentTime - msgStart < msgDur) {
                glColor3f(1.0f, 0.0f, 0.0f);
                drawText(-0.35f, screenTop - 0.3f, "NO GOING BACK!");
            } else {
                showMsg = false;
            }
        }

        //powerups
        glColor3f(0.0f, 1.0f, 0.0f);
            for (const auto& pu : powerUps) {
                if (pu.active) {
                    drawPlus(pu.x - pu.size/2, pu.y - pu.size/2, pu.size);
                }
            }

    } else {
        drawBackground();

        // Draw pause overlay
        glColor4f(0.0f, 0.0f, 0.0f, 0.9f);
        glBegin(GL_QUADS);
            glVertex2f(screenLeft, screenBottom);
            glVertex2f(screenRight, screenBottom);
            glVertex2f(screenRight, screenTop);
            glVertex2f(screenLeft, screenTop);
        glEnd();


        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(-0.4f, 0.2f, "GAME PAUSED");
        drawText(-0.5f, 0.0f, "Press ESC to continue");

        glutSwapBuffers();
        return;
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int, int) {
    switch(key) {
        case 27:  //Escape key
            if (gameState == PLAYING) {
                gameState = PAUSED;
            } else if (gameState == PAUSED) {
                gameState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME);
            }
            break;

        case '1':
            if (gameState == SELECT) {
                selectedCharacter = BMO;
                gameState = PLAYING;
            }
            break;

        case '2':
            if (gameState == SELECT) {
                selectedCharacter = REPO;
                gameState = PLAYING;
            }
            break;

        case 13:  // Enter key
            if (gameState == START) {
                charX = screenLeft + (selectedCharacter == BMO ? bmoHalfWidth : REPOHalfWidth) + armWidth + 0.4;
                gameState = SELECT;
                gameTime = 0;
                score = 0;
                recBroken = false;
                lastTime = glutGet(GLUT_ELAPSED_TIME);
            }
            break;

        case 'r':
        case 'R':
            if (gameState == GAME_OVER) {
                charX = screenLeft + (selectedCharacter == BMO ? bmoHalfWidth : REPOHalfWidth) + armWidth + 0.4;
                charY = 0.0f;
                hitCount = 0;
                isInvincible = false;
                Rains.clear();
                Rockets.clear();
                powerUps.clear();
                gameState = SELECT;
                gameTime = 0;
                score = 0;
                recBroken = false;
                lastTime = glutGet(GLUT_ELAPSED_TIME);
            }
            break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int, int) {
    if (gameState == PLAYING) {
        bool wasMoving = isMoving;
        isMoving = false;

        switch (key) {
            case GLUT_KEY_UP:
                if (isOnGround) {
                    verticalVelocity = 0.04f;
                    isOnGround = false;
                }
                break;
            case GLUT_KEY_DOWN:
                break;
            case GLUT_KEY_LEFT:
                charX -= moveSpeed;
                isMoving = true;
                if (charX - (selectedCharacter == BMO ? bmoHalfWidth : REPOHalfWidth) - armWidth < screenLeft) {
                    charX = screenLeft + (selectedCharacter == BMO ? bmoHalfWidth : REPOHalfWidth) + armWidth;
                    showMsg = true;
                    msgStart = glutGet(GLUT_ELAPSED_TIME);
                }
                break;
            case GLUT_KEY_RIGHT:
                charX += moveSpeed;
                isMoving = true;
                break;
        }

        if (!isMoving && wasMoving) {
            legAngle = 0.0f;
        }

        //round right to left
        if (charX > screenRight + (selectedCharacter == BMO ? bmoHalfWidth : REPOHalfWidth)) {
            charX = screenLeft - (selectedCharacter == BMO ? bmoHalfWidth : REPOHalfWidth);
            score++;
            Rains.clear();
            Rockets.clear();
            powerUps.clear();
                if (score > highScore) {
                        highScore = score;
                        saveHighScore();
                            if (!recBroken) {
                                newRecMsg = true;
                                newRecStart = glutGet(GLUT_ELAPSED_TIME);
                                recBroken = true;
                            }
                }
        }

        //Left boundary
        if (charX - (selectedCharacter == BMO ? bmoHalfWidth : REPOHalfWidth) - armWidth < screenLeft) {
            charX = screenLeft + (selectedCharacter == BMO ? bmoHalfWidth : REPOHalfWidth) + armWidth;
        }

        //Ground
        if (selectedCharacter == BMO) {
            if (charY - bmoHalfHeight - legHeight < screenBottom + groundHeight) {
                charY = screenBottom + groundHeight + bmoHalfHeight + legHeight;
            }
        } else {
            if (charY - REPOHalfHeight - 0.3f * scale < screenBottom + groundHeight) {
                charY = screenBottom + groundHeight + REPOHalfHeight + 0.3f * scale;
            }
        }
    }
    glutPostRedisplay();
}

void initia() {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(screenLeft, screenRight, screenBottom, screenTop);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1200, 600);
    glutCreateWindow("Electric Time ");
    initia();
    initBackground();
    loadHighScore();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, GamePage, 0);
    glutMainLoop();

    return 0;
}

