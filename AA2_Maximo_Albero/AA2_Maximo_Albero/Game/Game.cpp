#include "Game.h"
#include "Wall.h"
#include "../Utils/ConsoleControl.h"
#include <iostream>

Game::Game()
{
    _dungeonMap = new DungeonMap();
    _inputSystem = new InputSystem();
    _entityManager = new EntityManager();
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

    delete _entityManager;
    delete _inputSystem;
    delete _dungeonMap;
}


void Game::InitializeDungeon()
{
    // Crear una sala de ejemplo
    Room* room1 = new Room(Vector2(20, 10), Vector2(0, 0));
    _dungeonMap->AddRoom(room1);
    _dungeonMap->SetActiveRoom(0);

    // Crear el jugador
    _player = new Player(_playerPosition);

    // Colocar al jugador en el mapa
    UpdatePlayerOnMap();

    // Spawner algunos enemigos de prueba usando el EntityManager
    Room* currentRoom = _dungeonMap->GetActiveRoom();

    _entityManager->SpawnEnemy(Vector2(15, 7), currentRoom);
    _entityManager->SpawnEnemy(Vector2(3, 5), currentRoom);
    _entityManager->SpawnEnemy(Vector2(12, 2), currentRoom);

    _entityManager->SpawnChest(Vector2(5, 3), currentRoom);

    //_entityManager->SpawnItem(Vector2(3, 3), ItemType::COIN, currentRoom);
    //_entityManager->SpawnItem(Vector2(10, 4), ItemType::POTION, currentRoom);

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

    // Verificar si hay un enemigo en la posición objetivo
    Enemy* enemyAtPosition = _entityManager->GetEnemyAtPosition(newPosition);

    if (enemyAtPosition != nullptr)
    {
        // HAY UN ENEMIGO - ATACAR en lugar de moverse
        _player->Attack(enemyAtPosition);
        _player->UpdateActionTime();

        // Limpiar enemigos muertos después del ataque
        _entityManager->CleanupDeadEnemies(currentRoom);

        _gameMutex.unlock();
        return; // NO nos movemos, solo atacamos
    }

    Chest* chestAtPosition = _entityManager->GetChestAtPosition(newPosition);

    if (chestAtPosition != nullptr)
    {
        // HAY UN COFRE - ATACAR en lugar de moverse
        _player->Attack(chestAtPosition);
        _player->UpdateActionTime();

        // Limpiar cofres rotos después del ataque
        _entityManager->CleanupBrokenChests(currentRoom);

        _gameMutex.unlock();
        return; // NO nos movemos, solo atacamos
    }

    Item* itemAtPosition = _entityManager->GetItemAtPosition(newPosition);
    if (itemAtPosition != nullptr)
    {
        // Recoger el item según su tipo
        ItemType type = itemAtPosition->GetType();

        switch (type)
        {
        case ItemType::COIN:
            _player->AddCoin();
            //std::cout << "¡Moneda recogida! Total: " << _player->GetCoins() << std::endl;
            break;
        case ItemType::POTION:
            _player->AddPotion();
            //std::cout << "¡Poción recogida! Total: " << _player->GetPotionCount() << std::endl;
            break;
        case ItemType::WEAPON:
            _player->ChangeWeapon();
            //std::cout << "¡Cambio de Arma!" << std::endl;
            break;
        }

        // Eliminar el item del mapa
        _entityManager->RemoveItem(itemAtPosition, currentRoom);

        // No gastamos cooldown al recoger items, seguimos moviendo
    }

    // NO HAY ENEMIGO - MOVERSE
    Vector2 oldPosition = _playerPosition;

    // Limpiar la casilla anterior (dejar vacía)
    currentRoom->GetMap()->SafePickNode(oldPosition, [](Node* node) {
        if (node != nullptr)
        {
            node->SetContent(nullptr);
        }
        });

    // Actualizar posición
    _playerPosition = newPosition;

    // Colocar jugador en nueva posición
    UpdatePlayerOnMap();

    // Redibujar la casilla antigua (ahora vacía)
    currentRoom->GetMap()->SafePickNode(oldPosition, [](Node* node) {
        if (node != nullptr)
        {
            node->DrawContent(Vector2(0, 0));
        }
        });

    // Redibujar la casilla nueva (ahora con jugador)
    currentRoom->GetMap()->SafePickNode(_playerPosition, [](Node* node) {
        if (node != nullptr)
        {
            node->DrawContent(Vector2(0, 0));
        }
        });

    // Actualizar el tiempo de la última acción del jugador
    if (_player != nullptr)
    {
        _player->UpdateActionTime();
    }

    _gameMutex.unlock();
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