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
    _gameOver = false;
    _messages = new MessageSystem();

    _saveManager = new SaveManager("savegame.json", 5);
}

Game::~Game()
{
    Stop();

    if (_player != nullptr)
        delete _player;

    delete _saveManager;
    delete _messages;
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

    _entityManager->InitializeRoomEntities(currentRoom, currentX, currentY);
}

void Game::CreateWorldRooms(Vector2 roomSize)
{
    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            Room* room = new Room(roomSize, Vector2(0, 0));
            room->GeneratePortals(x, y, 3, 3);
            _dungeonMap->SetRoom(x, y, room);
        }
    }
}

void Game::ActivateRoomEntities(Room* room)
{
    if (room == nullptr)
        return;

    room->ActivateEntities();

    for (Enemy* enemy : room->GetEnemies())
    {
        if (enemy != nullptr)
            enemy->StartMovement();
    }
}

void Game::DeactivateRoomEntities(Room* room)
{
    if (room == nullptr)
        return;

    room->DeactivateEntities();

    for (Enemy* enemy : room->GetEnemies())
    {
        if (enemy != nullptr)
            enemy->StopMovement();
    }
}

void Game::PlacePlayerOnMap(Vector2 position)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom == nullptr || _player == nullptr)
        return;

    currentRoom->GetMap()->SafePickNode(position, [this](Node* node) {
        if (node != nullptr)
            node->SetContent(this->_player);
        });
}

void Game::SetupInputListeners()
{
    _inputSystem->AddListener(K_W, [this]() { this->OnMoveUp(); });
    _inputSystem->AddListener(K_S, [this]() { this->OnMoveDown(); });
    _inputSystem->AddListener(K_A, [this]() { this->OnMoveLeft(); });
    _inputSystem->AddListener(K_D, [this]() { this->OnMoveRight(); });
    _inputSystem->AddListener(K_SPACE, [this]() {
        if (this->_player != nullptr)
            this->_player->UsePotion();
        });
}

std::function<Vector2()> Game::GetPlayerPositionCallback()
{
    return [this]() {
        this->_gameMutex.lock();

        if (this->_gameOver || this->_player == nullptr)
        {
            this->_gameMutex.unlock();
            return Vector2(-1000, -1000);
        }

        Vector2 pos = this->_playerPosition;
        this->_gameMutex.unlock();
        return pos;
        };
}

std::function<void(Enemy*)> Game::GetEnemyAttackCallback()
{
    return [this](Enemy* enemy) {
        if (this->_gameOver)
            return;

        if (this->_player == nullptr || !this->_player->IsAlive())
            return;

        if (enemy != nullptr)
        {
            enemy->Attack(this->_player);

            if (this->_player != nullptr && !this->_player->IsAlive())
                this->CheckPlayerDeath();
        }
        };
}

bool Game::LoadSavedGame()
{
    if (!_saveManager->SaveFileExists())
        return false;

    _player = new Player(_playerPosition, _messages);

    if (_saveManager->LoadGame(_dungeonMap, _player, _entityManager))
    {
        _playerPosition = _player->GetPosition();
        Room* currentRoom = _dungeonMap->GetActiveRoom();

        if (currentRoom != nullptr)
        {
            ActivateRoomEntities(currentRoom);
            PlacePlayerOnMap(_playerPosition);
        }

        return true;
    }
    else
    {
        std::cout << " Error al cargar la partida. Iniciando nueva partida..." << std::endl;
        delete _player;
        _player = nullptr;
        return false;
    }
}

void Game::StartNewGame()
{
    _dungeonMap->SetActiveRoom(1, 1);
    _playerPosition = Vector2(10, 5);
    _player = new Player(_playerPosition, _messages);

    PlacePlayerOnMap(_playerPosition);
}

// ===== INICIO DEL JUEGO =====

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

    // Crear el mundo
    CreateWorldRooms(Vector2(20, 10));

    // Intentar cargar partida guardada
    bool loadedGame = LoadSavedGame();

    // Si no se cargó, crear nueva partida
    if (!loadedGame)
        StartNewGame();

    _ui->SetMapSize(Vector2(20, 10));
    _messages->Start();

    // Configurar input
    SetupInputListeners();

    _gameMutex.unlock();

    // Dibujar interfaz
    CC::Clear();
    DrawCurrentRoom();
    _ui->Start(_player);
    _inputSystem->StartListen();

    // Iniciar sistemas de entidades
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    _entityManager->StartEnemyMovement(
        currentRoom,
        GetPlayerPositionCallback(),
        GetEnemyAttackCallback()
    );

    if (!loadedGame)
        InitializeCurrentRoom();

    _spawner->Start(currentRoom);
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
    _gameMutex.unlock();

    // Detener todos los sistemas
    _saveManager->StopAutoSave();
    _inputSystem->StopListen();
    _entityManager->StopEnemyMovement();
    _spawner->Stop();

    if (_messages != nullptr)
        _messages->Stop();

    _ui->Stop();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void Game::DrawCurrentRoom()
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom != nullptr)
        currentRoom->Draw();
}

bool Game::CanMoveTo(Vector2 position)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom == nullptr)
        return false;

    bool canMove = false;

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
            node->SetContent(_player);
        });
}

void Game::RedrawPosition(Vector2 position)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    if (currentRoom == nullptr)
        return;

    currentRoom->GetMap()->SafePickNode(position, [](Node* node) {
        if (node != nullptr)
            node->DrawContent(Vector2(0, 0));
        });
}

bool Game::TryUsePortal(Vector2 newPosition)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    Portal* portal = nullptr;

    currentRoom->GetMap()->SafePickNode(newPosition, [&](Node* node) {
        if (node != nullptr)
        {
            portal = node->GetContent<Portal>();
        }
        });

    if (portal != nullptr)
    {
        PortalDir portalDirection = portal->GetDirection();
        _player->UpdateActionTime();
        _gameMutex.unlock();
        ChangeRoom(portalDirection);
        return true;
    }

    return false;
}

bool Game::TryAttackInRange(Vector2 direction, int attackRange)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();

    for (int range = 1; range <= attackRange; range++)
    {
        Vector2 targetPosition = Vector2(
            _playerPosition.X + (direction.X * range),
            _playerPosition.Y + (direction.Y * range)
        );

        // Verificar obstáculos
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

        if (_entityManager->TryAttackEnemyAt(targetPosition, _player, currentRoom) ||
            _entityManager->TryAttackChestAt(targetPosition, _player, currentRoom))
        {
            _player->UpdateActionTime();
            return true;
        }
    }

    return false;
}

bool Game::TryAttackAtPosition(Vector2 position)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();

    Enemy* enemy = _entityManager->GetEnemyAtPosition(position, currentRoom);
    if (enemy != nullptr)
    {
        _player->Attack(enemy);
        _player->UpdateActionTime();
        _entityManager->CleanupDeadEnemies(currentRoom);
        return true;
    }

    Chest* chest = _entityManager->GetChestAtPosition(position, currentRoom);
    if (chest != nullptr)
    {
        _player->Attack(chest);
        _player->UpdateActionTime();
        _entityManager->CleanupBrokenChests(currentRoom);
        return true;
    }

    return false;
}

void Game::TryPickupItem(Vector2 position)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    Item* item = _entityManager->GetItemAtPosition(position, currentRoom);

    if (item != nullptr)
    {
        ItemType type = item->GetType();
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
        _entityManager->RemoveItem(item, currentRoom);
    }
}

void Game::MovePlayerTo(Vector2 newPosition)
{
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    Vector2 oldPosition = _playerPosition;

    // Limpiar posición anterior
    currentRoom->GetMap()->SafePickNode(oldPosition, [](Node* node) {
        if (node != nullptr)
        {
            node->SetContent(nullptr);
        }
        });

    // Actualizar posición
    _playerPosition = newPosition;

    if (_player != nullptr)
    {
        _player->SetPosition(_playerPosition);
    }

    UpdatePlayerOnMap();

    // Redibujar
    RedrawPosition(oldPosition);
    RedrawPosition(_playerPosition);

    if (_player != nullptr)
    {
        _player->UpdateActionTime();
    }
}

void Game::MovePlayer(Vector2 direction)
{
    _gameMutex.lock();

    if (_gameOver || !_running || _player == nullptr || !_player->IsAlive())
    {
        _gameMutex.unlock();
        return;
    }

    if (!_player->CanPerformAction())
    {
        _gameMutex.unlock();
        return;
    }

    Vector2 newPosition = _playerPosition + direction;

    if (!CanMoveTo(newPosition))
    {
        _gameMutex.unlock();
        return;
    }

    // Intentar usar portal
    if (TryUsePortal(newPosition))
        return; // El mutex ya se desbloqueó en TryUsePortal

    // Intentar ataque a distancia
    int attackRange = _player->GetAttackRange();
    if (TryAttackInRange(direction, attackRange))
    {
        _gameMutex.unlock();
        return;
    }

    // Intentar ataque cuerpo a cuerpo
    if (TryAttackAtPosition(newPosition))
    {
        _gameMutex.unlock();
        return;
    }

    // Recoger item si hay
    TryPickupItem(newPosition);

    // Mover jugador
    MovePlayerTo(newPosition);

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
    case PortalDir::Left:   newX--; break;
    case PortalDir::Right:  newX++; break;
    case PortalDir::Up:     newY--; break;
    case PortalDir::Down:   newY++; break;
    }

    _gameMutex.unlock();

    // Detener threads
    _spawner->Stop();
    _entityManager->StopEnemyMovement();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    _gameMutex.lock();

    // Desactivar sala anterior
    if (oldRoom != nullptr)
    {
        oldRoom->GetMap()->SafePickNode(_playerPosition, [](Node* node) {
            if (node != nullptr)
                node->SetContent(nullptr);
            });

        DeactivateRoomEntities(oldRoom);
    }

    // Activar nueva sala
    _dungeonMap->SetActiveRoom(newX, newY);
    Room* newRoom = _dungeonMap->GetActiveRoom();

    if (newRoom != nullptr)
    {
        PortalDir oppositeDir = GetOppositeDirection(direction);
        _playerPosition = newRoom->GetSpawnPositionFromPortal(oppositeDir);

        if (_player != nullptr)
        {
            _player->SetPosition(_playerPosition);
        }

        ActivateRoomEntities(newRoom);
        UpdatePlayerOnMap();

        _gameMutex.unlock();

        CC::Clear();
        DrawCurrentRoom();
        InitializeCurrentRoom();

        _entityManager->SetCurrentRoom(newRoom);
        _entityManager->StartEnemyMovement(
            newRoom,
            GetPlayerPositionCallback(),
            GetEnemyAttackCallback()
        );

        _spawner->Start(newRoom);
    }
    else
    {
        _gameMutex.unlock();
    }
}

void Game::OnMoveUp() { MovePlayer(Vector2(0, -1)); }
void Game::OnMoveDown() { MovePlayer(Vector2(0, 1)); }
void Game::OnMoveLeft() { MovePlayer(Vector2(-1, 0)); }
void Game::OnMoveRight() { MovePlayer(Vector2(1, 0)); }

bool Game::IsPositionOccupied(Vector2 position)
{
    if (_playerPosition.X == position.X && _playerPosition.Y == position.Y)
        return true;

    return _entityManager->IsPositionOccupiedByEnemy(position);
}

PortalDir Game::GetOppositeDirection(PortalDir dir)
{
    switch (dir)
    {
    case PortalDir::Left:   return PortalDir::Right;
    case PortalDir::Right:  return PortalDir::Left;
    case PortalDir::Up:     return PortalDir::Down;
    case PortalDir::Down:   return PortalDir::Up;
    }
    return PortalDir::Left;
}

void Game::CheckPlayerDeath()
{
    if (_player == nullptr || _player->IsAlive())
        return;

    _gameMutex.lock();

    if (!_gameOver)
    {
        _gameOver = true;
        _gameMutex.unlock();

        if (_messages != nullptr)
            _messages->PushMessage("GAME OVER - El jugador ha muerto", 5);
    }
    else
    {
        _gameMutex.unlock();
    }
}