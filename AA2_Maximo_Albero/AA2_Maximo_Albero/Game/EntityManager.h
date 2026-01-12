#pragma once
#include <vector>
#include <mutex>
#include <functional>
#include <algorithm>
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

    Room* _currentRoom;
    std::function<Vector2()> _getPlayerPositionCallback;
    std::function<void(Enemy*)> _onEnemyAttackPlayer;

public:
    EntityManager() : _currentRoom(nullptr) {}

    ~EntityManager()
    {
        ClearAllEntities();
    }

    void SetCurrentRoom(Room* room);

    // ===== ENTITY MANAGEMENT =====
    void SpawnEnemy(Vector2 position, Room* room);
    void SpawnChest(Vector2 position, Room* room);
    void SpawnItem(Vector2 position, ItemType type, Room* room);

    void CleanupDeadEnemies(Room* room);
    void CleanupBrokenChests(Room* room);

    // Specific getters
    Enemy* GetEnemyAtPosition(Vector2 position, Room* room);
    Chest* GetChestAtPosition(Vector2 position, Room* room);
    Item* GetItemAtPosition(Vector2 position, Room* room);

    bool IsPositionOccupiedByEnemy(Vector2 position);
    bool IsPositionOccupiedByChest(Vector2 position);
    bool IsPositionOccupiedByItem(Vector2 position);

    // List getters
    std::vector<Enemy*> GetEnemies();
    std::vector<Chest*> GetChests();
    std::vector<Item*> GetItems();

    void ClearAllEntities();

    int GetEnemyCount();
    int GetChestCount();

    // Combat system
    bool TryAttackEnemyAt(Vector2 position, Player* attacker, Room* room);
    bool TryAttackChestAt(Vector2 position, Player* attacker, Room* room);

    // Loot system
    ItemType SelectLoot();
    void DropLoot(Vector2 position, ItemType lootItem, Room* room);
    void RemoveItem(Item* itemToRemove, Room* room);

    // Initialization
    void InitializeRoomEntities(Room* room, int roomX, int roomY);
    void RegisterLoadedEntities(Room* room);

    // Enemy callback configuration
    void SetupEnemyCallbacks(std::function<Vector2()> getPlayerPositionCallback,
        std::function<void(Enemy*)> onEnemyAttackPlayer);

    void ConfigureRoomEnemies(Room* room);

    // Movement validation (called from enemy threads)
    bool CanEnemyMoveTo(Enemy* movingEnemy, Vector2 newPosition);

    void Lock() { _managerMutex.lock(); }
    void Unlock() { _managerMutex.unlock(); }

private:
    // ===== GENERIC HELPER METHODS =====

    // Generic map spawn
    template<typename T>
    void PlaceEntityOnMap(T* entity, Vector2 position, Room* room);

    // Clear entity from map
    void ClearPositionOnMap(Vector2 position, Room* room);

    // Redraw position
    void RedrawPosition(Vector2 position, Room* room);

    // Generic getters
    template<typename T>
    T* GetEntityAtPosition(Vector2 position, Room* room);

    template<typename T>
    bool IsPositionOccupiedBy(Vector2 position);

    // Validation
    Vector2 FindValidSpawnPosition(Room* room);
};

// ===== TEMPLATE IMPLEMENTATIONS =====

template<typename T>
inline T* EntityManager::GetEntityAtPosition(Vector2 position, Room* room)
{
    if (room == nullptr)
        return nullptr;

    if constexpr (std::is_same<T, Enemy>::value)
    {
        for (Enemy* entity : room->GetEnemies())
        {
            Vector2 entityPos = entity->GetPosition();
            if (entityPos.X == position.X && entityPos.Y == position.Y)
                return entity;
        }
    }
    else if constexpr (std::is_same<T, Chest>::value)
    {
        for (Chest* entity : room->GetChests())
        {
            Vector2 entityPos = entity->GetPosition();
            if (entityPos.X == position.X && entityPos.Y == position.Y)
                return entity;
        }
    }
    else if constexpr (std::is_same<T, Item>::value)
    {
        for (Item* entity : room->GetItems())
        {
            Vector2 entityPos = entity->GetPosition();
            if (entityPos.X == position.X && entityPos.Y == position.Y)
                return entity;
        }
    }

    return nullptr;
}

template<typename T>
inline bool EntityManager::IsPositionOccupiedBy(Vector2 position)
{
    Lock();
    bool occupied = false;

    if constexpr (std::is_same<T, Enemy>::value)
    {
        for (const Enemy* entity : _enemies)
        {
            Vector2 entityPos = const_cast<Enemy*>(entity)->GetPosition();
            if (entityPos.X == position.X && entityPos.Y == position.Y)
            {
                occupied = true;
                break;
            }
        }
    }
    else if constexpr (std::is_same<T, Chest>::value)
    {
        for (const Chest* entity : _chests)
        {
            Vector2 entityPos = const_cast<Chest*>(entity)->GetPosition();
            if (entityPos.X == position.X && entityPos.Y == position.Y)
            {
                occupied = true;
                break;
            }
        }
    }
    else if constexpr (std::is_same<T, Item>::value)
    {
        for (const Item* entity : _items)
        {
            Vector2 entityPos = const_cast<Item*>(entity)->GetPosition();
            if (entityPos.X == position.X && entityPos.Y == position.Y)
            {
                occupied = true;
                break;
            }
        }
    }

    Unlock();
    return occupied;
}

template<typename T>
inline void EntityManager::PlaceEntityOnMap(T* entity, Vector2 position, Room* room)
{
    if (room == nullptr || entity == nullptr)
        return;

    room->GetMap()->SafePickNode(position, [entity](Node* node) {
        if (node != nullptr)
        {
            node->SetContent(entity);
        }
        });

    RedrawPosition(position, room);
}