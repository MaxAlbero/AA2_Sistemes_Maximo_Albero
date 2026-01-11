#include "EntityManager.h"

void EntityManager::SetCurrentRoom(Room* room)
{
    Lock();
    _currentRoom = room;
    Unlock();
}

// ===== MÉTODOS AUXILIARES GENÉRICOS =====

void EntityManager::ClearPositionOnMap(Vector2 position, Room* room)
{
    if (room == nullptr)
        return;

    room->GetMap()->SafePickNode(position, [](Node* node) {
        if (node != nullptr)
        {
            node->SetContent(nullptr);
        }
        });
}

void EntityManager::RedrawPosition(Vector2 position, Room* room)
{
    if (room == nullptr)
        return;

    room->GetMap()->SafePickNode(position, [](Node* node) {
        if (node != nullptr)
        {
            node->DrawContent(Vector2(0, 0));
        }
        });
}

// ===== SPAWN DE ENTIDADES =====

void EntityManager::SpawnEnemy(Vector2 position, Room* room)
{
    if (room == nullptr)
        return;

    Enemy* enemy = new Enemy(position);

    Lock();
    _enemies.push_back(enemy);
    Unlock();

    room->AddEnemy(enemy);
    PlaceEntityOnMap(enemy, position, room);

    // Configurar callbacks antes de iniciar el thread
    enemy->SetMovementCallbacks(
        [this](Enemy* e, Vector2 newPos) { return this->CanEnemyMoveTo(e, newPos); },
        _getPlayerPositionCallback,
        _onEnemyAttackPlayer
    );

    enemy->StartMovement();
}

void EntityManager::SpawnChest(Vector2 position, Room* room)
{
    if (room == nullptr)
        return;

    Chest* chest = new Chest(position);

    Lock();
    _chests.push_back(chest);
    Unlock();

    room->AddChest(chest);
    PlaceEntityOnMap(chest, position, room);
}

void EntityManager::SpawnItem(Vector2 position, ItemType type, Room* room)
{
    if (room == nullptr)
        return;

    Item* item = new Item(position, type);

    Lock();
    _items.push_back(item);
    Unlock();

    room->AddItem(item);
    PlaceEntityOnMap(item, position, room);
}

// ===== CLEANUP DE ENTIDADES =====

void EntityManager::CleanupDeadEnemies(Room* room)
{
    if (room == nullptr)
        return;

    std::vector<Enemy*>& enemies = room->GetEnemies();

    for (auto it = enemies.begin(); it != enemies.end();)
    {
        Enemy* enemy = *it;
        if (!enemy->IsAlive())
        {
            enemy->StopMovement();
            Vector2 enemyPos = enemy->GetPosition();

            // Limpiar del mapa
            ClearPositionOnMap(enemyPos, room);
            RedrawPosition(enemyPos, room);

            // Eliminar de lista global
            Lock();
            auto globalIt = std::find(_enemies.begin(), _enemies.end(), enemy);
            if (globalIt != _enemies.end())
            {
                _enemies.erase(globalIt);
            }
            Unlock();

            // Dropear loot y eliminar
            ItemType loot = SelectLoot();
            delete enemy;
            it = enemies.erase(it);
            DropLoot(enemyPos, loot, room);
        }
        else
        {
            ++it;
        }
    }
}

void EntityManager::CleanupBrokenChests(Room* room)
{
    if (room == nullptr)
        return;

    std::vector<Chest*>& chests = room->GetChests();

    for (auto it = chests.begin(); it != chests.end();)
    {
        Chest* chest = *it;
        if (chest->IsBroken())
        {
            Vector2 chestPos = chest->GetPosition();

            // Limpiar del mapa
            ClearPositionOnMap(chestPos, room);
            RedrawPosition(chestPos, room);

            // Eliminar de lista global
            Lock();
            auto globalIt = std::find(_chests.begin(), _chests.end(), chest);
            if (globalIt != _chests.end())
            {
                _chests.erase(globalIt);
            }
            Unlock();

            // Dropear loot y eliminar
            ItemType loot = SelectLoot();
            delete chest;
            it = chests.erase(it);
            DropLoot(chestPos, loot, room);
        }
        else
        {
            ++it;
        }
    }
}

// ===== GETTERS DE ENTIDADES =====

Enemy* EntityManager::GetEnemyAtPosition(Vector2 position, Room* room)
{
    return GetEntityAtPosition<Enemy>(position, room);
}

Chest* EntityManager::GetChestAtPosition(Vector2 position, Room* room)
{
    return GetEntityAtPosition<Chest>(position, room);
}

Item* EntityManager::GetItemAtPosition(Vector2 position, Room* room)
{
    return GetEntityAtPosition<Item>(position, room);
}

bool EntityManager::IsPositionOccupiedByEnemy(Vector2 position)
{
    return IsPositionOccupiedBy<Enemy>(position);
}

bool EntityManager::IsPositionOccupiedByChest(Vector2 position)
{
    return IsPositionOccupiedBy<Chest>(position);
}

bool EntityManager::IsPositionOccupiedByItem(Vector2 position)
{
    return IsPositionOccupiedBy<Item>(position);
}

std::vector<Enemy*> EntityManager::GetEnemies()
{
    Lock();
    std::vector<Enemy*> enemiesCopy = _enemies;
    Unlock();
    return enemiesCopy;
}

std::vector<Chest*> EntityManager::GetChests()
{
    Lock();
    std::vector<Chest*> chestsCopy = _chests;
    Unlock();
    return chestsCopy;
}

std::vector<Item*> EntityManager::GetItems()
{
    Lock();
    std::vector<Item*> itemsCopy = _items;
    Unlock();
    return itemsCopy;
}

void EntityManager::ClearAllEntities()
{
    Lock();

    for (Enemy* enemy : _enemies)
    {
        enemy->StopMovement();
        delete enemy;
    }
    _enemies.clear();

    for (Chest* chest : _chests)
    {
        delete chest;
    }
    _chests.clear();

    for (Item* item : _items)
    {
        delete item;
    }
    _items.clear();

    Unlock();
}

int EntityManager::GetEnemyCount()
{
    Lock();
    int count = _enemies.size();
    Unlock();
    return count;
}

int EntityManager::GetChestCount()
{
    Lock();
    int count = _chests.size();
    Unlock();
    return count;
}

// ===== SISTEMA DE COMBATE =====

bool EntityManager::TryAttackEnemyAt(Vector2 position, Player* attacker, Room* room)
{
    Lock();

    Enemy* enemy = nullptr;
    for (Enemy* e : _enemies)
    {
        Vector2 enemyPos = e->GetPosition();
        if (enemyPos.X == position.X && enemyPos.Y == position.Y)
        {
            enemy = e;
            break;
        }
    }

    if (enemy != nullptr)
    {
        attacker->Attack(enemy);
        Unlock();
        CleanupDeadEnemies(room);
        return true;
    }

    Unlock();
    return false;
}

bool EntityManager::TryAttackChestAt(Vector2 position, Player* attacker, Room* room)
{
    Lock();

    Chest* chest = nullptr;
    for (Chest* c : _chests)
    {
        Vector2 chestPos = c->GetPosition();
        if (chestPos.X == position.X && chestPos.Y == position.Y)
        {
            chest = c;
            break;
        }
    }

    if (chest != nullptr)
    {
        attacker->Attack(chest);
        Unlock();
        CleanupBrokenChests(room);
        return true;
    }

    Unlock();
    return false;
}

// ===== SISTEMA DE LOOT =====

ItemType EntityManager::SelectLoot()
{
    int lootNum = rand() % 3;

    switch (lootNum)
    {
    case 0: return ItemType::COIN;
    case 1: return ItemType::POTION;
    case 2: return ItemType::WEAPON;
    default: return ItemType::COIN;
    }
}

void EntityManager::DropLoot(Vector2 position, ItemType lootItem, Room* room)
{
    if (room == nullptr)
        return;

    SpawnItem(position, lootItem, room);
}

void EntityManager::RemoveItem(Item* itemToRemove, Room* room)
{
    if (room == nullptr || itemToRemove == nullptr)
        return;

    std::vector<Item*>& items = room->GetItems();

    for (auto it = items.begin(); it != items.end(); ++it)
    {
        if (*it == itemToRemove)
        {
            Vector2 itemPos = itemToRemove->GetPosition();

            // Limpiar del mapa
            ClearPositionOnMap(itemPos, room);
            RedrawPosition(itemPos, room);

            // Eliminar de la lista global
            Lock();
            auto globalIt = std::find(_items.begin(), _items.end(), itemToRemove);
            if (globalIt != _items.end())
            {
                _items.erase(globalIt);
            }
            Unlock();

            // Eliminar de la lista de la sala
            delete itemToRemove;
            items.erase(it);
            break;
        }
    }
}

// ===== INICIALIZACIÓN =====

void EntityManager::InitializeRoomEntities(Room* room, int roomX, int roomY)
{
    if (room == nullptr || room->IsInitialized())
        return;

    int entityCount = 5;

    for (int i = 0; i < entityCount; i++)
    {
        Vector2 spawnPos = FindValidSpawnPosition(room);

        if (spawnPos.X == -1)
            continue;

        int entityType = rand() % 2;

        switch (entityType)
        {
        case 0:
            SpawnEnemy(spawnPos, room);
            break;
        case 1:
            SpawnChest(spawnPos, room);
            break;
        }
    }

    room->SetInitialized(true);
}

void EntityManager::RegisterLoadedEntities(Room* room)
{
    if (room == nullptr)
        return;

    Lock();

    // Registrar enemigos
    for (Enemy* enemy : room->GetEnemies())
    {
        if (enemy != nullptr)
        {
            auto it = std::find(_enemies.begin(), _enemies.end(), enemy);
            if (it == _enemies.end())
            {
                _enemies.push_back(enemy);
            }
        }
    }

    // Registrar cofres
    for (Chest* chest : room->GetChests())
    {
        if (chest != nullptr)
        {
            auto it = std::find(_chests.begin(), _chests.end(), chest);
            if (it == _chests.end())
            {
                _chests.push_back(chest);
            }
        }
    }

    // Registrar items
    for (Item* item : room->GetItems())
    {
        if (item != nullptr)
        {
            auto it = std::find(_items.begin(), _items.end(), item);
            if (it == _items.end())
            {
                _items.push_back(item);
            }
        }
    }

    Unlock();
}

// ===== CONFIGURACIÓN DE CALLBACKS =====

void EntityManager::SetupEnemyCallbacks(std::function<Vector2()> getPlayerPositionCallback,
    std::function<void(Enemy*)> onEnemyAttackPlayer)
{
    Lock();
    _getPlayerPositionCallback = getPlayerPositionCallback;
    _onEnemyAttackPlayer = onEnemyAttackPlayer;
    Unlock();
}

// Configura los callbacks de todos los enemigos activos en una sala
void EntityManager::ConfigureRoomEnemies(Room* room)
{
    if (room == nullptr)
        return;

    for (Enemy* enemy : room->GetEnemies())
    {
        if (enemy != nullptr)
        {
            enemy->SetMovementCallbacks(
                [this](Enemy* e, Vector2 newPos) { return this->CanEnemyMoveTo(e, newPos); },
                _getPlayerPositionCallback,
                _onEnemyAttackPlayer
            );
        }
    }
}

// ===== VALIDACIÓN DE MOVIMIENTO =====

// Valida si un enemigo puede moverse a una posición
// Es llamado desde el thread del enemigo individual
bool EntityManager::CanEnemyMoveTo(Enemy* movingEnemy, Vector2 newPosition)
{
    if (movingEnemy == nullptr)
        return false;

    Lock();
    Room* room = _currentRoom;
    auto getPlayerPos = _getPlayerPositionCallback;
    Unlock();

    if (room == nullptr || !getPlayerPos)
        return false;

    Vector2 currentPosition = movingEnemy->GetPosition();
    Vector2 playerPosition = getPlayerPos();

    // Verificar paredes y portales
    bool canMove = true;
    room->GetMap()->SafePickNode(newPosition, [&](Node* node) {
        if (node != nullptr)
        {
            Wall* wall = node->GetContent<Wall>();
            if (wall != nullptr)
            {
                canMove = false;
            }

            Portal* portal = node->GetContent<Portal>();
            if (portal != nullptr)
            {
                canMove = false;
            }
        }
        });

    if (!canMove)
        return false;

    // No moverse a la posición del jugador
    if (newPosition.X == playerPosition.X && newPosition.Y == playerPosition.Y)
    {
        return false;
    }

    Lock();

    // Verificar colisión con otros enemigos
    for (Enemy* enemy : _enemies)
    {
        if (enemy == movingEnemy)
            continue;

        Vector2 enemyPos = enemy->GetPosition();
        if (enemyPos.X == newPosition.X && enemyPos.Y == newPosition.Y)
        {
            canMove = false;
            break;
        }
    }

    // Verificar colisión con cofres
    if (canMove)
    {
        for (Chest* chest : _chests)
        {
            Vector2 chestPos = chest->GetPosition();
            if (chestPos.X == newPosition.X && chestPos.Y == newPosition.Y)
            {
                canMove = false;
                break;
            }
        }
    }

    // Verificar colisión con items
    if (canMove)
    {
        for (Item* item : _items)
        {
            Vector2 itemPos = item->GetPosition();
            if (itemPos.X == newPosition.X && itemPos.Y == newPosition.Y)
            {
                canMove = false;
                break;
            }
        }
    }

    Unlock();

    // Si puede moverse, actualizar en el mapa
    if (canMove)
    {
        ClearPositionOnMap(currentPosition, room);
        PlaceEntityOnMap(movingEnemy, newPosition, room);
        RedrawPosition(currentPosition, room);
    }

    return canMove;
}

Vector2 EntityManager::FindValidSpawnPosition(Room* room)
{
    if (room == nullptr)
        return Vector2(-1, -1);

    Vector2 roomSize = room->GetSize();

    for (int attempt = 0; attempt < 20; attempt++)
    {
        int randomX = 2 + (rand() % (roomSize.X - 4));
        int randomY = 2 + (rand() % (roomSize.Y - 4));
        Vector2 pos(randomX, randomY);

        bool isValid = true;

        room->GetMap()->SafePickNode(pos, [&](Node* node) {
            if (node != nullptr && node->GetContent() != nullptr)
            {
                isValid = false;
            }
            });

        if (isValid)
            return pos;
    }

    return Vector2(-1, -1);
}