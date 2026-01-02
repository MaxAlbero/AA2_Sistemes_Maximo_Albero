#pragma once
#include "DungeonMap.h"
#include "Player.h"
#include "EntityManager.h"
#include "Spawner.h"
#include "Portal.h"
#include "../InputSystem/InputSystem.h"
#include <mutex>

class Game
{
public:
    Game();
    ~Game();

    void Start();
    void Stop();

private:
    DungeonMap* _dungeonMap;
    InputSystem* _inputSystem;
    EntityManager* _entityManager;
    Spawner* _spawner;
    Player* _player;
    Vector2 _playerPosition;

    int _currentRoomIndex;
    bool _running;
    std::mutex _gameMutex;

    void InitializeDungeon();
    void DrawCurrentRoom();

    bool CanMoveTo(Vector2 position);
    bool IsPositionOccupied(Vector2 position);
    void MovePlayer(Vector2 direction);
    void UpdatePlayerOnMap();

    void UpdateEnemyMovement();

    void ChangeRoom(PortalDir direction);
    PortalDir GetOppositeDirection(PortalDir dir);

    // Callbacks para input
    void OnMoveUp();
    void OnMoveDown();
    void OnMoveLeft();
    void OnMoveRight();
};