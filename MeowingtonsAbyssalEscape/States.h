#pragma once

#include <queue>
#include "SDL3/SDL.h"
#include "GameTypes.h"

struct SDLState
{
    SDL_Window* window;
    SDL_Renderer* renderer;
};

//things that happen in the game that something else needs to react to.they go into the queue when they happen, and PlayState::update reads them back out
enum class EventType
{
    PlayerFell,         //stepped onto an empty tile or off the edge
    LevelReset,         //pressed R
    LevelComplete       //stepped onto the end tile
};

struct EventQueue
{
    std::queue<EventType> events;

    void push(EventType type) { events.push(type); }
    bool empty() const { return events.empty(); }

    //takes the oldest event out of the queue. returns false when there are none left
    bool poll(EventType& type)
    {
        if (events.empty()) return false;
        type = events.front();
        events.pop();
        return true;
    }
};

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
    EventQueue events;

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

