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
int keysRequired = 0; // Number of keys needed to win (1-5)
int gameIntro = 0; // 0: Game, 1: Intro Screen
int playerPassedExitCheck = 0; // Flag set when player touches the door

float depthBuffer[1024]; // Stores distance to the closest wall for each vertical line

// Sprite structure for keys
typedef struct {
    float x;
    float y;
    int active;
} Sprite;

Sprite keySprites[MAX_KEYS];

// --- MATH ---

float degToRad(float a) { return a*PI/180.0; }
float FixAng(float a) { if(a>2*PI){ a-=2*PI;} if(a<0){ a+=2*PI;} return a; } // Fixed to use 2*PI for radians

float dist(float ax, float ay, float bx, float by)
{
    return sqrtf((bx-ax)*(bx-ax) + (by-ay)*(by-ay));
}

#define CHECK_WHITE_PIXEL(r, g, b) (r == 255 && g == 255 && b == 255)

// --- SCREEN RENDERING (2D) ---

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
                
                // Draw only non-white pixels
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
                
                // Draw only non-white pixels
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
                
                // Draw only non-white pixels
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

// --- MAP & COLLISION ---

int mapX=16, mapY=16, mapS=64; // Map size and cell size
int map[]=
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,
    1,0,1,0,0,1,0,1,1,1,0,1,0,1,0,1,
    1,0,1,0,0,0,0,1,0,0,0,0,0,1,0,1,
    1,0,1,1,1,1,0,1,0,1,1,1,1,1,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,
    1,1,1,1,0,1,1,0,1,1,0,1,1,0,1,1,
    1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,
    1,0,1,1,1,1,1,1,0,1,1,1,0,1,0,1,
    1,0,1,0,0,1,0,0,0,0,0,1,0,1,0,1,
    1,0,0,0,1,1,1,1,1,1,0,1,0,1,0,1,
    1,0,1,0,1,0,0,0,0,1,0,0,0,1,0,1,
    1,0,1,0,0,0,0,0,0,1,0,1,1,1,0,1,
    1,1,1,0,1,1,1,0,1,1,0,0,0,1,0,1,
    1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,4,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

int mapFloor[]= // Floor texture index
{
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
};

int mapCeiling[]= // Ceiling texture index
{
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
};

// Function to spawn keys
void findRandomEmptySpot(float *outX, float *outY) {
    int rx, ry, mp;
    int found = 0;
    while (!found) {
        // Generate random map coordinates
        rx = (rand() % (mapX - 2)) + 1;
        ry = (rand() % (mapY - 2)) + 1;
        
        mp = ry * mapX + rx;
        
        // Check if the cell is walkable (value 0)
        if (map[mp] == 0 && !(rx == 1 && ry == 1)) {
            *outX = rx * mapS + mapS / 2.0f;
            *outY = ry * mapS + mapS / 2.0f;
            found = 1;
        }
    }
}


int checkCollision(float x, float y)
{
    int mx = (int)(x) >> 6;
    int my = (int)(y) >> 6;
    int mp = my * mapX + mx;

    if(mp < 0 || mp >= mapX * mapY) return 1; // Out of bounds is a wall

    if(map[mp] == 1) return 1; // Regular wall
    
    // Door (4) logic: Check if required keys are collected
    if(map[mp] == 4) {
        if(keysCollected >= keysRequired) {
            // Player has enough keys and hits the exit door.
            playerPassedExitCheck = 1; 
            return 0; // Allowed to pass through (triggers game win on move update)
        } else {
            return 1; // Blocked: Need more keys
        }
    }

    return 0;
}

// --- HUD ---

void drawText(char *string, int x, int y, float r, float g, float b)
{
    glColor3f(r, g, b);
    glRasterPos2i(x, y);
    while (*string) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *string++);
    }
}


// --- RAYCASTING & RENDERING ---
void drawRays3D()
{
    int r,mx,my,mp,dof,y,i;
    float rx,ry,ra,xo,yo,disT;

    // Start angle is Player Angle (pa) minus half of the Field of View (30 degrees)
    ra = pa - DR*30.0f;
    ra = FixAng(ra); // Normalize angle

    for(r=0;r<128;r++) // Loop through 128 rays (for 1024 pixel width, 8 pixels per ray)
    {
        float disH, hx, hy; // Horizontal hit data
        int hMapValue;
        float aTan;
        float disV, vx, vy; // Vertical hit data
        int vMapValue;
        float nTan;
        float ca;
        float lineH;
        float lineOff;
        float shade;
        int texX;
        int wallType;

        // --- Horizontal Check (Checking for horizontal grid lines) ---
        dof=0;
        disH=1e6f;
        hMapValue=0;
        aTan = -1.0f / tanf(ra);

        // Looking Up (ra > PI)
        if(ra > PI) { 
            ry = (((int)py>>6)<<6) - 0.0001f; // Snap to the horizontal line just above player
            rx = (py - ry) * aTan + px; // Calculate x intersection
            yo = -64; 
            xo = -yo * aTan; 
        }
        // Looking Down (ra < PI)
        else { 
            ry = (((int)py>>6)<<6) + 64; // Snap to the horizontal line just below player
            rx = (py - ry) * aTan + px; // Calculate x intersection
            yo = 64; 
            xo = -yo * aTan; 
        }
        // Looking Straight Left or Right (0 or PI)
        if(fabsf(ra - 0.0f) < 1e-6 || fabsf(ra - PI) < 1e-6) { rx = px; ry = py; dof = 8; } 

        while(dof < 8) // Limit ray distance to 8 map cells
        {
            mx = (int)(rx) >> 6; my = (int)(ry) >> 6; mp = my * mapX + mx;
            if(mp >= 0 && mp < mapX*mapY) {
                // If the ray hits a wall (1) OR a door (4), record the hit.
                if(map[mp] == 1 || map[mp] == 4) { 
                    hx = rx; hy = ry; disH = dist(px,py,hx,hy); 
                    hMapValue = map[mp]; // Store map value
                    dof = 8; // Stop the ray
                } else { // Ray passes through open space
                    rx += xo; ry += yo; dof++; 
                }
            } else { rx += xo; ry += yo; dof++; } // Out of map bounds
        }

        // --- Vertical Check (Checking for vertical grid lines) ---
        dof = 0;
        disV = 1e6f;
        vMapValue=0;
        nTan = -tanf(ra);

        // Looking Left (ra > P2 && ra < P3)
        if(ra > P2 && ra < P3) { 
            rx = (((int)px>>6)<<6) - 0.0001f; // Snap to the vertical line just left of player
            ry = (px - rx) * nTan + py; 
            xo = -64; 
            yo = -xo * nTan; 
        }
        // Looking Right
        else { 
            rx = (((int)px>>6)<<6) + 64; // Snap to the vertical line just right of player
            ry = (px - rx) * nTan + py; 
            xo = 64; 
            yo = -xo * nTan; 
        }
        // Looking Straight Up or Down (P2 or P3)
        if(fabsf(ra - P2) < 1e-6 || fabsf(ra - P3) < 1e-6) { rx = px; ry = py; dof = 8; } 

        while(dof < 8)
        {
            mx = (int)(rx) >> 6; my = (int)(ry) >> 6; mp = my * mapX + mx;
            if(mp >= 0 && mp < mapX*mapY) {
                // If the ray hits a wall (1) OR a door (4), record the hit.
                if(map[mp] == 1 || map[mp] == 4) { 
                    vx = rx; vy = ry; disV = dist(px,py,vx,vy);
                    vMapValue = map[mp]; // Store map value
                    dof = 8; 
                } else { // Ray passes through open space
                    rx += xo; ry += yo; dof++; 
                }
            } else { rx += xo; ry += yo; dof++; } // Out of map bounds
        }

        // --- Final Distance Selection & Fish-eye Correction ---
        if(disV < disH) { rx = vx; ry = vy; disT = disV; wallType = vMapValue; shade = 1.0f; }
        else { rx = hx; ry = hy; disT = disH; wallType = hMapValue; shade = 0.7f; }

        ca = pa - ra; // Angle difference
        ca = FixAng(ca); // Normalize
        disT *= cosf(ca); // Fish-eye correction

        // Store distance in the depth buffer for sprite rendering
        for(i = r*8; i < (r+1)*8 && i < 1024; i++) {
            depthBuffer[i] = disT;
        }

        // Calculate wall height and vertical offset
        lineH = (mapS * 512.0f) / disT;
        if(lineH > 512) lineH = 512;
        lineOff = 256.0f - lineH/2.0f;

        // --- Draw Ceiling (T_3) ---
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
            
            // Calculate world coordinates for ceiling hit point
            wx = px + cosf(currentRa) * ceilingDist;
            wy = py + sinf(currentRa) * ceilingDist;
            
            // Map world coordinates to 32x32 texture coordinates
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
                glPointSize(8); // Draw the pixel block (8x8)
                glBegin(GL_POINTS);
                glVertex2i(r*8, y);
                glEnd();
            }
        }

        // --- Wall Texture Calculation ---
        if(disV < disH) {
            // Vertical wall hit: X texture coordinate is based on Y world coordinate
            texX = ((int)ry) % 32;
            if(texX < 0) texX += 32;
            shade = 1.0f;
        } else {
            // Horizontal wall hit: X texture coordinate is based on X world coordinate
            texX = ((int)rx) % 32;
            if(texX < 0) texX += 32;
            shade = 0.7f; // Darker shade for horizontal walls
        }
        
        // --- Draw Wall ---
        for(y = 0; y < (int)lineH; y++)
        {
            // Calculate Y texture coordinate
            int texY = (y * 32) / (int)lineH;
            int pixel;
            int red, green, blue;
            
            // Boundary checks
            if(texY < 0) texY = 0; if(texY >= 32) texY = 31;
            if(texX < 0) texX = 0; if(texX >= 32) texX = 31;

            pixel = (texY * 32 + texX) * 3;
            if(pixel >= 0 && pixel < 32*32*3 - 2)
            {
                if(wallType == 4) {
                    // Door always draws T_4
                    red   = (int)(T_4[pixel+0] * shade);
                    green = (int)(T_4[pixel+1] * shade);
                    blue  = (int)(T_4[pixel+2] * shade);
                } else {
                    // Regular Wall (T_1)
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

        // --- Draw Floor (T_2) ---
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
            // Calculate distance to the point on the floor
            floorDist = (mapS * 512.0f) / (dy * 2.0f * caFix);
            
            // Calculate world coordinates
            wx = px + cosf(currentRa) * floorDist;
            wy = py + sinf(currentRa) * floorDist;
            
            // Map world coordinates to 32x32 texture coordinates
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
        
        // Increment angle for the next ray
        ra += DR*0.5f;
        ra = FixAng(ra);
    }
    
}

// --- SPRITE RENDERING (KEYS) ---
void drawSprite(Sprite *s, const unsigned char *tex, int texWidth, int texHeight)
{
    float spx, spy;
    float spriteAngle;
    float spriteDist;
    float spriteHeight;
    float spriteWidth;
    int spriteScreenX;
    int startX, endX, startY, endY;
    int x, y; // Declared x and y here for C89 compliance
    
    if(!s->active) return;
    
    // Calculate player-to-sprite vector
    spx = s->x - px;
    spy = s->y - py;
    
    // Calculate sprite angle relative to player view angle
    spriteAngle = atan2f(spy, spx) - pa;
    while(spriteAngle < -PI) spriteAngle += 2*PI;
    while(spriteAngle > PI) spriteAngle -= 2*PI;
    
    // Frustum Culling: Only draw sprites within the 60 degree field of view
    if(spriteAngle < -DR * 30.0f || spriteAngle > DR * 30.0f) return;
    
    // Calculate distance
    spriteDist = sqrtf(spx*spx + spy*spy);
    if(spriteDist < 1) return;
    
    // Calculate projected height
    spriteHeight = (mapS * 512.0f) / spriteDist;
    if(spriteHeight > 512) spriteHeight = 512;
    
    spriteWidth = spriteHeight; // Keep aspect ratio
    
    // Calculate screen X position
    spriteScreenX = (int)(512 + (spriteAngle / (DR * 0.5f) * 8)); 
    
    // Calculate screen boundaries
    startX = spriteScreenX - (int)(spriteWidth / 2);
    endX = spriteScreenX + (int)(spriteWidth / 2);
    startY = 256 - (int)(spriteHeight / 2);
    endY = 256 + (int)(spriteHeight / 2);
    
    // Clamp to screen edges
    if(startX < 0) startX = 0;
    if(endX > 1024) endX = 1024;
    if(startY < 0) startY = 0;
    if(endY > 512) endY = 512;
    
    // Draw the key sprite
    for(x = startX; x < endX; x++)
    {
        if(x < 0 || x >= 1024) continue;
        
        // Depth check
        if(spriteDist >= depthBuffer[x]) continue;
        
        for(y = startY; y < endY; y++)
        {
            // Normalize coordinates
            float nx = (float)(x - startX) / spriteWidth;
            float ny = (float)(y - startY) / spriteHeight;
            
            int drawPixel = 0;
            
            // Key head (circle at top)
            float headCenterY = 0.25f;
            float headRadius = 0.15f;
            float distToHead = sqrtf((nx - 0.5f)*(nx - 0.5f) + (ny - headCenterY)*(ny - headCenterY));
            if(distToHead < headRadius) drawPixel = 1;
            
            // Key hole (cutout in the head)
            float holeCenterY = 0.25f;
            float holeRadius = 0.06f;
            float distToHole = sqrtf((nx - 0.5f)*(nx - 0.5f) + (ny - holeCenterY)*(ny - holeCenterY));
            if(distToHole < holeRadius) drawPixel = 0;
            
            // Key shaft (rectangle)
            if(nx > 0.43f && nx < 0.57f && ny > 0.35f && ny < 0.75f) drawPixel = 1;
            
            // Key teeth (bits at the bottom)
            if(nx > 0.43f && nx < 0.50f && ny > 0.70f && ny < 0.78f) drawPixel = 1;
            if(nx > 0.43f && nx < 0.50f && ny > 0.82f && ny < 0.90f) drawPixel = 1;
            
            if(drawPixel)
            {
                // Color for the key
                glColor3f(1.0f, 0.84f, 0.0f);
                glPointSize(1);
                glBegin(GL_POINTS);
                glVertex2i(x, y);
                glEnd();
            }
        }
    }
}

// --- LOGIC ---

void updateMovement()
{
    float walkSpeed = 3.0f;
    float rotSpeed = 0.05f;
    float newX, newY;
    int moved = 0;
    int i;
    
    // Only update movement if the game is actively running 
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

    // Forward/Backward Movement
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

    // Collision check and update player position
    if(!checkCollision(newX, newY))
    {
        px = newX;
        py = newY;
    }
    
    // Check for key collection (Iterate through all required keys)
    for(i = 0; i < keysRequired; i++)
    {
        if(keySprites[i].active)
        {
            float distToKey = dist(px, py, keySprites[i].x, keySprites[i].y);
            if(distToKey < 20) // Proximity check
            {
                keySprites[i].active = 0;
                keysCollected++; // Increment collected count
            }
        }
    }

    // Check if the player passed the exit check after moving
    if(playerPassedExitCheck)
    {
        gameWon = 1; // Win screen
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
        // Background Color
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(1024, 0);
        glVertex2i(1024, 512);
        glVertex2i(0, 512);
        glEnd();
        
        drawRays3D();
        
        // Draw key sprites
        for(i = 0; i < keysRequired; i++)
        {
            // Using NULL as texture argument since sprite is drawn procedurally
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
            drawText("EXIT IS OPEN!", 150, 500, 0.0f, 1.0f, 0.0f); // (Unlocked)
        }
    }
    
    glutSwapBuffers();
}

void keyDown(unsigned char key, int x, int y)
{
    float keyX, keyY;
    int i; // Declared i here for C89 compliance

    if(gameWon)
    {
        // Reset game state on any key press after winning
        gameWon = 0;
        gameStarted = 0;
        gameIntro = 0;
        keysCollected = 0;
        playerPassedExitCheck = 0;
        
        // Reset player position and direction
        px = 1 * mapS + mapS/2;
        py = 1 * mapS + mapS/2;
        pa = 0;
        pdx = cosf(pa) * 5.0f;
        pdy = sinf(pa) * 5.0f;
        
        // Re-initialize key states randomly
        keysRequired = (rand() % MAX_KEYS) + 1; // Keys required

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
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

void resize(int w, int h)
{
    // Set up 2D coordinate system (0,0 to 1024,512)
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1024, 512, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void init()
{
    float keyX, keyY;
    int i; 
    
    glClearColor(0.3f, 0.3f, 0.3f, 0);
    // Initial ortho setup, will be called again by resize
    gluOrtho2D(0, 1024, 512, 0); 

    // Initial player position (center of tile 1,1)
    px = 1 * mapS + mapS/2;
    py = 1 * mapS + mapS/2;

    pa = 0; // Initial angle 0 (looking right)
    pdx = cosf(pa) * 5.0f;
    pdy = sinf(pa) * 5.0f;
    
    // Initial key setup
    keysRequired = (rand() % MAX_KEYS) + 1; // 1 to 5 keys required
    keysCollected = 0;
    
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
    
    playerPassedExitCheck = 0;
}

int main(int argc, char* argv[])
{
    // Random number generator
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
