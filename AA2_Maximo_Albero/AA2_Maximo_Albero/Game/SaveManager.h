#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include "../dist/json/json.h"
#include "DungeonMap.h"
#include "Player.h"
#include "../Utils/GameConstants.h"

#include "EntityManager.h"

class SaveManager
{
public:
    SaveManager(const std::string& saveFilePath = "savegame.json", int autoSaveIntervalSeconds = 10)
        : _saveFilePath(saveFilePath),
        _autoSaveIntervalSeconds(autoSaveIntervalSeconds),
        _autoSaveThread(nullptr),
        _isAutoSaving(false),
        _entityManagerRef(nullptr) {
    }

    ~SaveManager() {
        StopAutoSave();
    }

    bool SaveGame(DungeonMap* dungeonMap, Player* player);
    bool LoadGame(DungeonMap* dungeonMap, Player* player, EntityManager* entityManager); // MODIFICADO

    bool SaveFileExists();

    void StartAutoSave(DungeonMap* dungeonMap, Player* player, EntityManager* entityManager); // MODIFICADO
    void StopAutoSave();

private:
    std::string _saveFilePath;
    int _autoSaveIntervalSeconds;

    std::thread* _autoSaveThread;
    std::atomic<bool> _isAutoSaving;
    std::mutex _saveMutex;

    DungeonMap* _dungeonMapRef;
    Player* _playerRef;
    EntityManager* _entityManagerRef; // AÑADIDO

    void AutoSaveLoop();
};