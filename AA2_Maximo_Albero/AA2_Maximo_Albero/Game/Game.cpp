#include "Game.h"
#include "Wall.h"
#include "../Utils/ConsoleControl.h"
#include <iostream>

Game::Game()
{
    _dungeonMap = new DungeonMap(3, 3);
    _inputSystem = new InputSystem();
    _entityManager = new EntityManager();
    _spawner = new Spawner(_entityManager, 10);
    _ui = new UI();
    _player = nullptr;
    _playerPosition = Vector2(1, 1);
    _currentRoomIndex = 0;
    _running = false;

    _saveManager = new SaveManager("savegame.json", 5); // Autoguardado cada 5 segundos
}

Game::~Game()
{
    Stop();

    if (_player != nullptr)
        delete _player;

    delete _saveManager;
    delete _ui;
    delete _spawner;
    delete _entityManager;
    delete _inputSystem;
    delete _dungeonMap;
}

void Game::InitializeCurrentRoom()
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom == nullptr)
        return;

    int currentX = _dungeonMap->GetCurrentX();
    int currentY = _dungeonMap->GetCurrentY();

    // Solo inicializar si no se ha inicializado antes
    _entityManager->InitializeRoomEntities(currentRoom, currentX, currentY);
}

void Game::Start()
{
    srand((unsigned int)time(NULL));

    _gameMutex.lock();

    if (_running)
    {
        _gameMutex.unlock();
        return;
    }

    _running = true;

    // Inicializar el mapa
    Vector2 roomSize(20, 10);

    // Crear las 9 salas
    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            Room* room = new Room(roomSize, Vector2(0, 0));
            room->GeneratePortals(x, y, 3, 3);
            _dungeonMap->SetRoom(x, y, room);
        }
    }

    // INTENTAR CARGAR PARTIDA GUARDADA
    bool loadedGame = false;
    if (_saveManager->SaveFileExists())
    {
        std::cout << "Partida guardada encontrada. Cargando..." << std::endl;

        // Crear jugador temporal
        _player = new Player(Vector2(0, 0));

        if (_saveManager->LoadGame(_dungeonMap, _player, _entityManager))
        {
            // Después de LoadGame, el DungeonMap ya tiene la sala correcta establecida
            // Ahora obtén la posición del jugador
            _playerPosition = _player->GetPosition();

            std::cout << " DEBUG - Sala activa después de cargar: ("
                << _dungeonMap->GetCurrentX() << ", "
                << _dungeonMap->GetCurrentY() << ")" << std::endl;
            std::cout << " DEBUG - Posición del jugador: ("
                << _playerPosition.X << ", " << _playerPosition.Y << ")" << std::endl;

            // Activar entidades de la sala actual (que ahora es la correcta)
            Room* currentRoom = _dungeonMap->GetActiveRoom();
            if (currentRoom != nullptr)
            {
                currentRoom->ActivateEntities();

                // Reactivar movimiento de enemigos
                for (Enemy* enemy : currentRoom->GetEnemies())
                {
                    if (enemy != nullptr)
                    {
                        enemy->StartMovement();
                    }
                }

                // Colocar jugador en el mapa
                currentRoom->GetMap()->SafePickNode(_playerPosition, [this](Node* node) {
                    if (node != nullptr)
                    {
                        node->SetContent(this->_player);
                    }
                    });
            }

            loadedGame = true;
            std::cout << " Partida cargada exitosamente en sala ("
                << _dungeonMap->GetCurrentX() << ", "
                << _dungeonMap->GetCurrentY() << "), posición jugador ("
                << _playerPosition.X << ", " << _playerPosition.Y << ")" << std::endl;
        }
        else
        {
            std::cerr << " Error al cargar la partida. Iniciando nueva partida..." << std::endl;
            delete _player;
            _player = nullptr;
        }
    }

    // Si no se cargó, crear nueva partida
    if (!loadedGame)
    {
        std::cout << "Iniciando nueva partida..." << std::endl;

        // AHORA SÍ establece la sala inicial para nueva partida
        _dungeonMap->SetActiveRoom(1, 1);

        _playerPosition = Vector2(10, 5);
        _player = new Player(_playerPosition);

        // Colocar al jugador en el mapa
        Room* currentRoom = _dungeonMap->GetActiveRoom();
        if (currentRoom != nullptr)
        {
            currentRoom->GetMap()->SafePickNode(_playerPosition, [this](Node* node) {
                if (node != nullptr)
                {
                    node->SetContent(this->_player);
                }
                });
        }
    }

    _ui->SetMapSize(Vector2(20, 10));

    // Registrar los listeners de input
    _inputSystem->AddListener(K_W, [this]() { this->OnMoveUp(); });
    _inputSystem->AddListener(K_S, [this]() { this->OnMoveDown(); });
    _inputSystem->AddListener(K_A, [this]() { this->OnMoveLeft(); });
    _inputSystem->AddListener(K_D, [this]() { this->OnMoveRight(); });

    _inputSystem->AddListener(K_SPACE, [this]() {
        if (this->_player != nullptr)
        {
            this->_player->UsePotion();
        }
        });

    _gameMutex.unlock();

    // Limpiar pantalla y dibujar el mapa inicial
    CC::Clear();
    DrawCurrentRoom();

    _ui->Start(_player);

    // Iniciar el sistema de input
    _inputSystem->StartListen();

    // Obtener la sala actual DESPUÉS de todo (sea cargada o nueva)
    Room* currentRoom = _dungeonMap->GetActiveRoom();

    // Iniciar movimiento de enemigos
    _entityManager->StartEnemyMovement(
        currentRoom,
        [this]() {
            this->_gameMutex.lock();
            Vector2 pos = this->_playerPosition;
            this->_gameMutex.unlock();
            return pos;
        },
        [this](Enemy* enemy) {
            if (enemy != nullptr && this->_player != nullptr)
            {
                enemy->Attack(this->_player);
            }
        }
    );

    // Solo inicializar la sala actual si NO se cargó una partida
    if (!loadedGame)
    {
        InitializeCurrentRoom();
    }

    _spawner->Start(currentRoom);

    // INICIAR AUTOGUARDADO
    _saveManager->StartAutoSave(_dungeonMap, _player, _entityManager);
}

   
void Game::Stop()
{
    _gameMutex.lock();

    if (!_running)
    {
        _gameMutex.unlock();
        return;
    }

    _running = false;

    // Detener autoguardado PRIMERO
    _saveManager->StopAutoSave();

    _inputSystem->StopListen();
    _entityManager->StopEnemyMovement();
    _spawner->Stop();
    _ui->Stop();

    _gameMutex.unlock();
}


void Game::DrawCurrentRoom()
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom != nullptr)
    {
        currentRoom->Draw();
    }
}

bool Game::CanMoveTo(Vector2 position)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom == nullptr)
        return false;

    bool canMove = false;

    // Esto sirve para verificar que no sea una pared
    currentRoom->GetMap()->SafePickNode(position, [&](Node* node) {
        if (node != nullptr)
        {

            Wall* wall = node->GetContent<Wall>();
            canMove = (wall == nullptr);
        }
        });

    return canMove;
}

void Game::UpdatePlayerOnMap()
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom == nullptr)
        return;

    currentRoom->GetMap()->SafePickNode(_playerPosition, [&](Node* node) {
        if (node != nullptr)
        {
            node->SetContent(_player);
        }
        });
}

void Game::MovePlayer(Vector2 direction)
{
    _gameMutex.lock();

    if (!_running)
    {
        _gameMutex.unlock();
        return;
    }

    // Verificar si el jugador puede realizar una acción
    if (_player != nullptr && !_player->CanPerformAction())
    {
        _gameMutex.unlock();
        return;
    }

    Vector2 newPosition = _playerPosition + direction;

    // Verificar si la posición es válida (no hay pared)
    if (!CanMoveTo(newPosition))
    {
        _gameMutex.unlock();
        return;
    }

    Room* currentRoom = _dungeonMap->GetActiveRoom();

    // Verificar si hay un portal en la nueva posición
    bool isPortal = false;
    Portal* portalPtr = nullptr;

    currentRoom->GetMap()->SafePickNode(newPosition, [&](Node* node) {
        if (node != nullptr)
        {
            Portal* portal = node->GetContent<Portal>();
            if (portal != nullptr)
            {
                isPortal = true;
                portalPtr = portal;
            }
        }
        });

    if (isPortal && portalPtr != nullptr)
    {
        //Guardar la dirección del portal y desbloquear ANTES de cambiar de sala
        PortalDir portalDirection = portalPtr->GetDirection();
        _player->UpdateActionTime();

        _gameMutex.unlock();

        // Ahora cambiar de sala SIN el mutex bloqueado
        ChangeRoom(portalDirection);
        return;
    }

    int attackRange = _player->GetAttackRange();
    bool attacked = false;

    for (int range = 1; range <= attackRange && !attacked; range++)
    {
        Vector2 targetPosition = Vector2(
            _playerPosition.X + (direction.X * range),
            _playerPosition.Y + (direction.Y * range)
        );

        bool pathBlocked = false;
        for (int checkRange = 1; checkRange <= range; checkRange++)
        {
            Vector2 checkPos = Vector2(
                _playerPosition.X + (direction.X * checkRange),
                _playerPosition.Y + (direction.Y * checkRange)
            );

            if (!CanMoveTo(checkPos))
            {
                pathBlocked = true;
                break;
            }
        }

        if (pathBlocked)
            break;

        if (_entityManager->TryAttackEnemyAt(targetPosition, _player, currentRoom))
        {
            _player->UpdateActionTime();
            attacked = true;
            break;
        }

        if (_entityManager->TryAttackChestAt(targetPosition, _player, currentRoom))
        {
            _player->UpdateActionTime();
            attacked = true;
            break;
        }
    }

    if (attacked)
    {
        _gameMutex.unlock();
        return;
    }

    Enemy* enemyAtPosition = _entityManager->GetEnemyAtPosition(newPosition, currentRoom);
    if (enemyAtPosition != nullptr)
    {
        _player->Attack(enemyAtPosition);
        _player->UpdateActionTime();
        _entityManager->CleanupDeadEnemies(currentRoom);
        _gameMutex.unlock();
        return;
    }

    Chest* chestAtPosition = _entityManager->GetChestAtPosition(newPosition, currentRoom);
    if (chestAtPosition != nullptr)
    {
        _player->Attack(chestAtPosition);
        _player->UpdateActionTime();
        _entityManager->CleanupBrokenChests(currentRoom);
        _gameMutex.unlock();
        return;
    }

    Item* itemAtPosition = _entityManager->GetItemAtPosition(newPosition, currentRoom);
    if (itemAtPosition != nullptr)
    {
        ItemType type = itemAtPosition->GetType();
        switch (type)
        {
        case ItemType::COIN:
            _player->AddCoin();
            break;
        case ItemType::POTION:
            _player->AddPotion();
            break;
        case ItemType::WEAPON:
            _player->ChangeWeapon();
            break;
        }
        _entityManager->RemoveItem(itemAtPosition, currentRoom);
    }

    Vector2 oldPosition = _playerPosition;

    currentRoom->GetMap()->SafePickNode(oldPosition, [](Node* node) {
        if (node != nullptr)
        {
            node->SetContent(nullptr);
        }
        });

    _playerPosition = newPosition;

    if (_player != nullptr)
    {
        _player->SetPosition(_playerPosition);
    }

    UpdatePlayerOnMap();

    currentRoom->GetMap()->SafePickNode(oldPosition, [](Node* node) {
        if (node != nullptr)
        {
            node->DrawContent(Vector2(0, 0));
        }
        });

    currentRoom->GetMap()->SafePickNode(_playerPosition, [](Node* node) {
        if (node != nullptr)
        {
            node->DrawContent(Vector2(0, 0));
        }
        });

    if (_player != nullptr)
    {
        _player->UpdateActionTime();
    }

    _gameMutex.unlock();
}

void Game::ChangeRoom(PortalDir direction)
{
    _gameMutex.lock();

    Room* oldRoom = _dungeonMap->GetActiveRoom();

    int newX = _dungeonMap->GetCurrentX();
    int newY = _dungeonMap->GetCurrentY();

    switch (direction)
    {
    case PortalDir::Left:
        newX--;
        break;
    case PortalDir::Right:
        newX++;
        break;
    case PortalDir::Up:
        newY--;
        break;
    case PortalDir::Down:
        newY++;
        break;
    }

    _gameMutex.unlock();

    // Detener threads
    _spawner->Stop();
    _entityManager->StopEnemyMovement();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    _gameMutex.lock();

    // Desactivar entidades de la sala anterior (NO eliminarlas)
    if (oldRoom != nullptr)
    {
        // Limpiar jugador
        oldRoom->GetMap()->SafePickNode(_playerPosition, [](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(nullptr);
            }
            });

        // Desactivar entidades (las quita del mapa pero las mantiene en memoria)
        oldRoom->DeactivateEntities();

        // Detener movimiento de enemigos de esta sala
        for (Enemy* enemy : oldRoom->GetEnemies())
        {
            if (enemy != nullptr)
            {
                enemy->StopMovement();
            }
        }
    }

    // Cambiar a la nueva sala
    _dungeonMap->SetActiveRoom(newX, newY);
    Room* newRoom = _dungeonMap->GetActiveRoom();

    if (newRoom != nullptr)
    {
        PortalDir oppositeDir = GetOppositeDirection(direction);
        _playerPosition = newRoom->GetSpawnPositionFromPortal(oppositeDir);

        if (_player != nullptr)
        {
            _player->SetPosition(_playerPosition);  // IMPORTANTE
        }

        //Activar entidades de la nueva sala
        newRoom->ActivateEntities();

        // Reactivar movimiento de enemigos de esta sala
        for (Enemy* enemy : newRoom->GetEnemies())
        {
            if (enemy != nullptr)
            {
                enemy->StartMovement();
            }
        }

        UpdatePlayerOnMap();

        _gameMutex.unlock();

        CC::Clear();
        DrawCurrentRoom();

        InitializeCurrentRoom();

        // CAMBIAR ESTO - actualizar la sala en el EntityManager
        _entityManager->SetCurrentRoom(newRoom);
        _entityManager->StartEnemyMovement(
            newRoom,
            [this]() {
                this->_gameMutex.lock();
                Vector2 pos = this->_playerPosition;
                this->_gameMutex.unlock();
                return pos;
            },
            [this](Enemy* enemy) {
                if (enemy != nullptr && this->_player != nullptr)
                {
                    enemy->Attack(this->_player);
                }
            }
        );

        _spawner->Start(newRoom);
    }
    else
    {
        _gameMutex.unlock();
    }
}

void Game::OnMoveUp()
{
    MovePlayer(Vector2(0, -1));
}

void Game::OnMoveDown()
{
    MovePlayer(Vector2(0, 1));
}

void Game::OnMoveLeft()
{
    MovePlayer(Vector2(-1, 0));
}

void Game::OnMoveRight()
{
    MovePlayer(Vector2(1, 0));
}

bool Game::IsPositionOccupied(Vector2 position) //NOTE: THIS WILL BE USED LATER WHEN SPAWNING ENTITIES PERIODICALLY IN THE MIDDLE OF THE RUN
{
    // Verificar si el jugador está en esa posición
    if (_playerPosition.X == position.X && _playerPosition.Y == position.Y)
        return true;

    // Verificar si hay un enemigo en esa posición (delegado al EntityManager)
    return _entityManager->IsPositionOccupiedByEnemy(position);
}

PortalDir Game::GetOppositeDirection(PortalDir dir)
{
    switch (dir)
    {
    case PortalDir::Left:
        return PortalDir::Right;
    case PortalDir::Right:
        return PortalDir::Left;
    case PortalDir::Up:
        return PortalDir::Down;
    case PortalDir::Down:
        return PortalDir::Up;
    }
    return PortalDir::Left;
}
