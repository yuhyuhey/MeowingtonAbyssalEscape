#include <iostream>
#include <string>
#include "States.h"

//draws text centred across the screen using sdl debug text, sdl docs mention this has limitations, but these don't apply to this use case.
void drawCentered(SDL_Renderer* renderer, float y, const std::string& text)
{
    //each letter 8 pixels wide
    float width = static_cast<float>(text.size()) * 8;
    SDL_RenderDebugText(renderer, (logSize - width) / 2, y, text.c_str());
}

void MenuState::handleEvent(const SDL_Event& event)
{
    //menu - enter starts a new game, escape quits
    if (event.key.key == SDLK_RETURN)
    {
        game.play.startNewGame();
        game.current = &game.play;
    }
    else if (event.key.key == SDLK_C && saveExists())
    {
        if (game.play.continueGame()) game.current = &game.play;
    }
    else if (event.key.key == SDLK_ESCAPE)
    {
        game.running = false;
    }
}

void MenuState::draw(SDLState& state)
{
    SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 255);
    drawCentered(state.renderer, 15, "MEOWINGTON'S");
    drawCentered(state.renderer, 25, "ABYSSAL ESCAPE");

    SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
    drawCentered(state.renderer, 50, "ENTER = NEW GAME");
    if (saveExists()) drawCentered(state.renderer, 60, "C = CONTINUE");
    drawCentered(state.renderer, 70, "ESC = QUIT");

    drawCentered(state.renderer, 90, "WASD = MOVE");
    drawCentered(state.renderer, 100, "Z = GRAB / PLACE");
    SDL_SetRenderDrawColor(state.renderer, 255, 200, 0, 255);
    drawCentered(state.renderer, 120, "R = RESET LEVEL &");
    drawCentered(state.renderer, 130, "FALL = -1 LIFE");
}

void PlayState::startNewGame()
{
    deleteSave();
    lives = maxLives;
    currentLevel = 1;
    loadLevel(grid, currentLevel);
    player = findSpawn(grid);
}

bool PlayState::continueGame()
{
    bool loaded = loadGame(currentLevel, lives, player, grid);
    deleteSave();
    //a save is used once, so you can't reload it to undo a mistake
    return loaded;
}

void PlayState::handleEvent(const SDL_Event& event)
{
    //playing, ignore keys pressed after a fall or a win
    if (!events.empty()) return;

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
    case SDLK_R: events.push(EventType::LevelReset); break;
    case SDLK_ESCAPE:
        saveGame(currentLevel, lives, player, grid);
        //leaving a game in progress saves it, then back to the menu
        game.current = &game.menu;
        break;
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
            if (grid[newY][newX] == BlockType::End) events.push(EventType::LevelComplete);

        }
        else
        {
            //off the edge / Empty tile = we fall
            std::cout << "Fell!" << std::endl;
            events.push(EventType::PlayerFell);
        }
    }
}
void PlayState::update()
{
    //a fall or R costs a life, then the level starts again
    EventType type;
    while (events.poll(type))
    {
        switch (type)
        {
             //a fall or R costs a life, then the level starts again
        case EventType::PlayerFell:
        case EventType::LevelReset:
            lives--;
            if (lives <= 0)
            {
                game.end.won = false;
                game.current = &game.end;
            }
            else
            {
                //every block goes back
                loadLevel(grid, currentLevel);
                //spawn has holdingBlock = false
                player = findSpawn(grid);   
            }
            break;

            //reaching the end loads the next level. Lives carry over
        case EventType::LevelComplete:
            currentLevel++;
            if (!loadLevel(grid, currentLevel))
            {
                game.end.won = true;
                game.current = &game.end;
            }
            else
            {
                player = findSpawn(grid);
            }
            break;
        }
    }
}

void PlayState::draw(SDLState& state)
{
    // draw every tile
    for (int y = 0; y < gridSize; y++)
    {
        for (int x = 0; x < gridSize; x++)
        {
            switch (grid[y][x])
            {
            case BlockType::Empty:  SDL_SetRenderDrawColor(state.renderer, 0, 0, 100, 255);   break;
            case BlockType::Active: SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255); break;
            case BlockType::Start:  SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255); break; // - UT
            case BlockType::End:    SDL_SetRenderDrawColor(state.renderer, 0, 255, 0, 255); break;
            }

            SDL_FRect tile =
            { static_cast<float>(x * tileSize),
              static_cast<float>(y * tileSize),
              tileSize,
              tileSize };
            SDL_RenderFillRect(state.renderer, &tile);
        }
    }
    //draw the player as an orange square with a red direction marker on the side it is facing = the direction you can pick/place tiles
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

    //the level number in the middle of the HUD - UT
    std::string levelText = "LV " + std::to_string(currentLevel);
    SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
    drawCentered(state.renderer, logSize + 0.5f, levelText);

    //block slot on the right of the HUD, an empty box that gets a white block in it while you are holding one
    SDL_FRect slot = { static_cast<float>(logSize - 11), static_cast<float>(logSize + 1), 7, 7 };
    SDL_SetRenderDrawColor(state.renderer, 120, 120, 120, 255);
    SDL_RenderRect(state.renderer, &slot);
    if (player.holdingBlock)
    {
        SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
        SDL_FRect held = { slot.x + 1, slot.y + 1, 5, 5 };
        SDL_RenderFillRect(state.renderer, &held);
    }
}

//closing the window in the middle of a game saves it
void PlayState::onQuit()
{
    saveGame(currentLevel, lives, player, grid);
}

void EndState::handleEvent(const SDL_Event& event)
{
    //game over and win screens, enter goes back to the menu
    if (event.key.key == SDLK_RETURN) game.current = &game.menu;
}

void EndState::draw(SDLState& state)
{
    if (!won)
    {
        SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 255);
        drawCentered(state.renderer, 50, "GAME OVER");
        SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
        drawCentered(state.renderer, 100, "ENTER = MENU");
    }
    else
    {
        SDL_SetRenderDrawColor(state.renderer, 0, 255, 0, 255);
        drawCentered(state.renderer, 50, "YOU MADE IT HOME!");
        SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
        drawCentered(state.renderer, 100, "ENTER = MENU");
    }
}