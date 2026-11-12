#pragma once

enum class BlockType
{
    Empty,
    Active,
    Start,
    End
};

enum class Direction
{
    Up,
    Right,
    Down,
    Left
};


struct Player
{
    int x;
    int y;
    Direction facing;
    bool holdingBlock;
};

//one tile is 9 x 9 pixels
const int tileSize = 9;

//16 x 16 tiles
const int gridSize = 16;

//144 x 144 pixels in total 
const int logSize = tileSize * gridSize;

//window is 4x bigger = 576 x 576
const int windowScale = 4;

//overall max lives (will discuss in ut)
const int maxLives = 3;

//a strip under the grid for lives
const int hudHeight = tileSize;

//144 x 153 pixels in total
const int logHeight = logSize + hudHeight;

//how much x and y change when facing each Direction (same order as the enum Up, Right, Down, Left)
const int dirX[4] = { 0, 1, 0, -1 };
const int dirY[4] = { -1, 0, 1, 0 };

//level and save functions, written in LevelSave.cpp
bool loadLevel(BlockType grid[gridSize][gridSize], int level);
Player findSpawn(BlockType grid[gridSize][gridSize]);
void saveGame(int level, int lives, const Player& player, BlockType grid[gridSize][gridSize]);
bool loadGame(int& level, int& lives, Player& player, BlockType grid[gridSize][gridSize]);
bool saveExists();
void deleteSave();