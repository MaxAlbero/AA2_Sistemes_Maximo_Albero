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

//Inicia el sistema de spawn periódico para una sala específica
// Parametro room: Sala donde se spawnerán las entidades
// Crea un thread que ejecuta SpawnLoop()
// Se llama al entrar a una sala o iniciar el juego
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

//Detiene el sistema de spawn
// Cuando: 1.El jugador cambia de sala (ChangeRoom) o 2.El juego termina
// Debe detenerse al cambiar de sala para que no spawnee en sala vacía
// THREAD-SAFETY: join() espera a que el thread termine antes de continuar
void Spawner::Stop()
{
    _spawnerMutex.lock();

    if (!_running)
    {
        _spawnerMutex.unlock();
        return;
    }

    _running = false;
    _spawnerMutex.unlock(); // IMPORTANTE: Desbloquear ANTES de join

    // Esperar un poco para que el loop detecte el cambio
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    if (_spawnThread != nullptr)
    {
        if (_spawnThread->joinable())
            _spawnThread->join();

        delete _spawnThread;
        _spawnThread = nullptr;
    }
}

// Loop principal que ejecuta spawns periódicos
// EJECUCIÓN: Thread separado, corre mientras _running == true
// FRECUENCIA: Cada _spawnIntervalSeconds segundos (por defecto 10)
void Spawner::SpawnLoop()
{
    while (_running)
    {
        // En lugar de sleep(10000), usar sleeps cortos de 100ms
        // Permite detener el thread rápidamente cuando _running cambie
        // _spawnIntervalSeconds * 10 = 10 segundos * 10 = 100 sleeps de 100ms
        for (int i = 0; i < _spawnIntervalSeconds * 10 && _running; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Verificar de nuevo antes de spawner por si se detuvo durante el sleep
        if (!_running)
            break;

        // Ejecutar spawn de entidad aleatoria
        SpawnRandomEntity();
    }
}

// Genera una entidad aleatoria(enemigo o cofre) en posición válida
// PROCESO:
//   1. Buscar posición libre válida (GetRandomFreePosition)
//   2. Decidir qué spawner (50% enemigo, 50% cofre)
//   3. Delegar spawn al EntityManager
void Spawner::SpawnRandomEntity()
{
    if (_currentRoom == nullptr || _entityManager == nullptr)
        return;

    Vector2 spawnPosition = GetRandomFreePosition();

    if (spawnPosition.X == -1 && spawnPosition.Y == -1)
        return; // No se encontró posición válida

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

//Encuentra una posición aleatoria válida para spawner una entidad
// RESTRICCIONES:
//   - No en paredes (bordes del mapa)
//   - No en casillas ocupadas (enemigos, cofres, items, jugador)
// ALGORITMO: Intentar hasta 50 veces generar posición aleatoria válida
// RETORNO: Vector2 con coordenadas válidas, o (-1, -1) si no se encontró
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

    return isValid;
}

void Spawner::SpawnInitialEntities(Room* room)
{
    for (int i = 0; i < 3; i++)
        SpawnRandomEntity();
}