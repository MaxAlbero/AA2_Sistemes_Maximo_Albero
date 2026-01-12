#include "Enemy.h"

void Enemy::Draw(Vector2 pos)
{
    CC::Lock();
    CC::SetColor(CC::DARKRED);
    CC::SetPosition(pos.X, pos.Y);
    std::cout << "E";
    CC::SetColor(CC::WHITE);
    CC::Unlock();
}

Vector2 Enemy::GetPosition() {
    _enemyMutex.lock();
    Vector2 pos = _position;
    _enemyMutex.unlock();
    return pos;
}

void Enemy::SetPosition(Vector2 newPos) {
    _enemyMutex.lock();
    _position = newPos;
    _enemyMutex.unlock();
}

bool Enemy::CanPerformAction()
{
    _enemyMutex.lock();
    auto currentTime = std::chrono::steady_clock::now();
    auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - _lastActionTime
    ).count();
    bool canAct = timeSinceLastAction >= _actionCooldownMs;
    _enemyMutex.unlock();
    return canAct;
}

void Enemy::UpdateActionTime()
{
    _enemyMutex.lock();
    _lastActionTime = std::chrono::steady_clock::now();
    _enemyMutex.unlock();
}

void Enemy::Attack(IDamageable* entity) const {
    if (entity != nullptr)
    {
        entity->ReceiveDamage(_damage);
    }
}

void Enemy::ReceiveDamage(int damageToReceive) {
    _enemyMutex.lock();
    _hp -= damageToReceive;

    if (_hp <= 0)
    {
        _hp = 0;
    }

    _enemyMutex.unlock();
}

bool Enemy::IsAlive() {
    _enemyMutex.lock();
    bool alive = _hp > 0;
    _enemyMutex.unlock();
    return alive;
}

int Enemy::GetHP() {
    _enemyMutex.lock();
    int hp = _hp;
    _enemyMutex.unlock();
    return hp;
}

Vector2 Enemy::GetRandomDirection()
{
    int direction = rand() % 4;

    switch (direction)
    {
    case 0: return Vector2(0, -1);  // Arriba
    case 1: return Vector2(0, 1);   // Abajo
    case 2: return Vector2(-1, 0);  // Izquierda
    case 3: return Vector2(1, 0);   // Derecha
    default: return Vector2(0, 0);
    }
}

// Starts the enemy's individual movement thread
// Each enemy manages its own movement autonomously
void Enemy::StartMovement()
{
    _enemyMutex.lock();

    if (_isActive)
    {
        _enemyMutex.unlock();
        return;
    }

    _shouldStop = false;
    _isActive = true;
    _enemyMutex.unlock();

    // Create thread that MovementLoop() executes
    _movementThread = new std::thread(&Enemy::MovementLoop, this);
}

// Stops the enemy's movement thread
void Enemy::StopMovement()
{
    _enemyMutex.lock();

    if (!_isActive)
    {
        _enemyMutex.unlock();
        return;
    }

    _shouldStop = true;
    _enemyMutex.unlock();

    if (_movementThread != nullptr)
    {
        if (_movementThread->joinable())
            _movementThread->join();

        delete _movementThread;
        _movementThread = nullptr;
    }

    _enemyMutex.lock();
    _isActive = false;
    _enemyMutex.unlock();
}

// Sets the callbacks so the enemy can query the world
void Enemy::SetMovementCallbacks(
    std::function<bool(Enemy*, Vector2)> canMoveCallback,
    std::function<Vector2()> getPlayerPosCallback,
    std::function<void(Enemy*)> onAttackPlayerCallback)
{
    _enemyMutex.lock();
    _canMoveToCallback = canMoveCallback;
    _getPlayerPositionCallback = getPlayerPosCallback;
    _onAttackPlayerCallback = onAttackPlayerCallback;
    _enemyMutex.unlock();
}

Json::Value Enemy::Code() {
    Json::Value json;
    CodeSubClassType<Enemy>(json);
    json["posX"] = _position.X;
    json["posY"] = _position.Y;
    json["hp"] = _hp;
    json["damage"] = _damage;
    return json;
}

void Enemy::Decode(Json::Value json) {
    _position.X = json["posX"].asInt();
    _position.Y = json["posY"].asInt();
    _hp = json["hp"].asInt();
    _damage = json["damage"].asInt();
    _lastActionTime = std::chrono::steady_clock::now();
}

// Enemy individual thread loop
// Each enemy manages its own movement and attack
void Enemy::MovementLoop()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        _enemyMutex.lock();
        bool shouldStop = _shouldStop;
        _enemyMutex.unlock();

        if (shouldStop)
            break;

        // Verify if enemy is alive
        if (!IsAlive())
            continue;

        // Verify cooldown
        if (!CanPerformAction())
            continue;

        // Verify that the callbacks are configured
        _enemyMutex.lock();
        auto canMove = _canMoveToCallback;
        auto getPlayerPos = _getPlayerPositionCallback;
        auto onAttackPlayer = _onAttackPlayerCallback;
        _enemyMutex.unlock();

        if (!canMove || !getPlayerPos || !onAttackPlayer)
            continue;

        // Get player position
        Vector2 playerPos = getPlayerPos();

        // Verify that the player exists (no está en -1000, -1000)
        if (playerPos.X == -1000 && playerPos.Y == -1000)
            continue;

        Vector2 currentPos = GetPosition();

        // Check if the enemy is adjacent to the player to attack
        int distX = abs(currentPos.X - playerPos.X);
        int distY = abs(currentPos.Y - playerPos.Y);
        bool isAdjacent = (distX + distY) == 1;

        if (isAdjacent)
        {
            // attack the player
            onAttackPlayer(this);
            UpdateActionTime();
            continue;
        }

        // Attempt to move
        Vector2 direction = GetRandomDirection();
        Vector2 newPos = currentPos + direction;

        // Ask if the enemy can move to the next position
        if (canMove(this, newPos))
        {
            SetPosition(newPos);
            UpdateActionTime();
        }
    }
}