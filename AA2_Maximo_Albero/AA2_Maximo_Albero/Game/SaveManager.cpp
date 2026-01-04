#include "SaveManager.h"

bool SaveManager::SaveGame(DungeonMap* dungeonMap, Player* player)
{
    std::lock_guard<std::mutex> lock(_saveMutex);

    try
    {
        if (dungeonMap == nullptr || player == nullptr)
        {
            std::cerr << "Error: DungeonMap o Player es nullptr" << std::endl;
            return false;
        }

        Json::Value root;

        // Guardar jugador
        root["player"] = player->Code();


        CC::Lock();
        CC::SetPosition(15, 11);
        std::cout << " DEBUG SaveGame: Guardando jugador en posición ("
            << player->GetPosition().X << ", "
            << player->GetPosition().Y << ")" << std::endl;
        CC::Unlock();

        // Guardar posición actual en el mundo
        root["currentX"] = dungeonMap->GetCurrentX();
        root["currentY"] = dungeonMap->GetCurrentY();
        root["worldWidth"] = dungeonMap->GetWorldWidth();
        root["worldHeight"] = dungeonMap->GetWorldHeight();

        // Guardar todas las salas (solo las que están inicializadas)
        Json::Value roomsData;
        for (int y = 0; y < dungeonMap->GetWorldHeight(); y++)
        {
            for (int x = 0; x < dungeonMap->GetWorldWidth(); x++)
            {
                Room* room = dungeonMap->GetRoom(x, y);
                if (room != nullptr && room->IsInitialized())
                {
                    roomsData[std::to_string(y)][std::to_string(x)] = room->Code();
                }
            }
        }
        root["rooms"] = roomsData;

        // Escribir al archivo
        std::ofstream file(_saveFilePath);
        if (!file.is_open())
        {
            std::cerr << "Error: No se pudo abrir el archivo para guardar: " << _saveFilePath << std::endl;
            return false;
        }

        Json::StyledWriter writer;
        file << writer.write(root);
        file.close();

        return true;
    }
    catch (const Json::Exception& e)
    {
        std::cerr << "Error JSON al guardar: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error al guardar la partida: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cerr << "Error desconocido al guardar la partida" << std::endl;
        return false;
    }
}

bool SaveManager::LoadGame(DungeonMap* dungeonMap, Player* player, EntityManager* entityManager)
{
    std::lock_guard<std::mutex> lock(_saveMutex);

    try
    {
        if (dungeonMap == nullptr || player == nullptr || entityManager == nullptr)
        {
            std::cerr << "Error: DungeonMap, Player o EntityManager es nullptr" << std::endl;
            return false;
        }

        // Abrir archivo
        std::ifstream file(_saveFilePath);
        if (!file.is_open())
        {
            std::cerr << "Error: No se pudo abrir el archivo de guardado: " << _saveFilePath << std::endl;
            return false;
        }

        // Parsear JSON
        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(file, root))
        {
            std::cerr << "Error al parsear JSON: " << reader.getFormattedErrorMessages() << std::endl;
            file.close();
            return false;
        }
        file.close();

        // Cargar jugador
        player->Decode(root["player"]);

        int xOffset = 15;
        int y = 10;

        CC::Lock();
        CC::SetPosition(xOffset, y++);
        std::cout << "DEBUG LoadGame: Jugador cargado en posición ("
            << root["player"]["posX"].asInt() << ", "
            << root["player"]["posY"].asInt() << ")" << std::endl;
        CC::Unlock();

        // Cargar posición del mundo
        int currentX = root["currentX"].asInt();
        int currentY = root["currentY"].asInt();
        int worldWidth = root["worldWidth"].asInt();
        int worldHeight = root["worldHeight"].asInt();

        // Verificar que las dimensiones coincidan
        if (worldWidth != dungeonMap->GetWorldWidth() ||
            worldHeight != dungeonMap->GetWorldHeight())
        {
            std::cerr << "Error: Las dimensiones del mundo guardado no coinciden" << std::endl;
            return false;
        }

        // Cargar todas las salas
        Json::Value roomsData = root["rooms"];
        for (int y = 0; y < worldHeight; y++)
        {
            for (int x = 0; x < worldWidth; x++)
            {
                std::string yKey = std::to_string(y);
                std::string xKey = std::to_string(x);

                Room* room = dungeonMap->GetRoom(x, y);
                if (room == nullptr)
                    continue;

                if (roomsData.isMember(yKey) && roomsData[yKey].isMember(xKey))
                {
                    room->Decode(roomsData[yKey][xKey]);

                    // Registrar todas las entidades en el EntityManager
                    entityManager->RegisterLoadedEntities(room);
                }

                // Regenerar portales basándose en la posición del mundo
                room->GeneratePortals(x, y, worldWidth, worldHeight);
            }
        }

        // Establecer la sala activa
        dungeonMap->SetActiveRoom(currentX, currentY);

        std::cout << "Partida cargada exitosamente desde: " << _saveFilePath << std::endl;
        return true;
    }
    catch (const Json::Exception& e)
    {
        std::cerr << "Error JSON al cargar: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error al cargar la partida: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cerr << "Error desconocido al cargar la partida" << std::endl;
        return false;
    }
}

void SaveManager::StartAutoSave(DungeonMap* dungeonMap, Player* player, EntityManager* entityManager)
{
    if (_isAutoSaving)
        return;

    _dungeonMapRef = dungeonMap;
    _playerRef = player;
    _entityManagerRef = entityManager;
    _isAutoSaving = true;

    _autoSaveThread = new std::thread(&SaveManager::AutoSaveLoop, this);

    std::cout << "Sistema de autoguardado iniciado (cada " << _autoSaveIntervalSeconds << " segundos)" << std::endl;
}

bool SaveManager::SaveFileExists()
{
    std::ifstream file(_saveFilePath);
    bool exists = file.good();
    file.close();
    return exists;
}

void SaveManager::StopAutoSave()
{
    if (!_isAutoSaving)
        return;

    _isAutoSaving = false;

    if (_autoSaveThread != nullptr)
    {
        if (_autoSaveThread->joinable())
            _autoSaveThread->join();

        delete _autoSaveThread;
        _autoSaveThread = nullptr;
    }

    std::cout << "Sistema de autoguardado detenido" << std::endl;
}

void SaveManager::AutoSaveLoop()
{
    while (_isAutoSaving)
    {
        // Esperar el intervalo de guardado
        for (int i = 0; i < _autoSaveIntervalSeconds * 10 && _isAutoSaving; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!_isAutoSaving)
            break;

        // Realizar guardado automático
        if (SaveGame(_dungeonMapRef, _playerRef))
        {
            std::cout << "[AutoSave] Partida guardada automáticamente" << std::endl;
        }
        else
        {
            std::cerr << "[AutoSave] Error al guardar automáticamente" << std::endl;
        }
    }
}