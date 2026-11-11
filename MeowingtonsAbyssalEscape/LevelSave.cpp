#include <fstream>
#include <string>
#include <cstdio>
#include "GameTypes.h"

//reads 16 lines of tiles from a file into the grid. Used by levels 
void readGrid(std::istream& file, BlockType grid[gridSize][gridSize])
{
    //start with everything empty
    for (int y = 0; y < gridSize; y++)
    {
        for (int x = 0; x < gridSize; x++)
        {
            grid[y][x] = BlockType::Empty;
        }
    }

    //then read one line of the file per row of the grid
    std::string line;
    for (int y = 0; y < gridSize && std::getline(file, line); y++)
    {
        for (int x = 0; x < gridSize && x < static_cast<int>(line.size()); x++)
        {
            switch (line[x])
            {
            case '#': grid[y][x] = BlockType::Active; break;
            case 'S': grid[y][x] = BlockType::Start;  break;
            case 'E': grid[y][x] = BlockType::End;    break;
            }
        }
    }
}

bool loadLevel(BlockType grid[gridSize][gridSize], int level)
{
    std::ifstream file("levels/level_" + std::to_string(level) + ".txt");
    if (!file) return false;

    readGrid(file, grid);
    return true;
}

//the player spawns on the start tile
Player findSpawn(BlockType grid[gridSize][gridSize])
{
    Player spawn = { 0, 0, Direction::Right, false };
    for (int y = 0; y < gridSize; y++)
    {
        for (int x = 0; x < gridSize; x++)
        {
            if (grid[y][x] == BlockType::Start)
            {
                spawn.x = x;
                spawn.y = y;
            }
        }
    }
    return spawn;
}

const std::string saveFile = "savegame.txt";
//saves the game in progress, level, lives, the player and every tile.
void saveGame(int level, int lives, const Player& player, BlockType grid[gridSize][gridSize])
{
    std::ofstream file(saveFile);
    if (!file) return;

    file << level << " " << lives << " " << player.x << " " << player.y << " "
        << static_cast<int>(player.facing) << " " << player.holdingBlock << "\n";

    for (int y = 0; y < gridSize; y++)
    {
        for (int x = 0; x < gridSize; x++)
        {
            char c = '.';
            switch (grid[y][x])
            {
            case BlockType::Empty:  c = '.'; break;
            case BlockType::Active: c = '#'; break;
            case BlockType::Start:  c = 'S'; break;
            case BlockType::End:    c = 'E'; break;
            }
            file << c;
        }
        file << "\n";
    }
}

//loads the save file. Returns false if there isn't one or invalid
//first line of the save file = level lives playerX playerY facing holding then the tiles underneath
bool loadGame(int& level, int& lives, Player& player, BlockType grid[gridSize][gridSize])
{
    std::ifstream file(saveFile);
    if (!file) return false;

    int l, h, px, py, f, held;
    if (!(file >> l >> h >> px >> py >> f >> held)) return false;

    //error handling no silly numbers
    if (l < 1 || h < 1 || h > maxLives) return false;
    if (px < 0 || px >= gridSize || py < 0 || py >= gridSize) return false;
    if (f < 0 || f > 3) return false;

    std::string restOfLine;
    //skip the end of the first line
    std::getline(file, restOfLine);
    readGrid(file, grid);

    level = l;
    lives = h;
    player.x = px;
    player.y = py;
    player.facing = static_cast<Direction>(f);
    player.holdingBlock = (held != 0);
    return true;
}

bool saveExists()
{
    std::ifstream file(saveFile);
    return file.good();
}

void deleteSave()
{
    std::remove(saveFile.c_str());
}