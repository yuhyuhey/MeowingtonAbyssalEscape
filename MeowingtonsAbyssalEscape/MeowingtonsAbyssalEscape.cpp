#include <iostream>
#include "SDL3/SDL.h"

struct SDLState
{
    SDL_Window* window;
    SDL_Renderer* renderer;
};

void cleanup(SDLState& state);

int main(int argc, char *argv[])
{
    SDLState state;
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL", nullptr);
        return 1;
    }

    int width = 600;
    int height = 400;
    state.window = SDL_CreateWindow("Meowington's Abyssal Escape", 600, 400, 0);
    if (!state.window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
        cleanup(state);
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(state.window, nullptr);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_Delay(10000);
    cleanup(state);
    return 0;
}

void cleanup(SDLState &state)
{
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}