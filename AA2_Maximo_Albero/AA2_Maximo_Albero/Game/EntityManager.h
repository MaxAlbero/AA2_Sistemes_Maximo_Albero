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
    std::atomic<bool> _isStopping;

    Room* _currentRoom;
    std::function<Vector2()> _getPlayerPositionCallback;
    std::function<void(Enemy*)> _onEnemyAttackPlayer;

public:
    EntityManager();
    ~EntityManager();

    void SetCurrentRoom(Room* room);

    // Spawns
    void SpawnEnemy(Vector2 position, Room* room);
    void SpawnChest(Vector2 position, Room* room);
    void SpawnItem(Vector2 position, ItemType type, Room* room);

    // Ataques unificados
    bool TryAttackAt(Vector2 pos, IAttacker* attacker, Room* room);

    // Movimiento enemigos
    void StartEnemyMovement(Room* room,
        std::function<Vector2()> getPlayerPositionCallback,
        std::function<void(Enemy*)> onEnemyAttackPlayer);

    void StopEnemyMovement();

    // Utilidades
    bool IsPositionBlocked(Vector2 position, Room* room);
    ItemType SelectLoot();
    void DropLoot(Vector2 position, ItemType lootItem, Room* room);

    void RegisterLoadedEntities(Room* room);
    void ClearAllEntities();

    // Funciones centralizadas de mapa
    void ClearMapPosition(Room* room, Vector2 pos);

private:
    void EnemyMovementLoop();
    void MoveEnemy(Enemy* enemy, Room* room, Vector2 playerPos);

    bool IsAdjacent(Vector2 pos1, Vector2 pos2);

    // Funciones centralizadas de mapa
    void PlaceContent(Room* room, INodeContent* content, Vector2 pos);

    // Eliminaciones específicas
    void HandleEnemyDeath(Enemy* enemy, Room* room);
    void HandleChestDeath(Chest* chest, Room* room);
};
