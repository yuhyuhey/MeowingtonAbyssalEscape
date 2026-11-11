#include "SDL3/SDL.h"
#include "GameTypes.h"
#include "States.h"

void cleanup(SDLState& state);

int main(int argc, char* argv[])
{
    SDLState state{};
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

    Game game;
    //the first level has to be there, so check before anything starts
    if (!loadLevel(game.play.grid, game.play.currentLevel))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Couldn't open levels/level_1.txt", state.window);
        cleanup(state);
        return 1;
    }

    while (game.running)
    {
        SDL_Event event{ 0 };
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            {
                game.current->onQuit();
                game.running = false;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                //ignore held keys
                if (event.key.repeat) break;

                game.current->handleEvent(event);
                break;
            }
            }
        }
        game.current->update();

        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
        SDL_RenderClear(state.renderer);

        game.current->draw(state);

        SDL_RenderPresent(state.renderer);
    }


    cleanup(state);
    return 0;
}

void cleanup(SDLState &state)
{
    if (state.renderer) SDL_DestroyRenderer(state.renderer);
    if (state.window) SDL_DestroyWindow(state.window);
    SDL_Quit();
}