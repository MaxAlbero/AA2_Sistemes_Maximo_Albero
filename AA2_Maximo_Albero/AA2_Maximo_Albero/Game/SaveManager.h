#pragma once
#include <string>
#include <fstream>
#include "../dist/json/json.h"
#include "DungeonMap.h"
#include "Player.h"

class SaveManager
{
public:
    SaveManager(const std::string& saveFilePath = "savegame.json") : _saveFilePath(saveFilePath) {}

    bool SaveGame(DungeonMap* dungeonMap, Player* player);

    bool LoadGame(DungeonMap* worldMap, Player* player);

    bool SaveFileExists();

private:
    std::string _saveFilePath;
};
