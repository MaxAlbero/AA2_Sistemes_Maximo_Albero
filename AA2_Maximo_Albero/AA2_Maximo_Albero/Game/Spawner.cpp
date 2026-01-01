#include "Spawner.h"
#include "Room.h"
#include "EntityManager.h"
#include "../NodeMap/NodeMap.h"
#include "Wall.h"
#include <cstdlib>

Spawner::~Spawner()
{
    Stop();
}

void Spawner::Start(Room* room)
{
    _spawnerMutex.lock();

    if (_running)
    {
        _spawnerMutex.unlock();
        return;
    }

    _currentRoom = room;
    _running = true;

    _spawnThread = new std::thread(&Spawner::SpawnLoop, this);

    _spawnerMutex.unlock();
}

void Spawner::Stop()
{
    _spawnerMutex.lock();

    if (!_running)
    {
        _spawnerMutex.unlock();
        return;
    }

    _running = false;

    _spawnerMutex.unlock();

    if (_spawnThread != nullptr)
    {
        if (_spawnThread->joinable())
            _spawnThread->join();

        delete _spawnThread;
        _spawnThread = nullptr;
    }
}

void Spawner::SpawnLoop()
{
    while (_running)
    {
        // Esperar el intervalo de spawn
        for (int i = 0; i < _spawnIntervalSeconds * 10 && _running; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!_running)
            break;

        SpawnRandomEntity();
    }
}

void Spawner::SpawnRandomEntity()
{
    if (_currentRoom == nullptr || _entityManager == nullptr)
        return;

    Vector2 spawnPosition = GetRandomFreePosition();

    if (spawnPosition.X == -1 && spawnPosition.Y == -1)
        return; // No se encontró posición válida

    rand();

    // Decidir qué spawner: 0 = Enemigo, 1 = Cofre
    int entityType = rand() % 2;

    switch (entityType)
    {
    case 0:
        _entityManager->SpawnEnemy(spawnPosition, _currentRoom);
        break;

    case 1:
        _entityManager->SpawnChest(spawnPosition, _currentRoom);
        break;
    }
}

Vector2 Spawner::GetRandomFreePosition()
{
    if (_currentRoom == nullptr)
        return Vector2(-1, -1);

    NodeMap* map = _currentRoom->GetMap();
    Vector2 mapSize = map->GetSize();

    // Intentar encontrar una posición válida (máximo 50 intentos)
    for (int attempts = 0; attempts < 50; attempts++)
    {
        // Generar posición aleatoria (evitando los bordes que son paredes)
        int x = 1 + (rand() % (mapSize.X - 2));
        int y = 1 + (rand() % (mapSize.Y - 2));

        Vector2 testPosition(x, y);

        if (IsPositionValid(testPosition))
        {
            return testPosition;
        }
    }

    return Vector2(-1, -1); // No se encontró posición válida
}

bool Spawner::IsPositionValid(Vector2 position)
{
    if (_currentRoom == nullptr)
        return false;

    NodeMap* map = _currentRoom->GetMap();
    bool isValid = false;

    map->SafePickNode(position, [&](Node* node) {
        if (node != nullptr)
        {
            // Verificar que no sea una pared
            Wall* wall = node->GetContent<Wall>();
            if (wall != nullptr)
            {
                isValid = false;
                return;
            }

            // Verificar que la casilla esté vacía
            INodeContent* content = node->GetContent();
            if (content == nullptr)
            {
                isValid = true;
            }
        }
        });

    // TODO: También deberías verificar que no esté cerca del jugador
    // Esto lo puedes implementar más adelante si lo necesitas

    return isValid;
}