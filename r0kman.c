/*
 * r0kman.c
 * 
 * Copyright 2012 ManiacTwister <ManiacTwister@s7t.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include <sysinit.h>
#include <stdio.h>

#include "basic/basic.h"
#include "basic/config.h"

#include "lcd/render.h"
#include "lcd/display.h"
#include "lcd/allfonts.h"
#include "filesystem/ff.h"
#include "filesystem/diskio.h"

#include "usetable.h"

uint8_t levels[1][10][16] = {
  {
    {0,4,4,4,6,4,4,4,4,4,4,6,4,4,4,2},
    {5,12,12,12,5,12,12,12,12,12,12,5,12,12,12,5},
    {5,13,8,12,9,12,0,30,30,2,12,9,12,8,13,5},
    {5,12,9,12,12,12,5,31,31,5,12,12,12,9,12,5},
    {5,12,12,12,8,12,1,4,4,3,12,8,12,12,12,5},
    {5,12,0,4,3,12,12,31,31,12,12,1,4,2,12,5},
    {5,12,5,12,12,12,0,4,4,2,12,12,12,5,12,5},
    {5,12,9,12,11,4,7,4,4,7,4,10,12,9,12,5},
    {5,12,13,12,12,12,12,12,12,12,12,12,12,13,12,5},
    {1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,3},
  }
};

uint8_t tiles[32][6] = {
  { 0, 7, 8, 16, 17, 18 },
  { 18, 17, 16, 8, 7, 0 },
  { 0, 56, 4, 2, 34, 18 },
  { 18, 34, 2, 4, 56, 0 },
  { 0, 63, 0, 0, 63, 0 },
  { 18, 18, 18, 18, 18, 18 },
  { 0, 63, 0, 0, 33, 18 },
  { 18, 33, 0, 0, 63, 0 },
  { 0, 12, 18, 18, 18, 18 },
  { 18, 18, 18, 18, 12, 0 },
  { 0, 60, 2, 2, 60, 0 },
  { 0, 15, 16, 16, 15, 0 },
  { 0, 0, 12, 12, 0, 0 },
  { 0, 12, 30, 30, 12, 0 },  
  { 12, 30, 15, 15, 30, 12 },
  { 28, 14, 7, 7, 14, 28 },
  { 0, 18, 63, 63, 30, 12 },
  { 0, 33, 51, 63, 30, 12 },
  { 12, 30, 60, 60, 30, 12 },
  { 14, 30, 56, 56, 28, 14 },
  { 12, 30, 63, 63, 18, 0 },
  { 12, 30, 63, 51, 33, 0 },
  { 12, 30, 63, 63, 30, 12 },
  { 0, 0, 33, 63, 30, 12 },
  { 0, 0, 0, 33, 30, 12 }, 
  { 0, 0, 0, 0, 30, 12 },
  { 0, 0, 0, 0, 12, 12 },
  { 0, 17, 10, 0, 10, 17 },
  { 12, 30, 43, 63, 63, 42 },
  { 12, 30, 53, 63, 63, 21 },
  { 0, 0, 63, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0 }
};

int fqz=50;
uint8_t dir=0;
uint8_t pacX;
uint8_t pacY;
uint8_t pacDX = 3;
uint8_t pacDY = 0;
uint16_t points = 0;
uint8_t pacFrame = 0;
int8_t pacFramed = 1;
uint8_t lives = 3;

uint8_t ghosts[4][9] = {
 /* x, y, dx, dy, state, mode, dir, targetX, targetY */ //TODO: timer for state 1
  { 7, 3, 0, 0, 0, 0, 0, 7, 5 }, // Blinky
  { 7, 1, 0, 0, 0, 0, 0, 7, 5 }, // Pinky
  { 8, 3, 0, 0, 0, 0, 0, 7, 5 }, // Inky
  { 8, 1, 0, 0, 0, 0, 0, 7, 5 }  // Clyde
}; 

bool color;
bool finished=false;
bool isStart=true;
bool run = true;


void renderLevel(void);
void renderPacman(void);
bool canStep(uint8_t,uint8_t,int);
void checkForPoint();
void drawTile(uint8_t,uint8_t,uint8_t);
void drawSpecialTile(uint8_t,uint8_t,uint8_t, uint8_t, uint8_t);
void drawMeta(void);
void renderGhosts(void);
void checkForGhost(void);
void resetValues(void);
bool pointsLeft(void);
void setStartscreenValues(void);
void startScreen(void); 
void makeStep(int);
void drawEndscreen(void);
void metaText(void);

void ram(void) {
  /* Color? */
  /*if(lcdRead(220)==14) {
    color=true;
  } else {
    color=false;
  }*/
    color=false;
        
  if(color) {
    return;
  }

  #ifdef SIMULATOR
    fqz = 650;
  #endif

  
  startScreen();
  
  resetValues();
  while(run) {
    uint8_t key = getInputRaw();
    if(!finished) {
      switch(key) {
        case BTN_LEFT:
          if(canStep(pacX-1, pacY, 5) && pacDY == 0)
            dir = 0;
          break;
        case BTN_UP:
          if(canStep(pacX, pacY-1, 5) && pacDX == 0)
            dir = 1;
          break;
        case BTN_RIGHT:
          if(canStep(pacX+1, pacY, 5) && pacDY == 0)
            dir = 2;
          break;
        case BTN_DOWN:
          if(canStep(pacX, pacY+1, 5) && pacDX == 0)
            dir = 3;
          break;
        case BTN_ENTER:
          run = false;
      }
      
      renderLevel();
      renderPacman();
      checkForPoint();
      renderGhosts();
      checkForGhost();
      drawMeta();

      if(!pointsLeft()) { 
        finished = true;
      }
    } else {
      if (key==BTN_DOWN) {
        run=false;
      }
      lcdClear();
      drawEndscreen();
    }
    lcdRefresh();
    delayms(fqz);
  }
  return;
}

void renderLevel() {
  lcdClear();
  for (uint8_t y=0; y < 10; y++) {
    for(uint8_t x=0; x < 16; x++) {
      drawTile(levels[0][y][x], x, y);
    }
  }
}

void renderPacman() {
  makeStep(5);
 
  // Render (r0k|pac)man
  if(pacFrame == 0) {
    drawSpecialTile(22, pacX, pacY, pacDX, pacDY);
    pacFramed = 1;
  } 
  else {
    drawSpecialTile(13 + dir*2 + pacFrame, pacX, pacY, pacDX, pacDY);
  }
  if(pacFrame == 2) {
    pacFramed = -1;
  }
  pacFrame += pacFramed;
}

bool canStep(uint8_t x, uint8_t y, int g) {
  return (levels[0][y][x]==31 || levels[0][y][x]==12 || levels[0][y][x]==13 || (g < 5 && ghosts[g][6]==1 && levels[0][y][x]==30));
}

void checkForPoint() {
  if(levels[0][pacY][pacX]==12) {
    points += 10;
    levels[0][pacY][pacX]=31;
  } else if(levels[0][pacY][pacX]==13) {
    points += 50;
    levels[0][pacY][pacX]=31;
    for(int g=0; g<4; g++) {
      //ghosts[g][4]=1; //state 1 = catchable
    }
  }
}

void checkForGhost() {
  for(int g=0; g<4; g++) {
    if(ghosts[g][0]==pacX && ghosts[g][1]==pacY && ghosts[g][4]==0) {
      lives--;
      if(lives>0) {
        pacX = 7;
        pacY = 5;
        pacDX = 3;
        pacDY = 0;
        dir=0;
      } else {
        finished=true;
      }
    }
  }
}

void renderGhosts() {
  // Set target
  /*
  for(i=0; i < 4; i++) {
    switch(i) {
      case 0:
        //blinky - target == r0kman
        ghosts[0][7]=pacX;
        ghosts[0][8]=pacY;
        break;
      case 1:
        //pinky
        break;
      case 2:
        //inky
        break;
      case 3:
        //clyde
        break;
    }
  }*/
  
  
  for(int g=0; g<4; g++) {
    makeStep(g);
    drawSpecialTile(28, ghosts[g][0], ghosts[g][1], ghosts[g][2], ghosts[g][3]);
  }
}

void makeStep(int pac) {
  uint8_t s_dir;
  uint8_t s_x;
  uint8_t s_dx;
  uint8_t s_y;
  uint8_t s_dy;
  
  if(pac==5) {
    s_dir = dir;
    s_x = pacX;
    s_dx = pacDX;
    s_y = pacY;
    s_dy = pacDY;
  } else {
    s_dir = ghosts[pac][6];
    s_x = ghosts[pac][0];
    s_dx = ghosts[pac][2];
    s_y = ghosts[pac][1];
    s_dy = ghosts[pac][3];
  }
  
  switch (s_dir) {
    // If necessary, optimize ;)
    case 0:
      if(canStep(s_x - (6 - s_dx) / 6, s_y, pac) || isStart) {
        if(s_dx == 0) {
          s_dx = 6;
          s_x--;
        }
        s_dx--;
      } else if(pac < 5){
        s_dir = getRandom() % 4;
      }
      break;
    case 1:
      if(canStep(s_x, s_y - (6 - s_dy) / 6, pac) || isStart) {
        if(s_dy == 0) {
          s_dy = 6;
          s_y--;
        }
        s_dy--;
      } else if(pac < 5){
        s_dir = getRandom() % 4;
      }
      break;
    case 2:
      if(canStep(s_x + (s_dx + 6) / 6, s_y, pac) || isStart) {
        s_dx++;
        if(s_dx == 6) {
          s_dx = 0;
          s_x++;
        }
      } else if(pac < 5){
        s_dir = getRandom() % 4;
      }
      break;
    case 3:
      if(canStep(s_x, s_y + (s_dy + 6) / 6, pac) || isStart) {
        s_dy++;
        if(s_dy == 6) {
          s_dy = 0;
          s_y++;
        }
      } else if(pac < 5){
        s_dir = getRandom() % 4;
      }
      break;
  }
   if(pac==5) {
    s_dir = dir;
    pacX = s_x;
    pacDX = s_dx;
    pacY = s_y;
    pacDY = s_dy;
  } else {
    ghosts[pac][6] = s_dir;
    ghosts[pac][0] = s_x;
    ghosts[pac][2] = s_dx;
    ghosts[pac][1] = s_y;
    ghosts[pac][3] = s_dy;
  }
}

void drawTile(uint8_t tile, uint8_t posX, uint8_t posY) {
  drawSpecialTile(tile, posX, posY, 0, 0);
}

void drawSpecialTile(uint8_t tile, uint8_t posX, uint8_t posY, uint8_t posDX, uint8_t posDY) {
  for(uint8_t y = 0; y < 6; y++) {
   uint8_t row = tiles[tile][y];
   for(uint8_t x = 0; x < 6; x++) {
    if( (row >> x) % 2 )
      lcdSetPixel(5-x+posX*6+posDX,y+posY*6+posDY,1);
   }
  }
}

void drawMeta() {
  uint8_t dx = DoString(5, 60, "Score: ");
  DoInt(dx, 60, points);
  
  for(uint8_t i=0; i<lives; i++) {
    drawSpecialTile(15, 13 + i, 10, 0, 1);
  }
}

bool pointsLeft() {
  for (uint8_t y=0; y < 10; y++) {
    for(uint8_t x=0; x < 16; x++) {
      if(levels[0][y][x] == 12 || levels[0][y][x] == 13) {
        return true;
      }
    }
  }
  return false;
}

void drawEndscreen() {
  metaText();
  DoInt(33, 34, points);
  DoString(0,25, "   Score:");
}

void metaText() {
  DoString(0, 2,"  r0kman by  ");
  DoString(0, 10,"ManiacTwister");
  
  if(isStart) DoString(0, 40,"Up:Insert coin");
  DoString(0,48, "  Down: Exit  "); 
}

/*
 * Just a first test - need optimization!*/
void startScreen() {
  while(isStart) {

    setStartscreenValues();
    
    while(pacX > 0) {
      if(getInputRaw()==BTN_UP) isStart=false;
      else if(getInputRaw()==BTN_DOWN){
        isStart=false;
        run=false;
      }
      lcdClear();
      metaText();
      renderPacman();
      renderGhosts();
      lcdRefresh();
      delayms(fqz);
      if(!isStart) break;
    }
    /*dir=2;
    ghosts[0][6]=2;
    ghosts[1][6]=2;
    ghosts[2][6]=2;
    ghosts[3][6]=2;
    while(pacX < 11) {
      if(getInputRaw()==BTN_UP) isStart=false;
      else if(getInputRaw()==BTN_DOWN){
        isStart=false;
        run=false;
      }
      lcdClear();
      metaText();
      renderPacman();
      renderGhosts();
      lcdRefresh();
      delayms(50);
      if(!isStart) break;
    }*/
  }
  resetValues();
}

void setStartscreenValues() {
  pacX = 11;
  pacY = 5;
  dir=0;
  ghosts[0][0] = 13;
  ghosts[1][0] = 14;
  ghosts[2][0] = 15;
  ghosts[3][0] = 16;
  ghosts[0][1] = 5;
  ghosts[1][1] = 5;
  ghosts[2][1] = 5;
  ghosts[3][1] = 5;
  ghosts[0][2] = 3;
  ghosts[1][2] = 3;
  ghosts[2][2] = 3;
  ghosts[3][2] = 3;
  ghosts[0][3] = 0;
  ghosts[1][3] = 0;
  ghosts[2][3] = 0;
  ghosts[3][3] = 0;
  ghosts[0][6]=0;
  ghosts[1][6]=0;
  ghosts[2][6]=0;
  ghosts[3][6]=0;
}

void resetValues() {

  /*const uint8_t tmplevels[1][10][16] = {
    {
      {0,4,4,4,6,4,4,4,4,4,4,6,4,4,4,2},
      {5,12,12,12,5,12,12,12,12,12,12,5,12,12,12,5},
      {5,13,8,12,9,12,0,30,30,2,12,9,12,8,13,5},
      {5,12,9,12,12,12,5,31,31,5,12,12,12,9,12,5},
      {5,12,12,12,8,12,1,4,4,3,12,8,12,12,12,5},
      {5,12,0,4,3,12,12,31,31,12,12,1,4,2,12,5},
      {5,12,5,12,12,12,0,4,4,2,12,12,12,5,12,5},
      {5,12,9,12,11,4,7,4,4,7,4,10,12,9,12,5},
      {5,12,13,12,12,12,12,12,12,12,12,12,12,13,12,5},
      {1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,3},
    }
  };
  memcpy(levels,tmplevels,sizeof(tmplevels));*/

  memset(ghosts, 0, sizeof(ghosts[0][0]) * 4 * 9);
  ghosts[0][0] = 7;
  ghosts[1][0] = 7;
  ghosts[2][0] = 8;
  ghosts[3][0] = 8;

  ghosts[0][1] = 3;
  ghosts[1][1] = 1;
  ghosts[2][1] = 3;
  ghosts[3][1] = 1;

  ghosts[0][7] = 7;
  ghosts[1][7] = 7;
  ghosts[2][7] = 7;
  ghosts[3][7] = 7;

  ghosts[0][8] = 5;
  ghosts[1][8] = 5;
  ghosts[2][8] = 5;
  ghosts[3][8] = 5;

  dir=0;
  pacX = 7;
  pacY = 5;
  pacDX = 3;
  pacDY = 0;
  points = 0;
  pacFrame = 0;
  pacFramed = 1;
  lives = 3;

  finished=false;
  isStart=false;
}
