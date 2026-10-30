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