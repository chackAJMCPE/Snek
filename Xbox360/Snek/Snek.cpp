#include "stdafx.h"
#include "../xhblib/xhblib.h"
#include <cstdlib>
#include <ctime>
#include "death.h"
#include "paused.h"
#include "splash.h"

// Screen dimensions
#define CELL_SIZE     16
#define GRID_WIDTH    (SCREEN_WIDTH / CELL_SIZE)
#define GRID_HEIGHT   (SCREEN_HEIGHT / CELL_SIZE)
#define MAX_SNAKE (GRID_WIDTH * GRID_HEIGHT)
#define SNAKE_UPDATE_INTERVAL 6
#define COOLDOWN_FRAMES 60

// Globals
int frameCounter = 0;
int pauseCooldown = 0;
int gameOverCooldown = 0;
bool g_quit = false;

struct Point { int x, y; };
Point snake[MAX_SNAKE];
int snakeLength;
int dirX, dirY;
Point apple;
bool gameOver;
bool IsGamePlaying;
bool isPaused;



uint32_t XPAD_GetButtons(int player) {
    uint32_t buttons = 0;
        // Gamepad input
    if (XPAD_IsButtonPressed(player, BUTTON_UP))    buttons |= BUTTON_UP;
    if (XPAD_IsButtonPressed(player, BUTTON_DOWN))  buttons |= BUTTON_DOWN;
    if (XPAD_IsButtonPressed(player, BUTTON_LEFT))  buttons |= BUTTON_LEFT;
    if (XPAD_IsButtonPressed(player, BUTTON_RIGHT)) buttons |= BUTTON_RIGHT;
    if (XPAD_IsButtonPressed(player, BUTTON_START)) buttons |= BUTTON_START;
    if (XPAD_IsButtonPressed(player, BUTTON_BACK))  buttons |= BUTTON_BACK;
    if (XPAD_IsButtonPressed(player, BUTTON_A))     buttons |= BUTTON_A;
    if (XPAD_IsButtonPressed(player, BUTTON_B))     buttons |= BUTTON_B;

    return buttons;
}


void initGame() {
    snakeLength = 5;
    for (int i = 0; i < snakeLength; ++i) {
        snake[i].x = GRID_WIDTH/2 - i;
        snake[i].y = GRID_HEIGHT/2;
    }
    dirX = 1; dirY = 0;
    gameOver = false;
    isPaused = false;
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
            if (snake[i].x == apple.x && snake[i].y == apple.y) { 
                valid = false; 
                break; 
            }
        }
    } while (!valid);
}

void handleInput(uint32_t pressed) {
    if (pauseCooldown > 0) pauseCooldown--;

    if ((pressed & BUTTON_START) && (pauseCooldown == 0)) {
        isPaused = !isPaused;
        pauseCooldown = COOLDOWN_FRAMES;
    }

    if (!isPaused) {
        if ((pressed & BUTTON_UP)    && dirY == 0) { dirX = 0; dirY = -1; }
        if ((pressed & BUTTON_DOWN)  && dirY == 0) { dirX = 0; dirY = 1; }
        if ((pressed & BUTTON_LEFT)  && dirX == 0) { dirX = -1; dirY = 0; }
        if ((pressed & BUTTON_RIGHT) && dirX == 0) { dirX = 1; dirY = 0; }
    }
}

void updateSnake() {
    for (int i = snakeLength - 1; i > 0; --i) snake[i] = snake[i-1];
    snake[0].x += dirX;
    snake[0].y += dirY;
    
    // Wrap around screen edges
    if (snake[0].x < 0) snake[0].x = GRID_WIDTH - 1;
    if (snake[0].x >= GRID_WIDTH) snake[0].x = 0;
    if (snake[0].y < 0) snake[0].y = GRID_HEIGHT - 1;
    if (snake[0].y >= GRID_HEIGHT) snake[0].y = 0;
    
    // Check collision with self
    for (int i = 1; i < snakeLength; ++i) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            gameOver = true;
            gameOverCooldown = COOLDOWN_FRAMES;
            return;
        }
    }
    
    // Check apple collision
    if (snake[0].x == apple.x && snake[0].y == apple.y) {
        if (snakeLength < MAX_SNAKE) ++snakeLength;
        respawnApple();
    }
}

void Paused(XD2D_Texture* pTexture) {
    int localCooldown = pauseCooldown;
    
    while(isPaused && !g_quit) {
        uint32_t pressed = XPAD_GetButtons(0);
        
        // Check exit condition
        if ((pressed & BUTTON_BACK) && (pressed & BUTTON_START)) {
            g_quit = true;
            return;
        }
        
        // Check unpause
        if ((pressed & BUTTON_START) && (localCooldown == 0)) {
            isPaused = false;
            pauseCooldown = COOLDOWN_FRAMES;
            return;
        }
        
        if (localCooldown > 0) localCooldown--;
        
        XD2D_DrawTexture(pTexture, 0, 0);
        XD2D_Render();
    }
}

void Splash(XD2D_Texture* pTexture) {
    while(!IsGamePlaying && !g_quit) {
        uint32_t pressed = XPAD_GetButtons(0);
        
        // Check exit condition
        if ((pressed & BUTTON_BACK) && (pressed & BUTTON_START)) {
            g_quit = true;
            return;
        }
        
        if (pressed & BUTTON_A) {
            IsGamePlaying = true;
            return;
        }
        
        XD2D_DrawTexture(pTexture, 0, 0);
        XD2D_Render();
    }
}

void GameOver(XD2D_Texture* pTexture) {
    int localCooldown = gameOverCooldown;
    
    while(!IsGamePlaying && !g_quit) {
        uint32_t pressed = XPAD_GetButtons(0);
        
        // Check exit condition
        if ((pressed & BUTTON_BACK) && (pressed & BUTTON_START)) {
            g_quit = true;
            return;
        }
        
        if ((localCooldown == 0) && (pressed & BUTTON_A)) {
            IsGamePlaying = true;
            initGame();
            return;
        }
        
        if (localCooldown > 0) localCooldown--;
        
        XD2D_DrawTexture(pTexture, 0, 0);
        XD2D_Render();
    }
}

void drawFrame() {
    
    if (isPaused) {
        // Paused state handled in separate function
        return;
    }
    
    // Draw apple
    XD2D_DrawRectangle(CELL_SIZE, CELL_SIZE, 
                       apple.x * CELL_SIZE, 
                       apple.y * CELL_SIZE, 
                       0xFFFF0000);  // Red
    
    // Draw snake
    for (int i = 0; i < snakeLength; ++i) {
        DWORD color = (i == 0) ? 0xFF00FF00 : 0xFF008800;  // Head: Green, Body: Dark Green
        XD2D_DrawRectangle(CELL_SIZE, CELL_SIZE, 
                           snake[i].x * CELL_SIZE, 
                           snake[i].y * CELL_SIZE, 
                           color);
    }
}

int main() {
	XD2D_Init();
	XD2D_Texture* pDeath = XD2D_LoadTextureFromMemory(
        death_dds_data,       // From texture.h
        death_dds_size,       // From texture.h
        death_width,          // From texture.h
        death_height,         // From texture.h
        death_pot_width,      // From texture.h
        death_pot_height      // From texture.h
    );
	XD2D_Texture* pPaused = XD2D_LoadTextureFromMemory(
        paused_dds_data,       // From texture.h
        paused_dds_size,       // From texture.h
        paused_width,          // From texture.h
        paused_height,         // From texture.h
        paused_pot_width,      // From texture.h
        paused_pot_height      // From texture.h
    );
	XD2D_Texture* pSplash = XD2D_LoadTextureFromMemory(
        splash_dds_data,       // From texture.h
        splash_dds_size,       // From texture.h
        splash_width,          // From texture.h
        splash_height,         // From texture.h
        splash_pot_width,      // From texture.h
        splash_pot_height      // From texture.h
    );

    IsGamePlaying = false;
    Splash(pSplash);
    
    if (g_quit) return 0;
    
    initGame();

    while (!g_quit) {
        uint32_t pressed = XPAD_GetButtons(0);
        
        // Check exit combination (BACK + START)
        if ((pressed & BUTTON_BACK) && (pressed & BUTTON_START)) {
            break;
        }
        
        if (IsGamePlaying) {
            if (!gameOver && !isPaused) {
                handleInput(pressed);
                frameCounter++;
                if (frameCounter >= SNAKE_UPDATE_INTERVAL) {
                    updateSnake();
                    frameCounter = 0;
                }
            } 
            else if (gameOver) {
                IsGamePlaying = false;
                GameOver(pDeath);
                continue;
            }
        }
        
        drawFrame();
        
        if (isPaused) {
            Paused(pPaused);
        } else {
            XD2D_Render();
        }
    }
    
	XD2D_ReleaseTexture(pPaused);
	XD2D_ReleaseTexture(pDeath);
	XD2D_ReleaseTexture(pSplash);

	XD2D_End();
    return 0;
}