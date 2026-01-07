#pragma once
#include "DungeonMap.h"
#include "Player.h"
#include "EntityManager.h"
#include "Spawner.h"
#include "Portal.h"
#include "../InputSystem/InputSystem.h"
#include "UI.h"
#include <mutex>
#include "SaveManager.h"
#include "../Utils/MessageSystem.h"

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
    UI* _ui;
    Player* _player;
    Vector2 _playerPosition;

    SaveManager* _saveManager;
    MessageSystem* _messages;

    int _currentRoomIndex;
    bool _running;
    std::mutex _gameMutex;

    void InitializeDungeon();
    void DrawCurrentRoom();

    bool LoadSavedGame();  // Nuevo método

    bool CanMoveTo(Vector2 position);
    bool IsPositionOccupied(Vector2 position);
    void MovePlayer(Vector2 direction);
    void UpdatePlayerOnMap();

    void UpdateEnemyMovement();

    void ChangeRoom(PortalDir direction);
    PortalDir GetOppositeDirection(PortalDir dir);

    void InitializeCurrentRoom();

    // Callbacks para input
    void OnMoveUp();
    void OnMoveDown();
    void OnMoveLeft();
    void OnMoveRight();
};