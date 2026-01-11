#include "EntityManager.h"

EntityManager::EntityManager()
    : _enemyMovementThread(nullptr), _movementActive(false),
    _isStopping(false), _currentRoom(nullptr)
{
}

EntityManager::~EntityManager()
{
    StopEnemyMovement();
    ClearAllEntities();
}

void EntityManager::SetCurrentRoom(Room* room)
{
    std::lock_guard<std::mutex> lock(_managerMutex);
    _currentRoom = room;
}

// ======================
// SPAWNS
// ======================

void EntityManager::SpawnEnemy(Vector2 position, Room* room)
{
    if (!room) return;

    Enemy* enemy = new Enemy(position);

    _enemies.push_back(enemy);
    room->AddEnemy(enemy);

    PlaceContent(room, enemy, position);
    enemy->StartMovement();
}

void EntityManager::SpawnChest(Vector2 position, Room* room)
{
    if (!room) return;

    Chest* chest = new Chest(position);

    _chests.push_back(chest);
    room->AddChest(chest);

    PlaceContent(room, chest, position);
}

void EntityManager::SpawnItem(Vector2 position, ItemType type, Room* room)
{
    if (!room) return;

    Item* item = new Item(position, type);

    _items.push_back(item);
    room->AddItem(item);

    PlaceContent(room, item, position);
}

// ======================
// ATAQUE UNIFICADO
// ======================

bool EntityManager::TryAttackAt(Vector2 position, Player* attacker, Room* room)
{
    if (!room || !attacker) return false;

    // Intentar atacar enemigo
    if (Enemy* enemy = room->GetEnemyAt(position))
    {
        attacker->Attack(enemy);

        if (!enemy->IsAlive())
            HandleEnemyDeath(enemy, room);

        return true;
    }

    // Intentar atacar cofre
    if (Chest* chest = room->GetChestAt(position))
    {
        attacker->Attack(chest);

        if (chest->IsBroken())
            HandleChestDeath(chest, room);

        return true;
    }

    return false;
}

// ======================
// ELIMINACIONES
// ======================

void EntityManager::HandleEnemyDeath(Enemy* enemy, Room* room)
{
    Vector2 pos = enemy->GetPosition();

    ClearMapPosition(room, pos);
    room->RemoveEnemy(enemy);

    _enemies.erase(std::remove(_enemies.begin(), _enemies.end(), enemy), _enemies.end());
    delete enemy;

    DropLoot(pos, SelectLoot(), room);
}

void EntityManager::HandleChestDeath(Chest* chest, Room* room)
{
    Vector2 pos = chest->GetPosition();

    ClearMapPosition(room, pos);
    room->RemoveChest(chest);

    _chests.erase(std::remove(_chests.begin(), _chests.end(), chest), _chests.end());
    delete chest;

    DropLoot(pos, SelectLoot(), room);
}

// ======================
// MOVIMIENTO ENEMIGOS
// ======================

void EntityManager::StartEnemyMovement(Room* room,
    std::function<Vector2()> getPlayerPositionCallback,
    std::function<void(Enemy*)> onEnemyAttackPlayer)
{
    if (_movementActive || _isStopping) return;

    _currentRoom = room;
    _getPlayerPositionCallback = getPlayerPositionCallback;
    _onEnemyAttackPlayer = onEnemyAttackPlayer;

    _movementActive = true;
    _enemyMovementThread = new std::thread(&EntityManager::EnemyMovementLoop, this);
}

void EntityManager::StopEnemyMovement()
{
    if (!_movementActive) return;

    _movementActive = false;

    if (_enemyMovementThread && _enemyMovementThread->joinable())
        _enemyMovementThread->join();

    delete _enemyMovementThread;
    _enemyMovementThread = nullptr;
}

// Loop principal que gestiona el movimiento de TODOS los enemigos activos
void EntityManager::EnemyMovementLoop()
{
    while (_movementActive)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!_currentRoom) continue;

        Vector2 playerPos = _getPlayerPositionCallback();
        auto enemies = _currentRoom->GetEnemies();

        for (Enemy* enemy : enemies)
        {
            if (enemy && enemy->IsAlive())
                MoveEnemy(enemy, _currentRoom, playerPos);
        }
    }
}

void EntityManager::MoveEnemy(Enemy* enemy, Room* room, Vector2 playerPos)
{
    if (!enemy->CanPerformAction()) return;

    Vector2 current = enemy->GetPosition();

    if (IsAdjacent(current, playerPos))
    {
        _onEnemyAttackPlayer(enemy);
        enemy->UpdateActionTime();
        return;
    }

    Vector2 newPos = current + enemy->GetRandomDirection();

    if (IsPositionBlocked(newPos, room))
        return;

    ClearMapPosition(room, current);
    enemy->SetPosition(newPos);
    PlaceContent(room, enemy, newPos);

    enemy->UpdateActionTime();
}

// ======================
// COLISIONES
// ======================

bool EntityManager::IsPositionBlocked(Vector2 position, Room* room)
{
    bool blocked = false;

    room->GetMap()->SafePickNode(position, [&](Node* node) {
        if (!node) return;

        if (node->GetContent<Wall>() || node->GetContent<Portal>())
            blocked = true;
        });

    if (blocked) return true;
    if (room->GetEnemyAt(position)) return true;
    if (room->GetChestAt(position)) return true;
    if (room->GetItemAt(position)) return true;

    return false;
}

// ======================
// MAPA
// ======================

void EntityManager::ClearMapPosition(Room* room, Vector2 pos)
{
    room->GetMap()->SafePickNode(pos, [](Node* n) {
        if (n) n->SetContent(nullptr);
        });

    room->GetMap()->SafePickNode(pos, [](Node* n) {
        if (n) n->DrawContent(Vector2(0, 0));
        });
}

void EntityManager::PlaceContent(Room* room, INodeContent* content, Vector2 pos)
{
    room->GetMap()->SafePickNode(pos, [content](Node* n) {
        if (n) n->SetContent(content);
        });

    room->GetMap()->SafePickNode(pos, [](Node* n) {
        if (n) n->DrawContent(Vector2(0, 0));
        });
}

// ======================
// UTILIDADES
// ======================

bool EntityManager::IsAdjacent(Vector2 a, Vector2 b)
{
    return abs(a.X - b.X) + abs(a.Y - b.Y) == 1;
}

ItemType EntityManager::SelectLoot()
{
    return static_cast<ItemType>(rand() % 3);
}

void EntityManager::DropLoot(Vector2 position, ItemType lootItem, Room* room)
{
    SpawnItem(position, lootItem, room);
}

void EntityManager::RegisterLoadedEntities(Room* room)
{
    for (Enemy* e : room->GetEnemies())
        if (std::find(_enemies.begin(), _enemies.end(), e) == _enemies.end())
            _enemies.push_back(e);

    for (Chest* c : room->GetChests())
        if (std::find(_chests.begin(), _chests.end(), c) == _chests.end())
            _chests.push_back(c);

    for (Item* i : room->GetItems())
        if (std::find(_items.begin(), _items.end(), i) == _items.end())
            _items.push_back(i);
}

void EntityManager::ClearAllEntities()
{
    for (Enemy* e : _enemies) delete e;
    for (Chest* c : _chests) delete c;
    for (Item* i : _items) delete i;

    _enemies.clear();
    _chests.clear();
    _items.clear();
}
