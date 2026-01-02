#include "Game.h"
#include "Wall.h"
#include "../Utils/ConsoleControl.h"
#include <iostream>

Game::Game()
{
    _dungeonMap = new DungeonMap(3,3);
    _inputSystem = new InputSystem();
    _entityManager = new EntityManager();
    _spawner = new Spawner(_entityManager, 5);
    _player = nullptr;
    _playerPosition = Vector2(1, 1);
    _currentRoomIndex = 0;
    _running = false;
}

Game::~Game()
{
    Stop();

    if (_player != nullptr)
        delete _player;

    delete _spawner;
    delete _entityManager;
    delete _inputSystem;
    delete _dungeonMap;
}


void Game::InitializeDungeon()
{
    // Crear un mundo de 3x3 salas
    //_dungeonMap = new DungeonMap(3, 3);

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

    // Empezar en el centro (1, 1)
    _dungeonMap->SetActiveRoom(1, 1);
    _playerPosition = Vector2(1, 1);

    // Crear el jugador
    _player = new Player(_playerPosition);

    // Colocar al jugador en el mapa
    UpdatePlayerOnMap();
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
    InitializeDungeon();

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

    // Iniciar el sistema de input en un thread separado
    _inputSystem->StartListen();

    // Iniciar movimiento de enemigos con callback para obtener la posición del jugador
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    _entityManager->StartEnemyMovement(
        currentRoom,
        // Callback 1: Obtener posición del jugador
        [this]() {
            this->_gameMutex.lock();
            Vector2 pos = this->_playerPosition;
            this->_gameMutex.unlock();
            return pos;
        },
        // Callback 2: Cuando un enemigo ataca al jugador
        [this](Enemy* enemy) {
            if (enemy != nullptr && this->_player != nullptr)
            {
                enemy->Attack(this->_player);
            }
        }
    );

    _spawner->Start(currentRoom);

    std::cout << "\n\nUsa las flechas o WASD para moverte." << std::endl;
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
    _inputSystem->StopListen();
    _entityManager->StopEnemyMovement();
    _spawner->Stop();

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

    // NUEVO: Verificar si hay un portal en la nueva posición
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
        // IMPORTANTE: Guardar la dirección del portal y desbloquear ANTES de cambiar de sala
        PortalDir portalDirection = portalPtr->GetDirection();
        _player->UpdateActionTime();

        _gameMutex.unlock(); // DESBLOQUEAR AQUÍ

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

    Enemy* enemyAtPosition = _entityManager->GetEnemyAtPosition(newPosition);
    if (enemyAtPosition != nullptr)
    {
        _player->Attack(enemyAtPosition);
        _player->UpdateActionTime();
        _entityManager->CleanupDeadEnemies(currentRoom);
        _gameMutex.unlock();
        return;
    }

    Chest* chestAtPosition = _entityManager->GetChestAtPosition(newPosition);
    if (chestAtPosition != nullptr)
    {
        _player->Attack(chestAtPosition);
        _player->UpdateActionTime();
        _entityManager->CleanupBrokenChests(currentRoom);
        _gameMutex.unlock();
        return;
    }

    Item* itemAtPosition = _entityManager->GetItemAtPosition(newPosition);
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

    // Guardar la sala actual antes de cambiar
    Room* oldRoom = _dungeonMap->GetActiveRoom();

    // Calcular nueva posición en el mundo
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

    // IMPORTANTE: Detener TODOS los threads ANTES de limpiar
    _spawner->Stop();
    _entityManager->StopEnemyMovement();

    // Esperar un poco para asegurar que los threads terminaron
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    _gameMutex.lock();

    // Limpiar jugador de la sala actual
    if (oldRoom != nullptr)
    {
        oldRoom->GetMap()->SafePickNode(_playerPosition, [](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(nullptr);
            }
            });
    }

    // CRÍTICO: Limpiar TODAS las entidades de la sala anterior del mapa
    // Esto previene enemigos fantasma
    if (oldRoom != nullptr)
    {
        // Obtener todas las entidades antes de limpiarlas
        std::vector<Enemy*> enemies = _entityManager->GetEnemies();

        // Limpiar enemigos del mapa
        for (Enemy* enemy : enemies)
        {
            Vector2 enemyPos = enemy->GetPosition();
            oldRoom->GetMap()->SafePickNode(enemyPos, [](Node* node) {
                if (node != nullptr)
                {
                    node->SetContent(nullptr);
                }
                });
        }

        // Limpiar cofres del mapa
        for (int x = 0; x < oldRoom->GetSize().X; x++)
        {
            for (int y = 0; y < oldRoom->GetSize().Y; y++)
            {
                Vector2 pos(x, y);
                oldRoom->GetMap()->SafePickNode(pos, [](Node* node) {
                    if (node != nullptr)
                    {
                        Chest* chest = node->GetContent<Chest>();
                        if (chest != nullptr)
                        {
                            node->SetContent(nullptr);
                        }
                    }
                    });
            }
        }

        // Limpiar items del mapa
        for (int x = 0; x < oldRoom->GetSize().X; x++)
        {
            for (int y = 0; y < oldRoom->GetSize().Y; y++)
            {
                Vector2 pos(x, y);
                oldRoom->GetMap()->SafePickNode(pos, [](Node* node) {
                    if (node != nullptr)
                    {
                        Item* item = node->GetContent<Item>();
                        if (item != nullptr)
                        {
                            node->SetContent(nullptr);
                        }
                    }
                    });
            }
        }
    }

    // NUEVO: Limpiar todas las entidades del EntityManager
    // Esto elimina los punteros y libera memoria
    _entityManager->ClearAllEntities();

    // Cambiar a la nueva sala
    _dungeonMap->SetActiveRoom(newX, newY);
    Room* newRoom = _dungeonMap->GetActiveRoom();

    if (newRoom != nullptr)
    {
        // Obtener posición de spawn en la nueva sala
        PortalDir oppositeDir = GetOppositeDirection(direction);
        _playerPosition = newRoom->GetSpawnPositionFromPortal(oppositeDir);

        // Colocar jugador en la nueva sala
        UpdatePlayerOnMap();

        _gameMutex.unlock();

        // Redibujar toda la sala
        CC::Clear();
        DrawCurrentRoom();

        // Reiniciar enemigos y spawner en la nueva sala
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