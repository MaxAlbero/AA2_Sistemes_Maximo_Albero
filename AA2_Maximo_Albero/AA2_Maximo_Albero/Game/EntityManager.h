#pragma once
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include "../NodeMap/Vector2.h"
#include "Enemy.h"
#include "Room.h"
#include "Wall.h"

class EntityManager
{
private:
    std::vector<Enemy*> _enemies;
    std::mutex _managerMutex;

    // Thread para gestionar movimiento de enemigos
    std::thread* _enemyMovementThread;
    std::atomic<bool> _movementActive;

public:
    EntityManager() : _enemyMovementThread(nullptr), _movementActive(false) {}

    ~EntityManager()
    {
        StopEnemyMovement();
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

        // Iniciar movimiento del enemigo
        enemy->StartMovement();
    }

    //void RemoveEnemy(Enemy* enemy, Room* room)
    //{
    //    if (enemy == nullptr || room == nullptr)
    //        return;

    //    // Detener el movimiento del enemigo
    //    enemy->StopMovement();

    //    _managerMutex.lock();

    //    Vector2 enemyPos = enemy->GetPosition();

    //    // Limpiar del mapa
    //    room->GetMap()->SafePickNode(enemyPos, [](Node* node) {
    //        if (node != nullptr)
    //        {
    //            node->SetContent(nullptr);
    //        }
    //        });

    //    // Redibujar la posición
    //    room->GetMap()->SafePickNode(enemyPos, [](Node* node) {
    //        if (node != nullptr)
    //        {
    //            node->DrawContent(Vector2(0, 0));
    //        }
    //        });

    //    // Eliminar de la lista
    //    auto it = std::find(_enemies.begin(), _enemies.end(), enemy);
    //    if (it != _enemies.end())
    //    {
    //        delete enemy;
    //        _enemies.erase(it);
    //    }

    //    _managerMutex.unlock();
    //}

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
                enemy->StopMovement();
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

    Enemy* GetEnemyAtPosition(Vector2 position)
    {
        _managerMutex.lock();

        Enemy* foundEnemy = nullptr;
        for (Enemy* enemy : _enemies)
        {
            Vector2 enemyPos = enemy->GetPosition();
            if (enemyPos.X == position.X && enemyPos.Y == position.Y)
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
            enemy->StopMovement();
            delete enemy;
        }
        _enemies.clear();

        _managerMutex.unlock();
    }

    int GetEnemyCount()
    {
        _managerMutex.lock();
        int count = _enemies.size();
        _managerMutex.unlock();
        return count;
    }

    // Sistema de movimiento de enemigos - SOLO ESTA VERSIÓN
    void StartEnemyMovement(Room* room, std::function<Vector2()> getPlayerPositionCallback, std::function<void(Enemy*)> onEnemyAttackPlayer)
    {
        if (_movementActive)
            return;

        _movementActive = true;
        _enemyMovementThread = new std::thread(&EntityManager::EnemyMovementLoop, this, room, getPlayerPositionCallback, onEnemyAttackPlayer);
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

    void Lock() { _managerMutex.lock(); }
    void Unlock() { _managerMutex.unlock(); }

private:
    bool IsAdjacent(Vector2 pos1, Vector2 pos2)
    {
        int distX = abs(pos1.X - pos2.X);
        int distY = abs(pos1.Y - pos2.Y);

        // Adyacente significa que la suma de distancias es exactamente 1
        // (horizontal o vertical, no diagonal)
        return (distX + distY) == 1;
    }

    void EnemyMovementLoop(Room* room,
        std::function<Vector2()> getPlayerPositionCallback,
        std::function<void(Enemy*)> onEnemyAttackPlayer)
    {
        while (_movementActive)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (!_movementActive)
                break;

            _managerMutex.lock();
            std::vector<Enemy*> enemiesCopy = _enemies;
            _managerMutex.unlock();

            // Obtener la posición actual del jugador
            Vector2 currentPlayerPosition = getPlayerPositionCallback();

            for (Enemy* enemy : enemiesCopy)
            {
                if (!enemy->IsAlive() || !enemy->CanPerformAction())
                    continue;

                Vector2 currentPos = enemy->GetPosition();

                if (IsAdjacent(currentPos, currentPlayerPosition))
                {
                    // El jugador está adyacente - ATACAR
                    onEnemyAttackPlayer(enemy);
                    enemy->UpdateActionTime();
                    continue; // No moverse, solo atacar
                }

                Vector2 direction = enemy->GetRandomDirection();
                Vector2 newPos = currentPos + direction;

                // Verificar si la nueva posición es válida (ahora con la posición actualizada del jugador)
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
        _managerMutex.unlock();

        return canMove;
    }
};