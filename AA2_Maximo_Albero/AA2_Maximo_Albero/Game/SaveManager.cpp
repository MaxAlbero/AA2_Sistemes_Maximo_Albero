#include "SaveManager.h"
#include <iostream>

bool SaveManager::SaveGame(DungeonMap* dungeonMap, Player* player)
{
    Json::Value saveData;

    try
    {
        saveData["player"] = player->Code();
        saveData["worldMap"] = worldMap->Code();

        std::ofstream file(_saveFilePath);
        if (!file.is_open())
        {
            int xOffset = MAP_WIDTH + 5;
            int y = 10;

            CC::Lock();
            CC::SetPosition(xOffset, y++);
            std::cout << "[SaveManager] Error: Could not open file for saving." << std::endl;
            CC::Unlock();
            return false;
        }

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "  ";
        file << Json::writeString(writer, saveData);
        file.close();

        return true;


    }   
    catch (const std::exception& e)
    {
        int xOffset = MAP_WIDTH + 5;
        int y = 10;

        CC::Lock();
        CC::SetPosition(xOffset, y++);
        std::cout << "[SaveManager] Error saving: " << e.what() << std::endl;
        CC::Unlock();
        return false;
    }

    //Json::Value root;

    //// Guardar jugador
    //root["player"] = player->Code();

    //// Guardar sala actual
    //root["currentRoomX"] = dungeonMap->GetCurrentX();
    //root["currentRoomY"] = dungeonMap->GetCurrentY();

    //// Guardar solo las salas inicializadas
    //Json::Value roomsJson(Json::arrayValue);
    //for (int y = 0; y < dungeonMap->GetWorldHeight(); y++) {
    //    for (int x = 0; x < dungeonMap->GetWorldWidth(); x++) {
    //        Room* room = dungeonMap->GetRoom(x, y);
    //        if (room != nullptr && room->IsInitialized()) {
    //            Json::Value roomJson = room->Code();
    //            roomJson["worldX"] = x;
    //            roomJson["worldY"] = y;
    //            roomsJson.append(roomJson);
    //        }
    //    }
    //}
    //root["rooms"] = roomsJson;

    //// Escribir archivo
    //std::ofstream file(_saveFilePath);
    //if (!file.is_open())
    //    return false;

    //Json::StyledWriter writer;
    //file << writer.write(root);
    //file.close();

    //return true;
}


bool SaveManager::LoadGame(DungeonMap* worldMap, Player* player)
{
    std::ifstream file(_saveFilePath);

    if (!file.is_open())
    {
        return false;
    }

    Json::Value saveData;
    Json::CharReaderBuilder reader;
    std::string errors;

    if (!Json::parseFromStream(reader, file, &saveData, &errors))
    {
        int xOffset = MAP_WIDTH + 5;
        int y = 10;

        CC::Lock();
        CC::SetPosition(xOffset, y++);
        std::cout << "[SaveManager] Parse error: " << errors << std::endl;
        CC::Unlock();
        file.close();
        return false;
    }

    file.close();

    try
    {
        player->Decode(saveData["player"]);
        worldMap->Decode(saveData["worldMap"]);

        int xOffset = MAP_WIDTH + 5;
        int y = 10;

        CC::Lock();
        CC::SetPosition(xOffset, y++);
        std::cout << "[SaveManager] Game loaded successfully." << std::endl;
        CC::Unlock();
        return true;
    }
    catch (const std::exception& e)
    {
        int xOffset = MAP_WIDTH + 5;
        int y = 10;

        CC::Lock();
        CC::SetPosition(xOffset, y++);
        std::cout << "[SaveManager] Error loading: " << e.what() << std::endl;
        CC::Unlock();
        return false;
    }
}

bool SaveManager::SaveFileExists()
{
    std::ifstream file(_saveFilePath);
    return file.good();
}