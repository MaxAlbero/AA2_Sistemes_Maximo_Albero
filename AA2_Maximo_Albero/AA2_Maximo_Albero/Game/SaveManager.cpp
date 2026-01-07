#include "SaveManager.h"

// Guarda el estado completo del juego en formato JSON
// GUARDA:
//   - Estado del jugador (HP, monedas, pociones, arma, posición)
//   - Posición actual en el mapamundi
//   - Estado de TODAS las salas inicializadas con sus entidades
// THREAD-SAFETY: Protegido con _saveMutex manualmente
bool SaveManager::SaveGame(DungeonMap* dungeonMap, Player* player)
{
    Lock();

    try
    {
        if (dungeonMap == nullptr || player == nullptr)
        {
            std::cerr << "Error: DungeonMap o Player es nullptr" << std::endl;
            Unlock();
            return false;
        }

        Json::Value root;

        // Serializa posición, HP, inventario, arma equipada del player
        root["player"] = player->Code();

        CC::Lock();
        CC::SetPosition(15, 11);
        CC::Unlock();

        // Guardar posición actual en el mundo
        root["currentX"] = dungeonMap->GetCurrentX();
        root["currentY"] = dungeonMap->GetCurrentY();
        root["worldWidth"] = dungeonMap->GetWorldWidth();
        root["worldHeight"] = dungeonMap->GetWorldHeight();

        // Solo guardar salas que el jugador ha visitado
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

        // Escribir al archivo JSON
        std::ofstream file(_saveFilePath);
        if (!file.is_open())
        {
            std::cerr << "Error: No se pudo abrir el archivo para guardar: " << _saveFilePath << std::endl;
            Unlock();
            return false;
        }

        Json::StyledWriter writer;
        file << writer.write(root);
        file.close();

        Unlock();
        return true;
    }
    catch (const Json::Exception& e)
    {
        std::cerr << "Error JSON al guardar: " << e.what() << std::endl;
        Unlock();
        return false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error al guardar la partida: " << e.what() << std::endl;
        Unlock();
        return false;
    }
    catch (...)
    {
        std::cerr << "Error desconocido al guardar la partida" << std::endl;
        Unlock();
        return false;
    }
}

// Carga una partida guardada y reconstruye el estado completo
// PROCESO:
//   1. Leer archivo JSON
//   2. Reconstruir jugador con todos sus stats
//   3. Reconstruir todas las salas con sus entidades
//   4. Registrar entidades en EntityManager
//   5. Establecer sala activa correcta
// THREAD-SAFETY: Protegido con _saveMutex manualmente
// IMPORTANTE: Se llama ANTES de iniciar threads, no hay race conditions
bool SaveManager::LoadGame(DungeonMap* dungeonMap, Player* player, EntityManager* entityManager)
{
    Lock();

    try
    {
        if (dungeonMap == nullptr || player == nullptr || entityManager == nullptr)
        {
            std::cerr << "Error: DungeonMap, Player o EntityManager es nullptr" << std::endl;
            Unlock();
            return false;
        }

        // Abrir archivo
        std::ifstream file(_saveFilePath);
        if (!file.is_open())
        {
            std::cerr << "Error: No se pudo abrir el archivo de guardado: " << _saveFilePath << std::endl;
            Unlock();
            return false;
        }

        // Parsear JSON
        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(file, root))
        {
            std::cerr << "Error al parsear JSON: " << reader.getFormattedErrorMessages() << std::endl;
            file.close();
            Unlock();
            return false;
        }
        file.close();

        // Cargar jugador
        player->Decode(root["player"]);

        int xOffset = 15;
        int y = 10;

        CC::Lock();
        CC::SetPosition(xOffset, y++);
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
            std::cout << "Error: Las dimensiones del mundo guardado no coinciden" << std::endl;
            Unlock();
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
        Unlock();
        return true;
    }
    catch (const Json::Exception& e)
    {
        std::cerr << "Error JSON al cargar: " << e.what() << std::endl;
        Unlock();
        return false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error al cargar la partida: " << e.what() << std::endl;
        Unlock();
        return false;
    }
    catch (...)
    {
        std::cerr << "Error desconocido al cargar la partida" << std::endl;
        Unlock();
        return false;
    }
}

//Loop que ejecuta autoguardado cada X segundos en thread separado
// EJECUCIÓN: Thread independiente iniciado por StartAutoSave()
// Guarda cada 5 segundos por defecto
// THREAD-SAFETY: SaveGame() ya tiene su propio mutex interno
void SaveManager::StartAutoSave(DungeonMap* dungeonMap, Player* player, EntityManager* entityManager)
{
    if (_isAutoSaving)
        return;

    _dungeonMapRef = dungeonMap;
    _playerRef = player;
    _entityManagerRef = entityManager;
    _isAutoSaving = true;

    _autoSaveThread = new std::thread(&SaveManager::AutoSaveLoop, this);

    CC::Lock();
    CC::SetPosition(MAP_WIDTH, 10);
    std::cout << "Sistema de autoguardado iniciado (cada " << _autoSaveIntervalSeconds << " segundos)" << std::endl;
    CC::Unlock();
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
            CC::Lock();
            CC::SetPosition(MAP_WIDTH, 11);
            std::cout << "[AutoSave] Partida guardada" << std::endl;
            CC::Unlock();
        }
        else
        {
            CC::Lock();
            CC::SetPosition(MAP_WIDTH, 11);
            std::cerr << "[AutoSave] Error al guardar" << std::endl;
            CC::Unlock();
        }
    }
}