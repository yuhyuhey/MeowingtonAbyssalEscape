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
        return 1;
    }

    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", state.window);
        cleanup(state);
        return 1;
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

            }
        }
        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
        SDL_RenderClear(state.renderer);
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