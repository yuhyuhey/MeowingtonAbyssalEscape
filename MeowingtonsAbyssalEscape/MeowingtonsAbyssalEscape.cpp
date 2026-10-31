#include <iostream>
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
};

const int tileSize = 9;                          //one tile is 9 x 9 pixels
const int gridSize = 16;                         //16 x 16 tiles
const int logSize = tileSize * gridSize;         //144 x 144 pixels in total 
const int windowScale = 4;                       //window is 4x bigger = 576 x 576



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
    int windowSize = logSize * windowScale;
    state.window = SDL_CreateWindow("Meowington's Abyssal Escape", windowSize, windowSize, 0);
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
    SDL_SetRenderLogicalPresentation(state.renderer, logSize, logSize, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    BlockType grid[gridSize][gridSize] = {};       //gridSize = 16 = 16 x 16 
    grid[8][1] = BlockType::Start;
    grid[8][2] = BlockType::Active;
    grid[8][3] = BlockType::Active;
    grid[9][3] = BlockType::Active;
    grid[8][7] = BlockType::End;

    //the player spawns on the start tile
    Player player = { 0, 0, Direction::Right };
    for (int y = 0; y < gridSize; y++)
    {
        for (int x = 0; x < gridSize; x++)
        {
            if (grid[y][x] == BlockType::Start)
            {
                player.x = x;
                player.y = y;
            }
        }
    }

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
                    //ignore the key being held down
                    if (event.key.repeat) break;     

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
                    }

                    if (dx != 0 || dy != 0)
                    {
                        int newX = player.x + dx;
                        int newY = player.y + dy;
                        bool insideGrid = newX >= 0 && newX < gridSize && newY >= 0 && newY < gridSize;

                        //for now moving onto an empty tile or off the edge does nothing
                        if (insideGrid && grid[newY][newX] != BlockType::Empty)
                        {
                            //move first
                            player.x = newX;         
                            player.y = newY;
                            player.facing = dir;     
                            //then face the way we moved
                        }
                    }
                    break;
                }
            }
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