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

    _saveManager = new SaveManager("savegame.json", 5); // Autoguardado cada 5 segundos
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
        _player = new Player(_playerPosition, _messages);

        if (_saveManager->LoadGame(_dungeonMap, _player, _entityManager))
        {
            // Después de LoadGame, el DungeonMap ya tiene la sala correcta establecida
            // Ahora obtén la posición del jugador
            _playerPosition = _player->GetPosition();

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
        _player = new Player(_playerPosition, _messages);

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

    _messages->Start();

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
            
            if (this->_gameOver || this->_player == nullptr)
            {
                this->_gameMutex.unlock();
                return Vector2(-1000, -1000);  // Posición imposible
            }

            Vector2 pos = this->_playerPosition;
            this->_gameMutex.unlock();
            return pos;
        },
        [this](Enemy* enemy) {
            if (this->_gameOver)
                return;

            if (this->_player == nullptr || !this->_player->IsAlive())
                return;

            if (enemy != nullptr)
            {
                enemy->Attack(this->_player);

                // Verificar muerte DESPUÉS del ataque
                if (this->_player != nullptr && !this->_player->IsAlive())
                {
                    this->CheckPlayerDeath();
                }
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
    _gameMutex.unlock();

    // Detener autoguardado PRIMERO
    _saveManager->StopAutoSave();

    // Detener sistema de input
    _inputSystem->StopListen();

    // Detener movimiento de enemigos
    _entityManager->StopEnemyMovement();

    // Detener spawner
    _spawner->Stop();

    // NUEVO: Detener sistema de mensajes si tiene threads
    if (_messages != nullptr)
    {
        _messages->Stop(); // Asegúrate de implementar esto si MessageSystem usa threads
    }

    // Detener UI
    _ui->Stop();

    // Pausa para asegurar que todos los threads terminaron
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
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

    if (_gameOver || !_running)
    {
        _gameMutex.unlock();
        return;
    }

    if (_player == nullptr || !_player->IsAlive())
    {
        _gameMutex.unlock();
        return;
    }

    // SISTEMA DE COOLDOWN: Verificar si el jugador puede realizar una acción
    // Evita spam de movimiento/ataque
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

    // ===== DETECCIÓN DE PORTALES =====
    // Si pisamos un portal, cambiar de sala sin procesar el resto
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
        // IMPORTANTE: Guardar dirección y desbloquear ANTES de cambiar sala
        // ChangeRoom() necesita adquirir el mutex, evitamos deadlock
        PortalDir portalDirection = portalPtr->GetDirection();
        _player->UpdateActionTime();
        _gameMutex.unlock();
        ChangeRoom(portalDirection);
        return;
    }

    // ===== SISTEMA DE ATAQUE CON RANGO =====
    // Permite atacar a distancia según el arma equipada
    // Espada = rango 1, Lanza = rango 2
    int attackRange = _player->GetAttackRange();
    bool attacked = false;

    // Iterar desde rango 1 hasta el máximo del arma
    for (int range = 1; range <= attackRange && !attacked; range++)
    {
        Vector2 targetPosition = Vector2(
            _playerPosition.X + (direction.X * range),
            _playerPosition.Y + (direction.Y * range)
        );

        // VERIFICACIÓN DE OBSTÁCULOS: La lanza NO atraviesa paredes
        // Comprobar cada casilla intermedia hasta el objetivo
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

        // Intentar atacar enemigo en esta posición
        if (_entityManager->TryAttackEnemyAt(targetPosition, _player, currentRoom))
        {
            _player->UpdateActionTime();
            attacked = true;
            break;
        }

        // Intentar atacar cofre en esta posición
        if (_entityManager->TryAttackChestAt(targetPosition, _player, currentRoom))
        {
            _player->UpdateActionTime();
            attacked = true;
            break;
        }
    }

    // Si se atacó algo, no moverse
    if (attacked)
    {
        _gameMutex.unlock();
        return;
    }

    // ===== ATAQUE CUERPO A CUERPO (Rango 1) =====
    // Verificar si hay enemigo/cofre en la casilla a la que intentamos movernos
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

    // ===== RECOLECCIÓN DE ITEMS =====
    // Si hay un item en la nueva posición, recogerlo automáticamente
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

    // ===== MOVIMIENTO REAL DEL JUGADOR =====
    Vector2 oldPosition = _playerPosition;

    // Limpiar posición anterior en el mapa
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

    // Colocar jugador en nueva posición
    UpdatePlayerOnMap();

    // REDIBUJAR: Solo las casillas afectadas (vieja y nueva)
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

    // Actualizar tiempo de última acción (cooldown)
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

    // Calcular coordenadas de la nueva sala según la dirección del portal
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

    // ===== FASE 1: DETENER THREADS =====
    // CRÍTICO: Detener antes de modificar estado de entidades
    // Evita race conditions con enemigos moviéndose durante el cambio
    _spawner->Stop();
    _entityManager->StopEnemyMovement();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Asegurar que los threads terminaron

    _gameMutex.lock();

    // ===== FASE 2: DESACTIVAR SALA ANTERIOR =====
    // Las entidades NO se eliminan, solo se quitan del mapa visual
    // Esto permite que sigan existiendo cuando volvamos a esta sala
    if (oldRoom != nullptr)
    {
        // Limpiar jugador del mapa visual
        oldRoom->GetMap()->SafePickNode(_playerPosition, [](Node* node) {
            if (node != nullptr)
            {
                node->SetContent(nullptr);
            }
            });

        // Desactivar entidades (quita de mapa pero mantiene en memoria)
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

    // ===== FASE 3: CAMBIAR A NUEVA SALA =====
    _dungeonMap->SetActiveRoom(newX, newY);
    Room* newRoom = _dungeonMap->GetActiveRoom();

    if (newRoom != nullptr)
    {
        // Calcular spawn position: lado opuesto al portal por el que entramos
        PortalDir oppositeDir = GetOppositeDirection(direction);
        _playerPosition = newRoom->GetSpawnPositionFromPortal(oppositeDir);

        // IMPORTANTE: Actualizar posición en el objeto Player
        if (_player != nullptr)
        {
            _player->SetPosition(_playerPosition);
        }

        // ===== FASE 4: ACTIVAR ENTIDADES DE NUEVA SALA =====
        // Coloca todas las entidades guardadas de vuelta en el mapa visual
        newRoom->ActivateEntities();

        // Reactivar movimiento de enemigos de esta sala
        for (Enemy* enemy : newRoom->GetEnemies())
        {
            if (enemy != nullptr)
            {
                enemy->StartMovement();
            }
        }

        // Colocar jugador en el mapa
        UpdatePlayerOnMap();

        _gameMutex.unlock();

        // Limpiar pantalla completa y redibujar (única vez que se hace Clear)
        CC::Clear();
        DrawCurrentRoom();

        // Inicializar sala si es primera vez (spawn inicial de enemigos/cofres)
        InitializeCurrentRoom();

        // ===== FASE 5: REINICIAR THREADS CON NUEVA SALA =====
        // Actualizar referencia de sala en EntityManager
        _entityManager->SetCurrentRoom(newRoom);

        // Reiniciar movimiento de enemigos con callbacks actualizados
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

        // Reiniciar spawner con la nueva sala
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

void Game::CheckPlayerDeath()
{
    if (_player == nullptr)
        return;

    // Verificar si el jugador está muerto
    if (!_player->IsAlive())
    {
        _gameMutex.lock();

        // Evitar múltiples game overs
        if (!_gameOver)
        {
            _gameOver = true;
            _gameMutex.unlock(); // Desbloquear ANTES de operaciones pesadas

            // Mostrar mensaje de Game Over
            if (_messages != nullptr)
            {
                _messages->PushMessage("GAME OVER - El jugador ha muerto", 5);
            }
        }
        else
        {
            _gameMutex.unlock();
        }
    }
}