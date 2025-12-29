#include "Game.h"
#include "Wall.h"
#include "../Utils/ConsoleControl.h"
#include <iostream>

Game::Game()
{
    _dungeonMap = new DungeonMap();
    _inputSystem = new InputSystem();
    _player = nullptr;
    _playerPosition = Vector2(1, 1);
    _currentRoomIndex = 0;
    _running = false;
}

Game::~Game()
{
    Stop();

    if (_player != nullptr)
        delete _player;

    delete _inputSystem;
    delete _dungeonMap;
}

void Game::InitializeDungeon()
{
    Room* room1 = new Room(Vector2(20, 10), Vector2(0, 0));
    _dungeonMap->AddRoom(room1);
    _dungeonMap->SetActiveRoom(0);

    _player = new Player(_playerPosition);

    UpdatePlayerOnMap();
}

void Game::Start()
{
    _gameMutex.lock();

    if (_running)
    {
        _gameMutex.unlock();
        return;
    }

    _running = true;

    InitializeDungeon();

    _inputSystem->AddListener(K_W, [this]() { this->OnMoveUp(); });
    _inputSystem->AddListener(K_S, [this]() { this->OnMoveDown(); });
    _inputSystem->AddListener(K_A, [this]() { this->OnMoveLeft(); });
    _inputSystem->AddListener(K_D, [this]() { this->OnMoveRight(); });

    _gameMutex.unlock();

    CC::Clear();
    DrawCurrentRoom();


    _inputSystem->StartListen();

    std::cout << "\n\nUsa WASD para moverte." << std::endl;
}

void Game::Stop()
{
    _gameMutex.lock();

    if (!_running)
    {
        _gameMutex.unlock();
        return;
    }

    _running = false;
    _inputSystem->StopListen();

    _gameMutex.unlock();
}

void Game::DrawCurrentRoom()
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom != nullptr)
    {
        currentRoom->Draw();
    }
}

bool Game::CanMoveTo(Vector2 position)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom == nullptr)
        return false;

    bool canMove = false;

    // Esto sirve para verificar que no sea una pared
    currentRoom->GetMap()->SafePickNode(position, [&](Node* node) {
        if (node != nullptr)
        {

            Wall* wall = node->GetContent<Wall>();
            canMove = (wall == nullptr);
        }
        });

    return canMove;
}

void Game::UpdatePlayerOnMap()
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom == nullptr)
        return;

    currentRoom->GetMap()->SafePickNode(_playerPosition, [&](Node* node) {
        if (node != nullptr)
        {
            node->SetContent(_player);
        }
        });
}

void Game::MovePlayer(Vector2 direction)
{
    _gameMutex.lock();

    if (!_running)
    {
        _gameMutex.unlock();
        return;
    }

    // Verificar si el jugador puede realizar una acción
    if (_player != nullptr && !_player->CanPerformAction())
    {
        _gameMutex.unlock();
        return; // No hacer nada si aún no ha pasado el cooldown
    }

    Vector2 newPosition = _playerPosition + direction;

    // Verificar si se puede mover a la nueva posición
    if (CanMoveTo(newPosition))
    {
        Room* currentRoom = _dungeonMap->GetActiveRoom();
        Vector2 oldPosition = _playerPosition;

        // Limpiar la casilla anterior (dejar vacía)
        currentRoom->GetMap()->SafePickNode(oldPosition, [](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(nullptr);
            }
            });

        // Actualizar posición
        _playerPosition = newPosition;

        // Colocar jugador en nueva posición
        UpdatePlayerOnMap();

        // Redibujar la casilla antigua (ahora vacía - se borrará)
        currentRoom->GetMap()->SafePickNode(oldPosition, [](Node* node) {
            if (node != nullptr)
            {
                node->DrawContent(Vector2(0, 0));
            }
            });

        // Redibujar la casilla nueva (ahora con jugador)
        currentRoom->GetMap()->SafePickNode(_playerPosition, [](Node* node) {
            if (node != nullptr)
            {
                node->DrawContent(Vector2(0, 0));
            }
            });

        // Actualizar el tiempo de la última acción del jugador
        if (_player != nullptr)
        {
            _player->UpdateActionTime();
        }
    }

    _gameMutex.unlock();
}

void Game::OnMoveUp()
{
    MovePlayer(Vector2(0, -1));
}

void Game::OnMoveDown()
{
    MovePlayer(Vector2(0, 1));
}

void Game::OnMoveLeft()
{
    MovePlayer(Vector2(-1, 0));
}

void Game::OnMoveRight()
{
    MovePlayer(Vector2(1, 0));
}