#pragma once
#include "DungeonMap.h"
#include "Player.h"
#include "EntityManager.h"
#include "Spawner.h"
#include "Portal.h"
#include "../InputSystem/InputSystem.h"
#include "UI.h"
#include <mutex>
#include <functional>
#include "SaveManager.h"
#include "../Utils/MessageSystem.h"

class Game
{
public:
    Game();
    ~Game();

    void Start();
    void Stop();

    bool IsGameOver() const { return _gameOver; }

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
    bool _gameOver;

    // ===== MÉTODOS DE INICIALIZACIÓN =====
    void CreateWorldRooms(Vector2 roomSize);
    bool LoadSavedGame();
    void StartNewGame();
    void SetupInputListeners();
    void InitializeCurrentRoom();

    // ===== MÉTODOS AUXILIARES PARA ENTIDADES =====
    void ActivateRoomEntities(Room* room);
    void DeactivateRoomEntities(Room* room);
    void PlacePlayerOnMap(Vector2 position);

    // ===== CALLBACKS =====
    std::function<Vector2()> GetPlayerPositionCallback();
    std::function<void(Enemy*)> GetEnemyAttackCallback();

    // ===== RENDERIZADO =====
    void DrawCurrentRoom();
    void RedrawPosition(Vector2 position);

    // ===== VALIDACIÓN Y MOVIMIENTO =====
    bool CanMoveTo(Vector2 position);
    bool IsPositionOccupied(Vector2 position);
    void MovePlayer(Vector2 direction);
    void MovePlayerTo(Vector2 newPosition);
    void UpdatePlayerOnMap();

    // ===== INTERACCIONES =====
    bool TryUsePortal(Vector2 newPosition);
    bool TryAttackInRange(Vector2 direction, int attackRange);
    bool TryAttackAtPosition(Vector2 position);
    void TryPickupItem(Vector2 position);

    // ===== GESTIÓN DE SALAS =====
    void ChangeRoom(PortalDir direction);
    PortalDir GetOppositeDirection(PortalDir dir);
    void StartRoomEnemyThreads(Room* room);

    // ===== CALLBACKS DE INPUT =====
    void OnMoveUp();
    void OnMoveDown();
    void OnMoveLeft();
    void OnMoveRight();

    // ===== GAME OVER =====
    void CheckPlayerDeath();
};