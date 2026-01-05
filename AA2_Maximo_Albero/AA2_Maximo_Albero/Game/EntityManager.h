#pragma once
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include "../NodeMap/Vector2.h"
#include "Enemy.h"
#include "Chest.h"
#include "Item.h"
#include "Player.h"
#include "Portal.h"

#include "Room.h"
#include "Wall.h"


class EntityManager
{
private:
    std::vector<Enemy*> _enemies;
    std::vector<Chest*> _chests;
    std::vector<Item*> _items;

    std::mutex _managerMutex;

    // Thread para gestionar movimiento de enemigos
    std::thread* _enemyMovementThread;
    std::atomic<bool> _movementActive;

    Room* _currentRoom;
    std::function<Vector2()> _getPlayerPositionCallback;
    std::function<void(Enemy*)> _onEnemyAttackPlayer;

public:
    EntityManager() : _enemyMovementThread(nullptr), _movementActive(false) {}

    ~EntityManager()
    {
        StopEnemyMovement();
        ClearAllEntities();
    }

    void SetCurrentRoom(Room* room);

    // Gestión de enemigos
    void SpawnEnemy(Vector2 position, Room* room);

    void SpawnChest(Vector2 position, Room* room);

    void CleanupDeadEnemies(Room* room);

    bool IsPositionOccupiedByEnemy(Vector2 position);

    Enemy* GetEnemyAtPosition(Vector2 position, Room* room);

    //Methods to get entities
    std::vector<Enemy*> GetEnemies();

    std::vector<Chest*> GetChests();

    std::vector<Item*> GetItems();

    void ClearAllEntities();

    int GetEnemyCount();

    // Sistema de movimiento de enemigos
    void StartEnemyMovement(Room* room, std::function<Vector2()> getPlayerPositionCallback, std::function<void(Enemy*)> onEnemyAttackPlayer);

    void StopEnemyMovement();
    bool IsPositionOccupiedByChest(Vector2 position);
    Chest* GetChestAtPosition(Vector2 position, Room* room);

    void CleanupBrokenChests(Room* room);

    int GetChestCount();

    void SpawnItem(Vector2 position, ItemType type, Room* room);

    Item* GetItemAtPosition(Vector2 position, Room* room);
    
    //if something is going to give problems, probably it's going to be this
    void RemoveItem(Item* itemToRemove, Room* room);

    bool IsPositionOccupiedByItem(Vector2 position);

    ItemType SelectLoot();

    void DropLoot(Vector2 position, ItemType lootItem, Room* room);

    void InitializeRoomEntities(Room* room, int roomX, int roomY);

    bool TryAttackEnemyAt(Vector2 position, Player* attacker, Room* room);

    bool TryAttackChestAt(Vector2 position, Player* attacker, Room* room);

    void RegisterLoadedEntities(Room* room);

    void Lock() { _managerMutex.lock(); }
    void Unlock() { _managerMutex.unlock(); }

private:
    bool IsAdjacent(Vector2 pos1, Vector2 pos2);

    void EnemyMovementLoop();

    bool CanEnemyMoveTo(Vector2 newPosition, Vector2 currentPosition, Room* room, Vector2 playerPosition);

    Vector2 FindValidSpawnPosition(Room* room);
    


};