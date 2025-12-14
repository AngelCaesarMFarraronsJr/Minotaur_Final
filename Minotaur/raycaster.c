#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include <time.h>
#include "Textures/T_1.ppm"
#include "Textures/T_2.ppm"
#include "Textures/T_3.ppm"
#include "Textures/T_4.ppm"
#include "Textures/start.ppm"
#include "Textures/win.ppm"
#include "Textures/intro.ppm"

#define PI 3.1415926535
#define P2 PI/2
#define P3 3*PI/2
#define DR 0.0174533 // Degree to radian conversion
#define MAX_KEYS 5 // Maximum number of keys

// Player state variables
float px, py, pdx, pdy, pa;

// Game state variables
int keyStates[256] = {0}; // Keyboard input handler
int gameStarted = 0; // 0: Start Screen, 1: Game Running
int gameWon = 0; // 0: Not won, 1: Win Screen
int keysCollected = 0; // Current number of keys picked up
int keysRequired = 0; // Number of keys needed to win
int gameIntro = 0; // 0: Game, 1: Intro Screen
int playerPassedExitCheck = 0; // Flag set when player touches the door

float depthBuffer[1024]; // Stores distance to the closest wall for each vertical line

// Sprite structure for key
typedef struct {
    float x;
    float y;
    int active;
} Sprite;

Sprite keySprites[MAX_KEYS];

// MATH

float degToRad(float a) { return a*PI/180.0; }
float FixAng(float a) { if(a>2*PI){ a-=2*PI;} if(a<0){ a+=2*PI;} return a; }

float dist(float ax, float ay, float bx, float by)
{
    return sqrtf((bx-ax)*(bx-ax) + (by-ay)*(by-ay));
}

#define CHECK_WHITE_PIXEL(r, g, b) (r == 255 && g == 255 && b == 255)

// SCREEN RENDERING (2D)

void drawStartScreen()
{
    int x, y;
    for(y=0; y<512; y++)
    {
        for(x=0; x<1024; x++)
        {
            int pixel = (y * 1024 + x) * 3;
            
            if(pixel >= 0 && pixel < 1024*512*3 - 2)
            {
                int red   = start[pixel+0];
                int green = start[pixel+1];
                int blue  = start[pixel+2];
                
                if(!CHECK_WHITE_PIXEL(red, green, blue)) 
                {
                    glPointSize(1);
                    glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                    glBegin(GL_POINTS);
                    glVertex2i(x, y);
                    glEnd();
                }
            }
        }
    }
}

void drawIntroScreen()
{
    int x, y;
    for(y=0; y<512; y++)
    {
        for(x=0; x<1024; x++)
        {
            int pixel = (y * 1024 + x) * 3;
            
            if(pixel >= 0 && pixel < 1024*512*3 - 2)
            {
                int red   = intro[pixel+0];
                int green = intro[pixel+1];
                int blue  = intro[pixel+2];
                
                if(!CHECK_WHITE_PIXEL(red, green, blue)) 
                {
                    glPointSize(1);
                    glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                    glBegin(GL_POINTS);
                    glVertex2i(x, y);
                    glEnd();
                }
            }
        }
    }
}

void drawWinScreen()
{
    int x, y;
    for(y=0; y<512; y++)
    {
        for(x=0; x<1024; x++)
        {
            int pixel = (y * 1024 + x) * 3;
            
            if(pixel >= 0 && pixel < 1024*512*3 - 2)
            {
                int red   = win[pixel+0];
                int green = win[pixel+1];
                int blue  = win[pixel+2];
                
                if(!CHECK_WHITE_PIXEL(red, green, blue))
                {
                    glPointSize(1);
                    glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                    glBegin(GL_POINTS);
                    glVertex2i(x, y);
                    glEnd();
                }
            }
        }
    }
}

// MAP GENERATION & COLLISION

int mapX=16, mapY=16, mapS=64; // Map dimensions
// Dynamic map arrays
int map[16*16]; 
int mapFloor[16*16];
int mapCeiling[16*16]; 

void initializeFloorCeiling() {
    int i;
    for(i = 0; i < mapX * mapY; i++) {
        mapFloor[i] = 2; // Texture T_2
        mapCeiling[i] = 3; // Texture T_3
    }
}

// Initializes the map
void initializeMap()
{
    int x, y;
    for (y = 0; y < mapY; y++) {
        for (x = 0; x < mapX; x++) {
            map[y * mapX + x] = 1;
        }
    }
}

// Maze generation
void generateMaze()
{
    int stackX[256];
    int stackY[256];
    int top = 0;
    int x, y, nx, ny, i, dir;
    int dx[] = {0, 0, 2, -2}; // Directions (2 steps)
    int dy[] = {2, -2, 0, 0}; 

    // 1. Initialize all cells to walls
    initializeMap();

    // 2. Start from an initial point
    x = 1; y = 1;
    map[y * mapX + x] = 0; // Carve out the starting cell

    // Push starting cell to stack
    stackX[top] = x;
    stackY[top] = y;
    top++;

    while (top > 0) 
    {
        // Peek current cell
        x = stackX[top-1];
        y = stackY[top-1];

        // 3. Randomly shuffle directions
        int directions[4] = {0, 1, 2, 3};
        for (i = 0; i < 4; i++) {
            int j = rand() % 4;
            int temp = directions[i];
            directions[i] = directions[j];
            directions[j] = temp;
        }

        int foundNeighbor = 0;
        // 4. Try all shuffled directions
        for (i = 0; i < 4; i++) {
            dir = directions[i];
            nx = x + dx[dir];
            ny = y + dy[dir];

            // Check boundaries (must be an interior cell and not the border)
            if (nx > 0 && nx < mapX - 1 && ny > 0 && ny < mapY - 1) 
            {
                int next_mp = ny * mapX + nx;
                
                // If the next cell is a wall, carve a path
                if (map[next_mp] == 1) 
                {
                    // Carve out cell in between
                    map[(y + dy[dir] / 2) * mapX + (x + dx[dir] / 2)] = 0;
                    // Carve out new cell
                    map[next_mp] = 0;

                    // Push new cell
                    stackX[top] = nx;
                    stackY[top] = ny;
                    top++;
                    foundNeighbor = 1;
                    break; // Move to new cell
                }
            }
        }
        
        if (!foundNeighbor) {
            // Backtrack
            top--;
        }
    }
}

// Place the door (4)
void placeDoor() {
    int rx, ry, mp;
    int found = 0;
    
    while (!found) {
        
        // Randomly pick one of the four edges (x=0, x=15, y=0, y=15)
        int edge = rand() % 4;
        
        if (edge == 0) { // Top border
            rx = (rand() % (mapX - 2)) + 1;
            ry = 0;
        } else if (edge == 1) { // Bottom border
            rx = (rand() % (mapX - 2)) + 1;
            ry = mapY - 1;
        } else if (edge == 2) { // Left border
            rx = 0;
            ry = (rand() % (mapY - 2)) + 1;
        } else { // Right border
            rx = mapX - 1;
            ry = (rand() % (mapY - 2)) + 1;
        }

        mp = ry * mapX + rx;
        
        int nx = rx, ny = ry;
        if (edge == 0) ny = 1; // Cell below
        if (edge == 1) ny = mapY - 2; // Cell above
        if (edge == 2) nx = 1; // Cell to the right
        if (edge == 3) nx = mapX - 2; // Cell to the left
        
        // Check the spot is not the player spawn
        if (rx != 1 || ry != 1) {
            // Check if the adjacent interior cell is a walkable (0)
            if (map[ny * mapX + nx] == 0) {
                map[mp] = 4; // Place door
                found = 1;
            }
        }
    }
}


// Key spawn function
void findRandomEmptySpot(float *outX, float *outY) {
    int rx, ry, mp;
    int found = 0;
    while (!found) {
        // Map coord RNG
        rx = (rand() % (mapX - 2)) + 1;
        ry = (rand() % (mapY - 2)) + 1;
        
        mp = ry * mapX + rx;
        
        // Check if the cell is walkable (value 0)
        if (map[mp] == 0 && !(rx == 1 && ry == 1)) {
            // Check if another key is already here
            int i;
            int keyOverlap = 0;
            for(i = 0; i < MAX_KEYS; i++) {
                int keyMx = (int)(keySprites[i].x) >> 6;
                int keyMy = (int)(keySprites[i].y) >> 6;
                if(keySprites[i].active && keyMx == rx && keyMy == ry) {
                    keyOverlap = 1;
                    break;
                }
            }
            
            if (!keyOverlap) {
                *outX = rx * mapS + mapS / 2.0f;
                *outY = ry * mapS + mapS / 2.0f;
                found = 1;
            }
        }
    }
}


int checkCollision(float x, float y)
{
    int mx = (int)(x) >> 6;
    int my = (int)(y) >> 6;
    int mp = my * mapX + mx;

    if(mp < 0 || mp >= mapX * mapY) return 1;

    if(map[mp] == 1) return 1; // Regular wall
    
    // Door logic
    if(map[mp] == 4) {
        if(keysCollected >= keysRequired) {
            // Player has enough keys
            playerPassedExitCheck = 1; 
            return 0; // Win state trigger
        } else {
            return 1; // Blocked
        }
    }

    return 0;
}

// HUD

void drawText(char *string, int x, int y, float r, float g, float b)
{
    glColor3f(r, g, b);
    glRasterPos2i(x, y);
    while (*string) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *string++);
    }
}


// RAYCASTING & RENDERING
void drawRays3D()
{
    int r,mx,my,mp,dof,y,i;
    float rx,ry,ra,xo,yo,disT;

    ra = pa - DR*30.0f;
    ra = FixAng(ra); 

    for(r=0;r<128;r++) 
    {
        float disH, hx, hy; 
        int hMapValue;
        float aTan;
        float disV, vx, vy; 
        int vMapValue;
        float nTan;
        float ca;
        float lineH;
        float lineOff;
        float shade;
        int texX;
        int wallType;

        // Horizontal Check 
        dof=0; disH=1e6f; hMapValue=0; aTan = -1.0f / tanf(ra);
        if(ra > PI) { ry = (((int)py>>6)<<6) - 0.0001f; rx = (py - ry) * aTan + px; yo = -64; xo = -yo * aTan; }
        else { ry = (((int)py>>6)<<6) + 64; rx = (py - ry) * aTan + px; yo = 64; xo = -yo * aTan; }
        if(fabsf(ra - 0.0f) < 1e-6 || fabsf(ra - PI) < 1e-6) { rx = px; ry = py; dof = 8; } 
        while(dof < 8) 
        {
            mx = (int)(rx) >> 6; my = (int)(ry) >> 6; mp = my * mapX + mx;
            if(mp >= 0 && mp < mapX*mapY) {
                if(map[mp] == 1 || map[mp] == 4) { 
                    hx = rx; hy = ry; disH = dist(px,py,hx,hy); 
                    hMapValue = map[mp]; 
                    dof = 8; 
                } else { rx += xo; ry += yo; dof++; }
            } else { rx += xo; ry += yo; dof++; } 
        }

        // Vertical Check
        dof = 0; disV = 1e6f; vMapValue=0; nTan = -tanf(ra);
        if(ra > P2 && ra < P3) { rx = (((int)px>>6)<<6) - 0.0001f; ry = (px - rx) * nTan + py; xo = -64; yo = -xo * nTan; }
        else { rx = (((int)px>>6)<<6) + 64; ry = (px - rx) * nTan + py; xo = 64; yo = -xo * nTan; }
        if(fabsf(ra - P2) < 1e-6 || fabsf(ra - P3) < 1e-6) { rx = px; ry = py; dof = 8; } 

        while(dof < 8)
        {
            mx = (int)(rx) >> 6; my = (int)(ry) >> 6; mp = my * mapX + mx;
            if(mp >= 0 && mp < mapX*mapY) {
                if(map[mp] == 1 || map[mp] == 4) { 
                    vx = rx; vy = ry; disV = dist(px,py,vx,vy);
                    vMapValue = map[mp]; 
                    dof = 8; 
                } else { rx += xo; ry += yo; dof++; }
            } else { rx += xo; ry += yo; dof++; } 
        }

        // Final Distance Selection & Fish-eye Correction
        if(disV < disH) { rx = vx; ry = vy; disT = disV; wallType = vMapValue; shade = 1.0f; }
        else { rx = hx; ry = hy; disT = disH; wallType = hMapValue; shade = 0.7f; }

        ca = pa - ra; 
        ca = FixAng(ca); 
        disT *= cosf(ca); 

        // Store distance in the depth buffer
        for(i = r*8; i < (r+1)*8 && i < 1024; i++) {
            depthBuffer[i] = disT;
        }

        // Calculate wall height and vertical offset
        lineH = (mapS * 512.0f) / disT;
        if(lineH > 512) lineH = 512;
        lineOff = 256.0f - lineH/2.0f;

        // Draw Ceiling (T_3)
        for(y = 0; y < (int)lineOff; y++)
        {
            float dy = 256.0f - y;
            float ceilingDist;
            float wx, wy;
            int localX, localY;
            int ceilingTexX, ceilingTexY;
            int ceilingPixel;
            
            if(fabsf(dy) < 1e-6f) continue;
            
            float currentRa = ra;
            float caFix = cosf(pa - currentRa); 
            ceilingDist = (mapS * 512.0f) / (dy * 2.0f * caFix);
            
            wx = px + cosf(currentRa) * ceilingDist;
            wy = py + sinf(currentRa) * ceilingDist;
            
            localX = ((int)wx) % 32;
            localY = ((int)wy) % 32;
            if(localX < 0) localX += 32;
            if(localY < 0) localY += 32;
            
            ceilingTexX = localX;
            ceilingTexY = localY;
            
            ceilingPixel = (ceilingTexY * 32 + ceilingTexX) * 3;
            
            if(ceilingPixel >= 0 && ceilingPixel < 32*32*3 - 2)
            {
                int red   = T_3[ceilingPixel+0];
                int green = T_3[ceilingPixel+1];
                int blue  = T_3[ceilingPixel+2];
                
                glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                glPointSize(8); 
                glBegin(GL_POINTS);
                glVertex2i(r*8, y);
                glEnd();
            }
        }

        // Wall Texture Calculation
        if(disV < disH) {
            texX = ((int)ry) % 32;
            if(texX < 0) texX += 32;
            shade = 1.0f;
        } else {
            texX = ((int)rx) % 32;
            if(texX < 0) texX += 32;
            shade = 0.7f; 
        }
        
        // Draw Wall
        for(y = 0; y < (int)lineH; y++)
        {
            int texY = (y * 32) / (int)lineH;
            int pixel;
            int red, green, blue;
            
            if(texY < 0) texY = 0; if(texY >= 32) texY = 31;
            if(texX < 0) texX = 0; if(texX >= 32) texX = 31;

            pixel = (texY * 32 + texX) * 3;
            if(pixel >= 0 && pixel < 32*32*3 - 2)
            {
                if(wallType == 4) {
                    red   = (int)(T_4[pixel+0] * shade);
                    green = (int)(T_4[pixel+1] * shade);
                    blue  = (int)(T_4[pixel+2] * shade);
                } else {
                    red   = (int)(T_1[pixel+0] * shade);
                    green = (int)(T_1[pixel+1] * shade);
                    blue  = (int)(T_1[pixel+2] * shade);
                }

                glPointSize(8);
                glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                glBegin(GL_POINTS);
                glVertex2i(r*8, (int)(y + lineOff));
                glEnd();
            }
        }

        // Draw Floor (T_2)
        for(y = (int)(lineH + lineOff); y < 512; y++)
        {
            float dy = y - 256.0f;
            float floorDist;
            float wx, wy;
            int localX, localY;
            int floorTexX, floorTexY;
            int floorPixel;
            
            if(fabsf(dy) < 1e-6f) continue;
            
            float currentRa = ra;
            float caFix = cosf(pa - currentRa);
            floorDist = (mapS * 512.0f) / (dy * 2.0f * caFix);
            
            wx = px + cosf(currentRa) * floorDist;
            wy = py + sinf(currentRa) * floorDist;
            
            localX = ((int)wx) % 32;
            localY = ((int)wy) % 32;
            if(localX < 0) localX += 32;
            if(localY < 0) localY += 32;
            
            floorTexX = localX;
            floorTexY = localY;
            
            floorPixel = (floorTexY * 32 + floorTexX) * 3;
            
            if(floorPixel >= 0 && floorPixel < 32*32*3 - 2)
            {
                int red   = T_2[floorPixel+0];
                int green = T_2[floorPixel+1];
                int blue  = T_2[floorPixel+2];
                
                glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                glPointSize(8);
                glBegin(GL_POINTS);
                glVertex2i(r*8, y);
                glEnd();
            }
        }
        
        ra += DR*0.5f;
        ra = FixAng(ra);
    }
    
}

// SPRITE RENDERING (KEYS)
void drawSprite(Sprite *s, const unsigned char *tex, int texWidth, int texHeight)
{
    float spx, spy;
    float spriteAngle;
    float spriteDist;
    float spriteHeight;
    float spriteWidth;
    int spriteScreenX;
    int startX, endX, startY, endY;
    int x, y; 
    
    if(!s->active) return;
    
    spx = s->x - px;
    spy = s->y - py;
    
    spriteAngle = atan2f(spy, spx) - pa;
    while(spriteAngle < -PI) spriteAngle += 2*PI;
    while(spriteAngle > PI) spriteAngle -= 2*PI;
    
    if(spriteAngle < -DR * 30.0f || spriteAngle > DR * 30.0f) return;
    
    spriteDist = sqrtf(spx*spx + spy*spy);
    if(spriteDist < 1) return;
    
    spriteHeight = (mapS * 512.0f) / spriteDist;
    if(spriteHeight > 512) spriteHeight = 512;
    
    spriteWidth = spriteHeight; 
    
    spriteScreenX = (int)(512 + (spriteAngle / (DR * 0.5f) * 8)); 
    
    startX = spriteScreenX - (int)(spriteWidth / 2);
    endX = spriteScreenX + (int)(spriteWidth / 2);
    startY = 256 - (int)(spriteHeight / 2);
    endY = 256 + (int)(spriteHeight / 2);
    
    if(startX < 0) startX = 0;
    if(endX > 1024) endX = 1024;
    if(startY < 0) startY = 0;
    if(endY > 512) endY = 512;
    
    for(x = startX; x < endX; x++)
    {
        if(x < 0 || x >= 1024) continue;
        
        if(spriteDist >= depthBuffer[x]) continue;
        
        for(y = startY; y < endY; y++)
        {
            float nx = (float)(x - startX) / spriteWidth;
            float ny = (float)(y - startY) / spriteHeight;
            
            int drawPixel = 0;
            
            // Key head
            float headCenterY = 0.25f;
            float headRadius = 0.15f;
            float distToHead = sqrtf((nx - 0.5f)*(nx - 0.5f) + (ny - headCenterY)*(ny - headCenterY));
            if(distToHead < headRadius) drawPixel = 1;
            
            // Key hole
            float holeCenterY = 0.25f;
            float holeRadius = 0.06f;
            float distToHole = sqrtf((nx - 0.5f)*(nx - 0.5f) + (ny - holeCenterY)*(ny - holeCenterY));
            if(distToHole < holeRadius) drawPixel = 0;
            
            // Key shaft
            if(nx > 0.43f && nx < 0.57f && ny > 0.35f && ny < 0.75f) drawPixel = 1;
            
            // Key teeth
            if(nx > 0.43f && nx < 0.50f && ny > 0.70f && ny < 0.78f) drawPixel = 1;
            if(nx > 0.43f && nx < 0.50f && ny > 0.82f && ny < 0.90f) drawPixel = 1;
            
            if(drawPixel)
            {
                glColor3f(1.0f, 0.84f, 0.0f);
                glPointSize(1);
                glBegin(GL_POINTS);
                glVertex2i(x, y);
                glEnd();
            }
        }
    }
}

// LOGIC

void updateMovement()
{
    float walkSpeed = 3.0f;
    float rotSpeed = 0.05f;
    float newX, newY;
    int moved = 0;
    int i;
    
    if(!gameStarted || gameWon || gameIntro) return;
    
    newX = px;
    newY = py;

    // Rotation
    if(keyStates['a']) {
        pa -= rotSpeed;
        if(pa < 0) { pa += 2*PI; }
        pdx = cosf(pa)*5.0f;
        pdy = sinf(pa)*5.0f;
        moved = 1;
    }
    if(keyStates['d']) {
        pa += rotSpeed;
        if(pa > 2*PI) { pa -= 2*PI; }
        pdx = cosf(pa)*5.0f;
        pdy = sinf(pa)*5.0f;
        moved = 1;
    }

    // Movement
    if(keyStates['w']) {
        newX += cosf(pa) * walkSpeed;
        newY += sinf(pa) * walkSpeed;
        moved = 1;
    }
    if(keyStates['s']) {
        newX -= cosf(pa) * walkSpeed;
        newY -= sinf(pa) * walkSpeed;
        moved = 1;
    }

    // Collision check and update player
    if(!checkCollision(newX, newY))
    {
        px = newX;
        py = newY;
    }
    
    // Check for keys
    for(i = 0; i < keysRequired; i++)
    {
        if(keySprites[i].active)
        {
            float distToKey = dist(px, py, keySprites[i].x, keySprites[i].y);
            if(distToKey < 20) // Distance check
            {
                keySprites[i].active = 0;
                keysCollected++;
            }
        }
    }

    // Exit check
    if(playerPassedExitCheck)
    {
        // Give a small delay before the win screen appears
        if (keysCollected >= keysRequired) {
            gameWon = 1; 
        } else {
             playerPassedExitCheck = 0; // Should not happen due to checkCollision, but for safety
        }
    }
    
    if(moved || gameWon) {
        glutPostRedisplay();
    }
}

void display()
{
    int i;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if(gameWon)
    {
        drawWinScreen();
    }
    else if(!gameStarted)
    {
        drawStartScreen();
    }
    else if(gameIntro)
    {
        drawIntroScreen();
    }
    else // (Maze + Keys)
    {
        // Draw black background (floor/ceiling fallback)
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(1024, 0);
        glVertex2i(1024, 512);
        glVertex2i(0, 512);
        glEnd();
        
        drawRays3D();
        
        // Draw key sprite
        for(i = 0; i < keysRequired; i++)
        {
            drawSprite(&keySprites[i], NULL, 32, 32); 
        }
        

        // Draw HUD
        char hudText[50];
        sprintf(hudText, "Keys: %d / %d", keysCollected, keysRequired);
        drawText(hudText, 10, 500, 1.0f, 1.0f, 0.0f); // (Keys collected/required)
        
        // Door Status
        if (keysCollected < keysRequired) {
            drawText("FIND THE KEYS TO ESCAPE (W, A, S, D)", 150, 500, 1.0f, 0.0f, 0.0f); // (Locked)
        } else {
            drawText("EXIT IS OPEN! PRESS W TO ESCAPE.", 150, 500, 0.0f, 1.0f, 0.0f); // (Unlocked)
        }
    }
    
    glutSwapBuffers();
}

void resetGame() {
    float keyX, keyY;
    int i; 
    
    // Generate a new random map
    generateMaze(); 
    placeDoor();    
    initializeFloorCeiling(); 

    // Reset player
    px = 1 * mapS + mapS/2;
    py = 1 * mapS + mapS/2;
    pa = 0;
    pdx = cosf(pa) * 5.0f;
    pdy = sinf(pa) * 5.0f;
    
    keysCollected = 0;
    playerPassedExitCheck = 0;
    
    // Re-initialize key states
    keysRequired = (rand() % MAX_KEYS) + 1; // Keys required (1-5)

    for(i = 0; i < keysRequired; i++)
    {
        findRandomEmptySpot(&keyX, &keyY);
        keySprites[i].x = keyX;
        keySprites[i].y = keyY;
        keySprites[i].active = 1;
    }
    // Deactivate unused slots
    for(i = keysRequired; i < MAX_KEYS; i++) {
        keySprites[i].active = 0;
    }
}


void keyDown(unsigned char key, int x, int y)
{
    if(gameWon)
    {
        // Reset game state after winning
        gameWon = 0;
        gameStarted = 0;
        gameIntro = 0;
        
        resetGame(); // Regenerate map, keys, and player position
        
        glutPostRedisplay();
        return;
    }
    
    if(!gameStarted)
    {
        // Start Screen > Intro Screen
        gameStarted = 1;
        gameIntro = 1; // Set intro state active
        glutPostRedisplay();
    }
    else if(gameIntro)
    {
        // Intro Screen > Game
        gameIntro = 0; // Set intro state inactive, entering main game loop
        glutPostRedisplay();
    }
    
    // Store key state for continuous movement
    keyStates[key] = 1;
}

void keyUp(unsigned char key, int x, int y)
{
    keyStates[key] = 0;
}

void timer(int value)
{
    updateMovement();
    glutTimerFunc(16, timer, 0);
}

void resize(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1024, 512, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void init()
{
    glClearColor(0.3f, 0.3f, 0.3f, 0);
    gluOrtho2D(0, 1024, 512, 0); 

    resetGame(); // Initial game setup
}

int main(int argc, char* argv[])
{
    // RNG
    srand(time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1024, 512);
    
    // Window Dimensions
    int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    int windowX = (screenWidth - 1024) / 2;
    int windowY = (screenHeight - 512) / 2;
    
    glutInitWindowPosition(windowX, windowY);
    glutCreateWindow("RayCaster");
    
    init();
    
    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutReshapeFunc(resize);
    glutTimerFunc(0, timer, 0);
    
    glutMainLoop();
    return 0;
}
