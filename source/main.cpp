#include <grrlib.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/machine/processor.h>
#include <ogc/pad.h>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

// images
#include "splash_png.h"
#include "death_png.h"
#include "paused_png.h"

constexpr int SCREEN_WIDTH  = 640;
constexpr int SCREEN_HEIGHT = 480;
constexpr int CELL_SIZE     = 16;
constexpr int GRID_WIDTH    = SCREEN_WIDTH / CELL_SIZE;
constexpr int GRID_HEIGHT   = SCREEN_HEIGHT / CELL_SIZE;
constexpr int MAX_SNAKE     = GRID_WIDTH * GRID_HEIGHT;
constexpr int SNAKE_UPDATE_INTERVAL = 6;
constexpr int COOLDOWN_FRAMES = 60;

int frameCounter = 0;
int pauseCooldown = 0;
int gameOverCooldown = 0;

struct Point { int x, y; };

Point snake[MAX_SNAKE];
int snakeLength;
int dirX, dirY;
Point apple;
bool gameOver;
bool IsGamePlaying;
bool isPaused;

void wait(u32 sec) {
    usleep(sec * 1000);
}

void initGame() {
    snakeLength = 5;
    for (int i = 0; i < snakeLength; ++i) {
        snake[i].x = GRID_WIDTH/2 - i;
        snake[i].y = GRID_HEIGHT/2;
    }
    dirX = 1; dirY = 0;
    gameOver = false;
    isPaused = 0;
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    apple.x = std::rand() % GRID_WIDTH;
    apple.y = std::rand() % GRID_HEIGHT;
    
    pauseCooldown = 0;
    gameOverCooldown = 0;
}

void respawnApple() {
    bool valid;
    do {
        valid = true;
        apple.x = std::rand() % GRID_WIDTH;
        apple.y = std::rand() % GRID_HEIGHT;
        for (int i = 0; i < snakeLength; ++i) {
            if (snake[i].x == apple.x && snake[i].y == apple.y) { valid = false; break; }
        }
    } while (!valid);
}

void handleInput(u32 pressed) {
    if (pauseCooldown > 0) pauseCooldown--;

    if ((pressed & PAD_BUTTON_START) && (pauseCooldown == 0)) {
        isPaused = !isPaused;
        pauseCooldown = COOLDOWN_FRAMES;
    }

    if (!isPaused) {
        if ((pressed & PAD_BUTTON_UP)    && dirY == 0) { dirX = 0; dirY = -1; }
        if ((pressed & PAD_BUTTON_DOWN)  && dirY == 0) { dirX = 0; dirY = 1; }
        if ((pressed & PAD_BUTTON_LEFT)  && dirX == 0) { dirX = -1; dirY = 0; }
        if ((pressed & PAD_BUTTON_RIGHT) && dirX == 0) { dirX = 1; dirY = 0; }
    }
}

void updateSnake() {
    for (int i = snakeLength - 1; i > 0; --i) snake[i] = snake[i-1];
    snake[0].x += dirX;
    snake[0].y += dirY;
    if (snake[0].x < 0) snake[0].x = GRID_WIDTH - 1;
    if (snake[0].x >= GRID_WIDTH) snake[0].x = 0;
    if (snake[0].y < 0) snake[0].y = GRID_HEIGHT - 1;
    if (snake[0].y >= GRID_HEIGHT) snake[0].y = 0;
    for (int i = 1; i < snakeLength; ++i) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            gameOver = true;
            gameOverCooldown = COOLDOWN_FRAMES;
            return;
        }
    }
    if (snake[0].x == apple.x && snake[0].y == apple.y) {
        if (snakeLength < MAX_SNAKE) ++snakeLength;
        respawnApple();
    }
}

void Paused() {
    GRRLIB_texImg* pausedTex = GRRLIB_LoadTexture(paused_png);
    GRRLIB_SetHandle(pausedTex, pausedTex->w / 2, pausedTex->h / 2);
    int localCooldown = pauseCooldown;
    
    while(true) {
        if (localCooldown > 0) localCooldown--;
        
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsHeld(0);
        if ((pressed & PAD_BUTTON_START) && (localCooldown == 0)) {
            isPaused = 0;
            pauseCooldown = COOLDOWN_FRAMES;
            GRRLIB_FreeTexture(pausedTex);
            break;
        }
        GRRLIB_DrawImg(0, 0, pausedTex, 0, 1, 1, 0xFFFFFFFF);
        GRRLIB_Render();
    }
}

void drawFrame() {

    GRRLIB_FillScreen(0x00000088);

    if (isPaused) {
        Paused();
    } else {
        GRRLIB_Rectangle(apple.x * CELL_SIZE, apple.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, 0xFF0000FF, true);
        for (int i = 0; i < snakeLength; ++i) {
            u32 color;
            if (i == 0) {
                color = 0x00FF00FF; // Head
            } else {
                color = 0x00AA00FF; // Body
            }
            GRRLIB_Rectangle(snake[i].x * CELL_SIZE, snake[i].y * CELL_SIZE, CELL_SIZE, CELL_SIZE, color, true);
        }
    }
}

void Splash() {
    GRRLIB_texImg* splashTex = GRRLIB_LoadTexture(splash_png);
    GRRLIB_SetHandle(splashTex, splashTex->w / 2, splashTex->h / 2);
    while(!IsGamePlaying) {
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsHeld(0);
        if (pressed & PAD_BUTTON_A) {
            IsGamePlaying = 1;
            GRRLIB_FreeTexture(splashTex);
            break;
        }
        GRRLIB_DrawImg(0, 0, splashTex, 0, 1, 1, 0xFFFFFFFF);
        GRRLIB_Render();
    }
}

void GameOver() {
    GRRLIB_texImg* deathTex = GRRLIB_LoadTexture(death_png);
    GRRLIB_SetHandle(deathTex, deathTex->w / 2, deathTex->h / 2);
    
    int localCooldown = gameOverCooldown;
    
    while(true) {
        if (localCooldown > 0) localCooldown--;
        
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsHeld(0);
        
        if ((localCooldown == 0) && (pressed & PAD_BUTTON_A)) {
            GRRLIB_FreeTexture(deathTex);
            IsGamePlaying = 1;
            initGame();
            break;
        }
        
        GRRLIB_DrawImg(0, 0, deathTex, 0, 1, 1, 0xFFFFFFFF);
        GRRLIB_Render();
    }
}

int main(int argc, char **argv) {
    VIDEO_Init();
    PAD_Init();
    GRRLIB_Init();

    GRRLIB_SetBackgroundColour(0, 0, 0, 0);
    
    IsGamePlaying = 0;

    Splash();

    initGame();

    while (true) {
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsHeld(0);

        if ((pressed & PAD_TRIGGER_Z) && (pressed & PAD_TRIGGER_R) && (pressed & PAD_BUTTON_B) && (pressed & PAD_BUTTON_DOWN)) {
            break;
        }
        if (IsGamePlaying == 1)  {
            if (!gameOver && !isPaused) {
                handleInput(pressed);
                frameCounter++;
                if (frameCounter >= SNAKE_UPDATE_INTERVAL) {
                    updateSnake();
                    frameCounter = 0;
                }
            } else if (gameOver) {
                IsGamePlaying = 0;
                GameOver();
            }
        } 
        drawFrame();
        GRRLIB_Render();
    }
    GRRLIB_Exit();
    return 0;
}