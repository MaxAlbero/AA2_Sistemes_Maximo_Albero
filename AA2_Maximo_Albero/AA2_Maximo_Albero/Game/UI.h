#pragma once
#include "../NodeMap/Vector2.h"
#include "Player.h"
#include "../Utils/ConsoleControl.h"
#include <iostream>
#include <thread>
#include <atomic>

#define TOP_UI_TEXT_AREA 0
#define BOTTOM_UI_TEXT_AREA 13

class UI
{
public:
    UI() : _player(nullptr), _uiThread(nullptr), _running(false) {}
    ~UI() { Stop(); }

    void Start(Player* player);
    void Stop();
    void SetMapSize(const Vector2& size) { _mapSize = size; }
    void DrawSidebar();

private:
    Player* _player;
    Vector2 _mapSize;
    std::thread* _uiThread;
    std::atomic<bool> _running;

    void UpdateLoop();
};