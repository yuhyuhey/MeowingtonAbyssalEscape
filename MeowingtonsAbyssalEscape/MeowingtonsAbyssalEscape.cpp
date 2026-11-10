#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
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

//draws text centred across the screen using sdl debug text, docs mention this has limitations, but they are look fine for now. if deploying later on i'll use sdl ttf for example.
void drawCentered(SDL_Renderer* renderer, float y, const std::string& text)
{   
    //each letter 8 pixels wide
    float width = static_cast<float>(text.size()) * 8;
    SDL_RenderDebugText(renderer, (logSize - width) / 2, y, text.c_str());
}

//what the game is doing right now, each state handles its own keys, updates and draws itself,
//and the loop in main just passes everything on to whichever state is current
struct Game;

struct State
{
    virtual ~State() {}
    virtual void handleEvent(const SDL_Event& event) = 0;
    virtual void update() {}
    virtual void draw(SDLState& state) = 0;
    virtual void onQuit() {}
};

//the title screen
struct MenuState : State
{
    Game& game;
    MenuState(Game& g) : game(g) {}

    void handleEvent(const SDL_Event& event) override;
    void draw(SDLState& state) override;
};

//a game in progress. these used to be loose variables in main
struct PlayState : State
{
    Game& game;
    int currentLevel = 1;
    //gridSize = 16 = 16 x 16 
    BlockType grid[gridSize][gridSize] = {};
    Player player = { 0, 0, Direction::Right, false };
    int lives = maxLives;
    bool resetLevel = false;
    bool levelComplete = false;

    PlayState(Game& g) : game(g) {}

    void startNewGame();
    bool continueGame();
    void handleEvent(const SDL_Event& event) override;
    void update() override;
    void draw(SDLState& state) override;
    void onQuit() override;
};

//the game over and win screens only differ in the text
struct EndState : State
{
    Game& game;
    bool won = false;
    EndState(Game& g) : game(g) {}

    void handleEvent(const SDL_Event& event) override;
    void draw(SDLState& state) override;
};

//holds the three states and which one is current. they are just members, so nothing needs new or delete
struct Game
{
    MenuState menu;
    PlayState play;
    EndState end;
    State* current;
    bool running = true;

    Game() : menu(*this), play(*this), end(*this), current(&menu) {}
};

void MenuState::handleEvent(const SDL_Event& event)
{
    //menu, enter starts a new game, escape quits - text/indication add later/user feedback
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
    if (resetLevel || levelComplete) return;

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
            if (grid[newY][newX] == BlockType::End) levelComplete = true;

        }
        else
        {
            //off the edge / Empty tile = we fall
            std::cout << "Fell!" << std::endl;
            resetLevel = true;
        }
    }
}
void PlayState::update()
{
    //a fall or R costs a life, then the level starts again
    if (resetLevel)
    {
        resetLevel = false;
        lives--;
        if (lives <= 0)
        {
            game.end.won = false;
            game.current = &game.end;
        }
        else
        {
            loadLevel(grid, currentLevel);      //every block goes back
            player = findSpawn(grid);           //spawn has holdingBlock = false 
        }
    }

    //reaching the end loads the next level. Lives carry over
    if (levelComplete)
    {
        levelComplete = false;
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

    /*a small white block on the player to show its held, will add this to hud ? to be mentioned in user feedback! - Moved to hud
    if (player.holdingBlock)
    {
        SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
        SDL_FRect held = { px + 3, py + 3, 3, 3 };
        SDL_RenderFillRect(state.renderer, &held);
    }*/

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

    //block slot on the right of the HUD, an empty box that gets a white block in it while you are holding one - moved from character - UT
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