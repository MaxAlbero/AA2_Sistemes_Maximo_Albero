#pragma once
#include <vector>
#include <mutex>
#include "../NodeMap/Vector2.h"
#include "Enemy.h"
#include "Room.h"

class EntityManager
{
private:
    std::vector<Enemy*> _enemies;
    std::mutex _managerMutex;

public:
    EntityManager() {}

    ~EntityManager()
    {
        ClearAllEntities();
    }

    // Gestión de enemigos
    void SpawnEnemy(Vector2 position, Room* room)
    {
        if (room == nullptr)
            return;

        _managerMutex.lock();

        Enemy* enemy = new Enemy(position);
        _enemies.push_back(enemy);

        _managerMutex.unlock();

        // Colocar enemigo en el mapa
        room->GetMap()->SafePickNode(position, [enemy](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(enemy);
            }
            });

        // Dibujar el enemigo
        room->GetMap()->SafePickNode(position, [](Node* node) {
            if (node != nullptr)
            {
                node->DrawContent(Vector2(0, 0));
            }
            });
    }

    void RemoveEnemy(Enemy* enemy, Room* room)
    {
        if (enemy == nullptr || room == nullptr)
            return;

        _managerMutex.lock();

        Vector2 enemyPos = enemy->GetPosition();

        // Limpiar del mapa
        room->GetMap()->SafePickNode(enemyPos, [](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(nullptr);
            }
            });

        // Redibujar la posición
        room->GetMap()->SafePickNode(enemyPos, [](Node* node) {
            if (node != nullptr)
            {
                node->DrawContent(Vector2(0, 0));
            }
            });

        // Eliminar de la lista
        auto it = std::find(_enemies.begin(), _enemies.end(), enemy);
        if (it != _enemies.end())
        {
            delete enemy;
            _enemies.erase(it);
        }

        _managerMutex.unlock();
    }

    void CleanupDeadEnemies(Room* room)
    {
        if (room == nullptr)
            return;

        _managerMutex.lock();

        for (auto it = _enemies.begin(); it != _enemies.end();)
        {
            Enemy* enemy = *it;
            if (!enemy->IsAlive())
            {
                Vector2 enemyPos = enemy->GetPosition();

                // Limpiar del mapa
                room->GetMap()->SafePickNode(enemyPos, [](Node* node) {
                    if (node != nullptr)
                    {
                        node->SetContent(nullptr);
                    }
                    });

                // Redibujar la posición
                room->GetMap()->SafePickNode(enemyPos, [](Node* node) {
                    if (node != nullptr)
                    {
                        node->DrawContent(Vector2(0, 0));
                    }
                    });

                delete enemy;
                it = _enemies.erase(it);
            }
            else
            {
                ++it;
            }
        }

        _managerMutex.unlock();
    }

    bool IsPositionOccupiedByEnemy(Vector2 position)
    {
        _managerMutex.lock();

        bool occupied = false;
        for (const Enemy* enemy : _enemies)
        {
            if (enemy->GetPosition().X == position.X &&
                enemy->GetPosition().Y == position.Y)
            {
                occupied = true;
                break;
            }
        }

        _managerMutex.unlock();
        return occupied;
    }

    Enemy* GetEnemyAtPosition(Vector2 position)
    {
        _managerMutex.lock();

        Enemy* foundEnemy = nullptr;
        for (Enemy* enemy : _enemies)
        {
            if (enemy->GetPosition().X == position.X &&
                enemy->GetPosition().Y == position.Y)
            {
                foundEnemy = enemy;
                break;
            }
        }

        _managerMutex.unlock();
        return foundEnemy;
    }

    std::vector<Enemy*> GetEnemies()
    {
        _managerMutex.lock();
        std::vector<Enemy*> enemiesCopy = _enemies;
        _managerMutex.unlock();
        return enemiesCopy;
    }

    void ClearAllEntities()
    {
        _managerMutex.lock();

        for (Enemy* enemy : _enemies)
        {
            delete enemy;
        }
        _enemies.clear();

        _managerMutex.unlock();

        // Aquí en el futuro añadirás:
        // for (Chest* chest : _chests) delete chest;
        // _chests.clear();
    }

    int GetEnemyCount()
    {
        _managerMutex.lock();
        int count = _enemies.size();
        _managerMutex.unlock();
        return count;
    }

    void Lock() { _managerMutex.lock(); }
    void Unlock() { _managerMutex.unlock(); }
};