#include <iostream>
#include <fstream>
#include <string>
#include "SDL3/SDL.h"


struct SDLState 
{
    SDL_Window* window;
    SDL_Renderer* renderer;
};

void cleanup(SDLState& state);

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

const int tileSize = 9;                          //one tile is 9 x 9 pixels
const int gridSize = 16;                         //16 x 16 tiles
const int logSize = tileSize * gridSize;         //144 x 144 pixels in total 
const int windowScale = 4;                       //window is 4x bigger = 576 x 576
const int maxLives = 3;                          //overall max lives (will discuss in ut)
const int hudHeight = tileSize;                  //a strip under the grid for lives
const int logHeight = logSize + hudHeight;       //144 x 153 pixels in total

//how much x and y change when facing each Direction (same order as the enum Up, Right, Down, Left)
const int dirX[4] = { 0, 1, 0, -1 };
const int dirY[4] = { -1, 0, 1, 0 };

bool loadLevel(BlockType grid[gridSize][gridSize], int level)
{
    std::ifstream file("levels/level_" + std::to_string(level) + ".txt");
    if (!file) return false;

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



int main(int argc, char *argv[])
{
    SDLState state;
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL", nullptr);
        return 1;
    }

    //int width = 600;
    //int height = 400;
    int windowW = logSize * windowScale;
    int windowH = logHeight * windowScale;
    state.window = SDL_CreateWindow("Meowington's Abyssal Escape", windowW, windowH, 0);
    if (!state.window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
        cleanup(state);
        return 1;
    }

    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", state.window);
        cleanup(state);
        return 1;
    }

    //configure the presentation
    //int logW = 640;
    //int logH = 320;
    SDL_SetRenderLogicalPresentation(state.renderer, logSize, logHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    int currentLevel = 1;
    //gridSize = 16 = 16 x 16 
    BlockType grid[gridSize][gridSize] = {};
    if (!loadLevel(grid, currentLevel))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Couldn't open levels/level_1.txt", state.window);
        cleanup(state);
        return 1;
    }

    Player player = findSpawn(grid);
    int lives = maxLives;
    bool resetLevel = false;
    bool levelComplete = false;
    bool running = true;
    while (running)
    {
        SDL_Event event{ 0 };
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                {
                    running = false;
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                {
                    //ignore held keys, and keys pressed after a fall
                    if (event.key.repeat || resetLevel || levelComplete) break;

                    //work out which way the key wants to go
                    int dx = 0;
                    int dy = 0;
                    Direction dir = player.facing;
                    switch (event.key.key)
                    {
                    case SDLK_W: case SDLK_UP:    dy = -1; dir = Direction::Up;    break;
                    case SDLK_D: case SDLK_RIGHT: dx = 1;  dir = Direction::Right; break;
                    case SDLK_S: case SDLK_DOWN:  dy = 1;  dir = Direction::Down;  break;
                    case SDLK_A: case SDLK_LEFT:  dx = -1; dir = Direction::Left;  break;
                    case SDLK_Z:
                    {
                        //the tile in front of the player
                        int frontX = player.x + dirX[static_cast<int>(player.facing)];
                        int frontY = player.y + dirY[static_cast<int>(player.facing)];
                        bool frontInside = frontX >= 0 && frontX < gridSize && frontY >= 0 && frontY < gridSize;

                        if (frontInside)
                        {
                            //pick up
                            BlockType front = grid[frontY][frontX];
                            if (!player.holdingBlock && (front == BlockType::Active || front == BlockType::Start))
                            {

                                grid[frontY][frontX] = BlockType::Empty;     
                                player.holdingBlock = true;
                            }
                            //place
                            else if (player.holdingBlock && front == BlockType::Empty)
                            {
                                grid[frontY][frontX] = BlockType::Active;    
                                player.holdingBlock = false;
                            }
                        }
                        break;
                    }
                    case SDLK_R: resetLevel = true; break;
                    }

                    if (dx != 0 || dy != 0)
                    {
                        int newX = player.x + dx;
                        int newY = player.y + dy;
                        bool insideGrid = newX >= 0 && newX < gridSize && newY >= 0 && newY < gridSize;

                        if (insideGrid && grid[newY][newX] != BlockType::Empty)
                        {
                            //move first
                            player.x = newX;         
                            player.y = newY;
                            player.facing = dir;
                            //then face the way we moved
                            if (grid[newY][newX] == BlockType::End) levelComplete = true;
                            
                        }
                        else
                        {
                            //off the edge / Empty tile = we fall
                            std::cout << "Fell!" << std::endl;
                            resetLevel = true;
                        }
                    }
                    break;
                }
            }
        }
        //a fall or R costs a life, then the level starts again
        if (resetLevel)
        {
            resetLevel = false;
            lives--;
            if (lives <= 0)
            {
                std::cout << "Game over!" << std::endl;
                lives = maxLives;               //for now we just start again. A menu comes later
                currentLevel = 1;
            }
            loadLevel(grid, currentLevel);      //every block goes back
            player = findSpawn(grid);           //spawn has holdingBlock = false 
        }

        //reaching the end loads the next level. Lives carry over
        if (levelComplete)
        {
            levelComplete = false;
            currentLevel++;
            if (!loadLevel(grid, currentLevel))     //no next file, so that was the last level, will test this
            {
                std::cout << "You made it home!" << std::endl;
                lives = maxLives;
                currentLevel = 1;
                loadLevel(grid, currentLevel);
            }
            player = findSpawn(grid);
        }

        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
        SDL_RenderClear(state.renderer);

        // draw every tile
        for (int y = 0; y < gridSize; y++)
        {
            for (int x = 0; x < gridSize; x++)
            {
                switch (grid[y][x])
                {
                case BlockType::Empty:  SDL_SetRenderDrawColor(state.renderer, 0, 0, 100, 255);   break;
                case BlockType::Active: SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255); break;
                //start is a different colour for debugging, but when game is finalised this to be changed to active
                case BlockType::Start:  SDL_SetRenderDrawColor(state.renderer, 0, 0,  0, 255);  break; 
                case BlockType::End:    SDL_SetRenderDrawColor(state.renderer, 0, 255, 0, 255); break;
                }

                SDL_FRect tile = 
                { static_cast<float>(x * tileSize), 
                  static_cast<float>(y * tileSize), 
                  tileSize, 
                  tileSize};
                SDL_RenderFillRect(state.renderer, &tile);
            }
        }
        //draw the player as an orange square with a red direction marker on the side it is facing - to visual update later
        float px = static_cast<float>(player.x * tileSize);
        float py = static_cast<float>(player.y * tileSize);

        SDL_SetRenderDrawColor(state.renderer, 255, 140, 0, 255);
        SDL_FRect body = { px + 1, py + 1, 7, 7 }; 
        SDL_RenderFillRect(state.renderer, &body);

        //start in the middle of the tile
        float markX = px + 3;      
        float markY = py + 3;
        //then slide the direction marker out to the facing edge
        switch (player.facing)      
        {
        case Direction::Up:    markY -= 3; break;
        case Direction::Down:  markY += 3; break;
        case Direction::Left:  markX -= 3; break;
        case Direction::Right: markX += 3; break;
        }
        SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 255);
        SDL_FRect marker = { markX, markY, 3, 3 };
        SDL_RenderFillRect(state.renderer, &marker);

        //a small white block on the player to show its held, will add this to hud? to be mentioned in user feedback!
        if (player.holdingBlock)    
        {
            SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
            SDL_FRect held = { px + 3, py + 3, 3, 3 };
            SDL_RenderFillRect(state.renderer, &held);
        }

        // HUD DRAWING AND COLOURING!!
        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
        SDL_FRect hud = { 0, static_cast<float>(logSize), static_cast<float>(logSize), static_cast<float>(hudHeight) };
        SDL_RenderFillRect(state.renderer, &hud);

        SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 255);
        for (int i = 0; i < lives; i++)
        {
            SDL_FRect heart = { static_cast<float>(3 + i * 8), static_cast<float>(logSize + 2), 5, 5 };
            SDL_RenderFillRect(state.renderer, &heart);
        }

        SDL_RenderPresent(state.renderer);
    }


    cleanup(state);
    return 0;
}

void cleanup(SDLState &state)
{
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}