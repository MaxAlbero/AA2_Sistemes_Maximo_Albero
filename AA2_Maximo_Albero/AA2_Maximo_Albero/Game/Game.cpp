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

    // Only place on map, DO NOT start threads
    room->ActivateEntities();
}

void Game::StartRoomEnemyThreads(Room* room)
{
    if (room == nullptr)
        return;

    // Configure callbacks for all enemies in the room
    _entityManager->ConfigureRoomEnemies(room);

    // Start individual threads for each enemy
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

    // STEP 1: Stop all enemy threads
    for (Enemy* enemy : room->GetEnemies())
    {
        if (enemy != nullptr)
            enemy->StopMovement();
    }

    // STEP 2: Wait a moment to ensure all have finished
    // This prevents race conditions where a thread is still drawing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // STEP 3: Remove all entities from the map
    room->DeactivateEntities();
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
        std::cout << " Error loading game. Starting new game..." << std::endl;
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

// GAME START

void Game::Start()
{
    _gameMutex.lock();

    if (_running)
    {
        _gameMutex.unlock();
        return;
    }

    _running = true;

    // Create the world
    CreateWorldRooms(Vector2(20, 10));

    // Try to load saved game
    bool loadedGame = LoadSavedGame();

    // If not loaded, create new game
    if (!loadedGame)
        StartNewGame();

    _ui->SetMapSize(Vector2(20, 10));
    _messages->Start();

    // Configure input
    SetupInputListeners();

    _gameMutex.unlock();

    // Draw interface
    CC::Clear();
    DrawCurrentRoom();
    _ui->Start(_player);
    _inputSystem->StartListen();

    // Configure global EntityManager callbacks
    Room* currentRoom = _dungeonMap->GetActiveRoom();
    _entityManager->SetCurrentRoom(currentRoom);
    _entityManager->SetupEnemyCallbacks(
        GetPlayerPositionCallback(),
        GetEnemyAttackCallback()
    );

    if (!loadedGame)
    {
        InitializeCurrentRoom();
    }
    else
    {
        ActivateRoomEntities(currentRoom);
        StartRoomEnemyThreads(currentRoom); // Start threads AFTER drawing
    }

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

    // Stop all systems
    _saveManager->StopAutoSave();
    _inputSystem->StopListen();

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

        // Check obstacles
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

    // Clear previous position
    currentRoom->GetMap()->SafePickNode(oldPosition, [](Node* node) {
        if (node != nullptr)
        {
            node->SetContent(nullptr);
        }
        });

    // Update position
    _playerPosition = newPosition;

    if (_player != nullptr)
    {
        _player->SetPosition(_playerPosition);
    }

    UpdatePlayerOnMap();

    // Redraw
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

    // Try to use portal
    if (TryUsePortal(newPosition))
        return; // Mutex already unlocked in TryUsePortal

    // Try ranged attack
    int attackRange = _player->GetAttackRange();
    if (TryAttackInRange(direction, attackRange))
    {
        _gameMutex.unlock();
        return;
    }

    // Try melee attack
    if (TryAttackAtPosition(newPosition))
    {
        _gameMutex.unlock();
        return;
    }

    // Pick up item if there is one
    TryPickupItem(newPosition);

    // Move player
    MovePlayerTo(newPosition);

    _gameMutex.unlock();
}

void Game::ChangeRoom(PortalDir direction)
{
    // PHASE 1: PREPARATION (without locks)

    // Calculate new position
    int newX, newY;

    _gameMutex.lock();
    Room* oldRoom = _dungeonMap->GetActiveRoom();
    newX = _dungeonMap->GetCurrentX();
    newY = _dungeonMap->GetCurrentY();
    _gameMutex.unlock();

    switch (direction)
    {
    case PortalDir::Left:   newX--; break;
    case PortalDir::Right:  newX++; break;
    case PortalDir::Up:     newY--; break;
    case PortalDir::Down:   newY++; break;
    }

    // PHASE 2: STOP ALL SYSTEMS
    // CRITICAL: Stop spawner first
    _spawner->Stop();

    // PHASE 3: STOP ENEMY THREADS FROM CURRENT ROOM
    // This MUST be done BEFORE touching the map
    if (oldRoom != nullptr)
    {
        for (Enemy* enemy : oldRoom->GetEnemies())
        {
            if (enemy != nullptr)
                enemy->StopMovement();
        }
    }

    // Wait for all threads to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // PHASE 4: MODIFY THE MAP (with lock)
    _gameMutex.lock();

    if (oldRoom != nullptr)
    {
        // Remove player from map
        oldRoom->GetMap()->SafePickNode(_playerPosition, [](Node* node) {
            if (node != nullptr)
                node->SetContent(nullptr);
            });

        // Deactivate entities (no threads running anymore)
        oldRoom->DeactivateEntities();
    }

    // Switch to new room
    _dungeonMap->SetActiveRoom(newX, newY);
    Room* newRoom = _dungeonMap->GetActiveRoom();

    if (newRoom == nullptr)
    {
        _gameMutex.unlock();
        return;
    }

    // Calculate player position
    PortalDir oppositeDir = GetOppositeDirection(direction);
    _playerPosition = newRoom->GetSpawnPositionFromPortal(oppositeDir);

    if (_player != nullptr)
    {
        _player->SetPosition(_playerPosition);
    }

    // Activate entities on map (without threads yet)
    ActivateRoomEntities(newRoom);
    UpdatePlayerOnMap();

    _gameMutex.unlock();

    CC::Clear();
    DrawCurrentRoom();

    InitializeCurrentRoom();

    _entityManager->SetCurrentRoom(newRoom);

    StartRoomEnemyThreads(newRoom);
    _spawner->Start(newRoom);
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
    _gameMutex.lock();

    if (_player == nullptr || _player->IsAlive() || _gameOver)
    {
        _gameMutex.unlock();
        return;
    }

    _gameOver = true;
    _gameMutex.unlock();

    if (_messages != nullptr)
        _messages->PushMessage("GAME OVER - El jugador ha muerto", 5);
}