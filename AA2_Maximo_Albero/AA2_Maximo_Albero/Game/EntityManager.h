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

    Room* _currentRoom;  // AÑADIR ESTO
    std::function<Vector2()> _getPlayerPositionCallback;  // AÑADIR ESTO
    std::function<void(Enemy*)> _onEnemyAttackPlayer;

public:
    EntityManager() : _enemyMovementThread(nullptr), _movementActive(false) {}

    ~EntityManager()
    {
        StopEnemyMovement();
        ClearAllEntities();
    }

    void SetCurrentRoom(Room* room) {  // AÑADIR ESTE MÉTODO
        _managerMutex.lock();
        _currentRoom = room;
        _managerMutex.unlock();
    }

    // Gestión de enemigos
    void SpawnEnemy(Vector2 position, Room* room)
    {
        if (room == nullptr)
            return;

        Enemy* enemy = new Enemy(position);

        _managerMutex.lock();
        _enemies.push_back(enemy);
        _managerMutex.unlock();



        //Añadir a la sala
        room->AddEnemy(enemy);

        // Colocar en el mapa
        room->GetMap()->SafePickNode(position, [enemy](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(enemy);
            }
            });

        room->GetMap()->SafePickNode(position, [](Node* node) {
            if (node != nullptr)
            {
                node->DrawContent(Vector2(0, 0));
            }
            });

        enemy->StartMovement();
    }

    void SpawnChest(Vector2 position, Room* room)
    {
        if (room == nullptr)
            return;

        Chest* chest = new Chest(position);


        _managerMutex.lock();
        _chests.push_back(chest);
        _managerMutex.unlock();

        // Añadir a la sala
        room->AddChest(chest);

        room->GetMap()->SafePickNode(position, [chest](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(chest);
            }
            });

        room->GetMap()->SafePickNode(position, [](Node* node) {
            if (node != nullptr)
            {
                node->DrawContent(Vector2(0, 0));
            }
            });
    }

    void CleanupDeadEnemies(Room* room)
    {
        if (room == nullptr)
            return;

        std::vector<Enemy*>& enemies = room->GetEnemies();

        for (auto it = enemies.begin(); it != enemies.end();)
        {
            Enemy* enemy = *it;
            if (!enemy->IsAlive())
            {
                enemy->StopMovement();
                Vector2 enemyPos = enemy->GetPosition();

                ItemType loot = SelectLoot();

                room->GetMap()->SafePickNode(enemyPos, [](Node* node) {
                    if (node != nullptr)
                    {
                        node->SetContent(nullptr);
                    }
                    });

                room->GetMap()->SafePickNode(enemyPos, [](Node* node) {
                    if (node != nullptr)
                    {
                        node->DrawContent(Vector2(0, 0));
                    }
                    });

                _managerMutex.lock();
                auto globalIt = std::find(_enemies.begin(), _enemies.end(), enemy);
                if (globalIt != _enemies.end())
                {
                    _enemies.erase(globalIt);
                }
                _managerMutex.unlock();

                delete enemy;
                it = enemies.erase(it);

                DropLoot(enemyPos, loot, room);
            }
            else
            {
                ++it;
            }
        }
    }

    bool IsPositionOccupiedByEnemy(Vector2 position)
    {
        _managerMutex.lock();

        bool occupied = false;
        for (const Enemy* enemy : _enemies)
        {
            Vector2 enemyPos = const_cast<Enemy*>(enemy)->GetPosition();
            if (enemyPos.X == position.X && enemyPos.Y == position.Y)
            {
                occupied = true;
                break;
            }
        }

        _managerMutex.unlock();
        return occupied;
    }

    Enemy* GetEnemyAtPosition(Vector2 position, Room* room)
    {
        if (room == nullptr)
            return nullptr;

        for (Enemy* enemy : room->GetEnemies())
        {
            Vector2 enemyPos = enemy->GetPosition();
            if (enemyPos.X == position.X && enemyPos.Y == position.Y)
            {
                return enemy;
            }
        }
        return nullptr;
    }

    //Methods to get entities
    std::vector<Enemy*> GetEnemies()
    {
        _managerMutex.lock();
        std::vector<Enemy*> enemiesCopy = _enemies;
        _managerMutex.unlock();
        return enemiesCopy;
    }

    std::vector<Chest*> GetChests()
    {
        _managerMutex.lock();
        std::vector<Chest*> chestsCopy = _chests;
        _managerMutex.unlock();
        return chestsCopy;
    }

    std::vector<Item*> GetItems()
    {
        _managerMutex.lock();
        std::vector<Item*> itemsCopy = _items;
        _managerMutex.unlock();
        return itemsCopy;
    }

    void ClearAllEntities()
    {
        _managerMutex.lock();

        for (Enemy* enemy : _enemies)
        {
            enemy->StopMovement();
            delete enemy;
        }
        _enemies.clear();


        for (Chest* chest : _chests) {
            delete chest;
        }
        _chests.clear();

        for(Item* item : _items){
            delete item;
        }
        _items.clear();

        _managerMutex.unlock();
    }

    int GetEnemyCount()
    {
        _managerMutex.lock();
        int count = _enemies.size();
        _managerMutex.unlock();
        return count;
    }

    // Sistema de movimiento de enemigos
    void StartEnemyMovement(Room* room, std::function<Vector2()> getPlayerPositionCallback, std::function<void(Enemy*)> onEnemyAttackPlayer)
    {
        if (_movementActive)
            return;

        _managerMutex.lock();
        _currentRoom = room;
        _getPlayerPositionCallback = getPlayerPositionCallback;
        _onEnemyAttackPlayer = onEnemyAttackPlayer;
        _managerMutex.unlock();

        _movementActive = true;
        _enemyMovementThread = new std::thread(&EntityManager::EnemyMovementLoop, this);
    }

    void StopEnemyMovement()
    {
        if (!_movementActive)
            return;

        _movementActive = false;

        if (_enemyMovementThread != nullptr)
        {
            if (_enemyMovementThread->joinable())
                _enemyMovementThread->join();

            delete _enemyMovementThread;
            _enemyMovementThread = nullptr;
        }
    }

    bool IsPositionOccupiedByChest(Vector2 position)
    {
        _managerMutex.lock();

        bool occupied = false;
        for (const Chest* chest : _chests)
        {
            Vector2 chestPos = const_cast<Chest*>(chest)->GetPosition();
            if (chestPos.X == position.X && chestPos.Y == position.Y)
            {
                occupied = true;
                break;
            }
        }

        _managerMutex.unlock();
        return occupied;
    }

    Chest* GetChestAtPosition(Vector2 position, Room* room)
    {
        if (room == nullptr)
            return nullptr;

        for (Chest* chest : room->GetChests())
        {
            Vector2 chestPos = chest->GetPosition();
            if (chestPos.X == position.X && chestPos.Y == position.Y)
            {
                return chest;
            }
        }
        return nullptr;
    }

    void CleanupBrokenChests(Room* room)
    {
        if (room == nullptr)
            return;

        std::vector<Chest*>& chests = room->GetChests();

        for (auto it = chests.begin(); it != chests.end();)
        {
            Chest* chest = *it;
            if (chest->IsBroken())
            {

                Vector2 chestPos = chest->GetPosition();

                ItemType loot = SelectLoot();

                room->GetMap()->SafePickNode(chestPos, [](Node* node) {
                    if (node != nullptr)
                    {
                        node->SetContent(nullptr);
                    }
                    });

                room->GetMap()->SafePickNode(chestPos, [](Node* node) {
                    if (node != nullptr)
                    {
                        node->DrawContent(Vector2(0, 0));
                    }
                    });

                _managerMutex.lock();
                auto globalIt = std::find(_chests.begin(), _chests.end(), chest);
                if (globalIt != _chests.end())
                {
                    _chests.erase(globalIt);
                }
                _managerMutex.unlock();

                delete chest;
                it = chests.erase(it);

                DropLoot(chestPos, loot, room);
            }
            else
            {
                ++it;
            }
        }
    }

    int GetChestCount()
    {
        _managerMutex.lock();
        int count = _chests.size();
        _managerMutex.unlock();
        return count;
    }

    void SpawnItem(Vector2 position, ItemType type, Room* room)
    {
        if (room == nullptr)
            return;

        Item* item = new Item(position, type);

        _managerMutex.lock();
        _items.push_back(item);
        _managerMutex.unlock();

        // Añadir a la sala
        room->AddItem(item);

        room->GetMap()->SafePickNode(position, [item](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(item);
            }
            });

        room->GetMap()->SafePickNode(position, [](Node* node) {
            if (node != nullptr)
            {
                node->DrawContent(Vector2(0, 0));
            }
            });
    }

    Item* GetItemAtPosition(Vector2 position, Room* room)
    {
        if (room == nullptr)
            return nullptr;

        for (Item* item : room->GetItems())
        {
            Vector2 itemPos = item->GetPosition();
            if (itemPos.X == position.X && itemPos.Y == position.Y)
            {
                return item;
            }
        }
        return nullptr;
    }
    
    //if something is going to give problems, probably it's going to be this
    void RemoveItem(Item* itemToRemove, Room* room)
    {
        if (room == nullptr || itemToRemove == nullptr)
            return;

        std::vector<Item*>& items = room->GetItems();

        for (auto it = items.begin(); it != items.end(); ++it)
        {
            if (*it == itemToRemove)
            {
                Vector2 itemPos = itemToRemove->GetPosition();

                // Limpiar del mapa
                room->GetMap()->SafePickNode(itemPos, [](Node* node) {
                    if (node != nullptr)
                    {
                        node->SetContent(nullptr);
                    }
                    });

                room->GetMap()->SafePickNode(itemPos, [](Node* node) {
                    if (node != nullptr)
                    {
                        node->DrawContent(Vector2(0, 0));
                    }
                    });

                // Eliminar de la lista global del EntityManager
                _managerMutex.lock();
                auto globalIt = std::find(_items.begin(), _items.end(), itemToRemove);
                if (globalIt != _items.end())
                {
                    _items.erase(globalIt);
                }
                _managerMutex.unlock();

                // Eliminar de la lista de la sala
                delete itemToRemove;
                items.erase(it);

                break; //Salir del bucle después de encontrar el item
            }
        }
    }

    bool IsPositionOccupiedByItem(Vector2 position)
    {
        _managerMutex.lock();

        bool occupied = false;
        for (const Item* item : _items)
        {
            Vector2 itemPos = const_cast<Item*>(item)->GetPosition();
            if (itemPos.X == position.X && itemPos.Y == position.Y)
            {
                occupied = true;
                break;
            }
        }

        _managerMutex.unlock();
        return occupied;
    }

    ItemType SelectLoot() {
        srand((unsigned int)time(NULL)); //this is a problem TO DO: See if I can fix it without needing to use srand in every place I make a random choice
        ItemType lootItem;
        int lootNum = rand() % 3;

        switch (lootNum) {
        case 0:
            lootItem = ItemType::COIN;
            break;
        case 1:
            lootItem = ItemType::POTION;
            break;
        case 2:
            lootItem = ItemType::WEAPON;
            break;
        default:
            lootItem = ItemType::COIN;
        }

        return lootItem;
    }

    void DropLoot(Vector2 position, ItemType lootItem, Room* room)
    {
        if (room == nullptr)
            return;

        SpawnItem(position, lootItem, room);
    }

    void InitializeRoomEntities(Room* room, int roomX, int roomY)
    {
        if (room == nullptr || room->IsInitialized())
            return;

        int entityCount = 3; //1 + (rand() % 2);  // Genera 1 o 2

        for (int i = 0; i < entityCount; i++)
        {
            Vector2 spawnPos = FindValidSpawnPosition(room);

            if (spawnPos.X == -1)
                continue;  // No se encontró posición válida


            int entityType = rand() % 2;

            switch (entityType) {
            case 0:
                SpawnEnemy(spawnPos, room);
                break;
            case 1:
                SpawnChest(spawnPos, room);
                break;
            }
        }

        room->SetInitialized(true);
    }

    bool TryAttackEnemyAt(Vector2 position, Player* attacker, Room* room)
    {
        _managerMutex.lock();

        Enemy* enemy = nullptr;
        for (Enemy* e : _enemies)
        {
            Vector2 enemyPos = e->GetPosition();
            if (enemyPos.X == position.X && enemyPos.Y == position.Y)
            {
                enemy = e;
                break;
            }
        }

        if (enemy != nullptr)
        {
            attacker->Attack(enemy);
            _managerMutex.unlock();

            CleanupDeadEnemies(room);
            return true;
        }

        _managerMutex.unlock();
        return false;
    }

    bool TryAttackChestAt(Vector2 position, Player* attacker, Room* room)
    {
        _managerMutex.lock();

        Chest* chest = nullptr;
        for (Chest* c : _chests)
        {
            Vector2 chestPos = c->GetPosition();
            if (chestPos.X == position.X && chestPos.Y == position.Y)
            {
                chest = c;
                break;
            }
        }

        if (chest != nullptr)
        {
            attacker->Attack(chest);
            _managerMutex.unlock();

            CleanupBrokenChests(room);
            return true;
        }

        _managerMutex.unlock();
        return false;
    }


    void Lock() { _managerMutex.lock(); }
    void Unlock() { _managerMutex.unlock(); }

    void RegisterLoadedEntities(Room* room)
    {
        if (room == nullptr)
            return;

        _managerMutex.lock();

        // Registrar enemigos
        for (Enemy* enemy : room->GetEnemies())
        {
            if (enemy != nullptr)
            {
                // Añadir al vector global si no está ya
                auto it = std::find(_enemies.begin(), _enemies.end(), enemy);
                if (it == _enemies.end())
                {
                    _enemies.push_back(enemy);
                }
            }
        }

        // Registrar cofres
        for (Chest* chest : room->GetChests())
        {
            if (chest != nullptr)
            {
                auto it = std::find(_chests.begin(), _chests.end(), chest);
                if (it == _chests.end())
                {
                    _chests.push_back(chest);
                }
            }
        }

        // Registrar items
        for (Item* item : room->GetItems())
        {
            if (item != nullptr)
            {
                auto it = std::find(_items.begin(), _items.end(), item);
                if (it == _items.end())
                {
                    _items.push_back(item);
                }
            }
        }

        _managerMutex.unlock();
    }

private:
    bool IsAdjacent(Vector2 pos1, Vector2 pos2)
    {
        int distX = abs(pos1.X - pos2.X);
        int distY = abs(pos1.Y - pos2.Y);

        // Adyacente significa que la suma de distancias es exactamente 1
        // (horizontal o vertical, no diagonal)
        return (distX + distY) == 1;
    }

       void EnemyMovementLoop()  //  Sin parámetros ahora
       {
           while (_movementActive)
           {
               std::this_thread::sleep_for(std::chrono::milliseconds(100));

               if (!_movementActive)
                   break;

               // Obtener sala actual y callbacks de forma segura
               _managerMutex.lock();
               Room* room = _currentRoom;
               auto getPlayerPos = _getPlayerPositionCallback;
               auto onAttack = _onEnemyAttackPlayer;
               std::vector<Enemy*> enemiesCopy = _enemies;
               _managerMutex.unlock();

               if (room == nullptr)
                   continue;

               // Obtener la posición actual del jugador
               Vector2 currentPlayerPosition = getPlayerPos();

               std::vector<Enemy*>& enemies = room->GetEnemies();

               for (Enemy* enemy : enemies)  // Usar enemies de la sala actual
               {
                   // Verificar si el enemigo está en la lista global
                   bool isInGlobalList = false;
                   _managerMutex.lock();
                   for (Enemy* globalEnemy : _enemies)
                   {
                       if (globalEnemy == enemy)
                       {
                           isInGlobalList = true;
                           break;
                       }
                   }
                   _managerMutex.unlock();

                   if (!isInGlobalList || !enemy->IsAlive() || !enemy->CanPerformAction())
                       continue;

                   Vector2 currentPos = enemy->GetPosition();

                   if (IsAdjacent(currentPos, currentPlayerPosition))
                   {
                       // El jugador está adyacente - ATACAR
                       onAttack(enemy);
                       enemy->UpdateActionTime();
                       continue;
                   }

                   Vector2 direction = enemy->GetRandomDirection();
                   Vector2 newPos = currentPos + direction;

                   if (CanEnemyMoveTo(newPos, currentPos, room, currentPlayerPosition))
                   {
                       // Limpiar posición anterior
                       room->GetMap()->SafePickNode(currentPos, [](Node* node) {
                           if (node != nullptr)
                           {
                               node->SetContent(nullptr);
                           }
                           });

                       // Actualizar posición del enemigo
                       enemy->SetPosition(newPos);

                       // Colocar enemigo en nueva posición
                       room->GetMap()->SafePickNode(newPos, [enemy](Node* node) {
                           if (node != nullptr)
                           {
                               node->SetContent(enemy);
                           }
                           });

                       // Redibujar ambas posiciones
                       room->GetMap()->SafePickNode(currentPos, [](Node* node) {
                           if (node != nullptr)
                           {
                               node->DrawContent(Vector2(0, 0));
                           }
                           });

                       room->GetMap()->SafePickNode(newPos, [](Node* node) {
                           if (node != nullptr)
                           {
                               node->DrawContent(Vector2(0, 0));
                           }
                           });

                       enemy->UpdateActionTime();
                   }
               }
           }
       }

    bool CanEnemyMoveTo(Vector2 newPosition, Vector2 currentPosition, Room* room, Vector2 playerPosition)
    {
        if (room == nullptr)
            return false;

        bool canMove = true;

        // Verificar si hay una pared
        room->GetMap()->SafePickNode(newPosition, [&](Node* node) {
            if (node != nullptr)
            {
                Wall* wall = node->GetContent<Wall>();
                if (wall != nullptr)
                {
                    canMove = false;
                }

                Portal* portal = node->GetContent<Portal>();
                if (portal != nullptr) {
                    canMove = false;
                }
            }
            });

        if (!canMove)
            return false;

        // Verificar si el jugador está en esa posición
        if (newPosition.X == playerPosition.X && newPosition.Y == playerPosition.Y)
        {
            return false;
        }

        // Verificar si hay otro enemigo (excepto el actual)
        _managerMutex.lock();
        for (Enemy* enemy : _enemies)
        {
            Vector2 enemyPos = enemy->GetPosition();
            if (enemyPos.X == newPosition.X && enemyPos.Y == newPosition.Y)
            {
                // Si es la posición actual del enemigo que se está moviendo, está bien
                if (!(enemyPos.X == currentPosition.X && enemyPos.Y == currentPosition.Y))
                {
                    canMove = false;
                    break;
                }
            }
        }

        if (canMove)
        {
            for (Chest* chest : _chests)
            {
                Vector2 chestPos = chest->GetPosition();
                if (chestPos.X == newPosition.X && chestPos.Y == newPosition.Y)
                {
                    canMove = false;
                    break;
                }
            }
        }

        if (canMove)
        {
            for (Item* item : _items)
            {
                Vector2 itemPos = item->GetPosition();
                if (itemPos.X == newPosition.X && itemPos.Y == newPosition.Y)
                {
                    canMove = false;
                    break;
                }
            }
        }

        _managerMutex.unlock();

        return canMove;
    }

    Vector2 FindValidSpawnPosition(Room* room)
    {
        if (room == nullptr)
            return Vector2(-1, -1);

        Vector2 roomSize = room->GetSize();

        // Intentar hasta 20 veces encontrar una posición válida
        for (int attempt = 0; attempt < 20; attempt++)
        {
            // Generar posición aleatoria evitando bordes (paredes)
            int randomX = 2 + (rand() % (roomSize.X - 4));
            int randomY = 2 + (rand() % (roomSize.Y - 4));
            Vector2 pos(randomX, randomY);

            bool isValid = true;

            // Verificar que no haya contenido en esa posición
            room->GetMap()->SafePickNode(pos, [&](Node* node) {
                if (node != nullptr && node->GetContent() != nullptr)
                {
                    isValid = false;
                }
                });

            if (isValid)
                return pos;
        }

        return Vector2(-1, -1);
    }
    


};