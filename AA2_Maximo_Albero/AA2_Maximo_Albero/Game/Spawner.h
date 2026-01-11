#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>
#include "../NodeMap/Vector2.h"

// Forward declarations
class Room;
class EntityManager;

class Spawner
{
public:
    Spawner(EntityManager* entityManager, int spawnIntervalSeconds = 10)
        : _entityManager(entityManager),
        _spawnIntervalSeconds(spawnIntervalSeconds),
        _running(false),
        _spawnThread(nullptr) {
    }

    ~Spawner();

    void Start(Room* room);
    void Stop();

    void SpawnInitialEntities(Room* room);

private:
    EntityManager* _entityManager;
    Room* _currentRoom;
    int _spawnIntervalSeconds;

    std::thread* _spawnThread;
    std::atomic<bool> _running;
    std::mutex _spawnerMutex;

    void SpawnLoop();
    void SpawnRandomEntity();
    Vector2 GetRandomFreePosition();
    bool IsPositionValid(Vector2 position);
};