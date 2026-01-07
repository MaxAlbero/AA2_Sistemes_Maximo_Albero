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

// Verifica si el enemigo puede realizar una acción(moverse o atacar)
// Mismo sistema de cooldown que utiliza player
// Cada enemigo tiene su propio _actionCooldownMs independiente
// USO: En EntityManager::EnemyMovementLoop() antes de mover cada enemigo
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


// Actualiza timestamp de última acción del enemigo, después de moverse o atacar
// Activa cooldown individual del enemigo
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

// Genera una dirección aleatoria para movimiento de IA básica
// DIRECCIONES: Arriba, Abajo, Izquierda, Derecha (no diagonales)
// USO: En EntityManager::EnemyMovementLoop() para decidir hacia dónde moverse
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

// Inicia el thread de movimiento individual del enemigo
// THREAD: Cada enemigo tiene su propio thread (_movementThread)
// IMPORTANTE: El thread se crea pero el movimiento real lo gestiona EntityManager
// PROPÓSITO: Mantener el enemigo "activo" y listo para moverse
void Enemy::StartMovement()
{
    // Evitar crear múltiples threads
    if (_isActive)
        return;

    _shouldStop = false;
    _isActive = true;

    // Crear thread que ejecuta MovementLoop()
    _movementThread = new std::thread(&Enemy::MovementLoop, this);
}

// Detiene el thread de movimiento del enemigo
// CUÁNDO: 1.Enemigo muere, 2.Jugador sale de la sala (Game::ChangeRoom) o 3.Juego termina
// THREAD-SAFETY: join() espera a que el thread termine antes de eliminarlo
void Enemy::StopMovement()
{
    if (!_isActive)
        return;

    _shouldStop = true;

    if (_movementThread != nullptr)
    {
        if (_movementThread->joinable())
            _movementThread->join();

        delete _movementThread;
        _movementThread = nullptr;
    }

    _isActive = false;
}

// Esta función será llamada desde el EntityManager
// Para solicitar un movimiento al enemigo
void Enemy::RequestMove(std::function<bool(Vector2, Vector2&)> canMoveCallback)
{
    if (!CanPerformAction())
        return;

    Vector2 direction = GetRandomDirection();
    Vector2 currentPos = GetPosition();
    Vector2 newPos = currentPos + direction;
    Vector2 confirmedNewPos;

    // Preguntar al EntityManager si podemos movernos
    if (canMoveCallback(newPos, confirmedNewPos))
    {
        SetPosition(confirmedNewPos);
        UpdateActionTime();
    }
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

//Loop del thread individual del enemigo
// Este loop NO mueve al enemigo directamente
// PROPÓSITO: Mantener el thread activo - el movimiento real lo hace EntityManager
// RAZÓN: Centralizar lógica de colisiones y movimiento en EntityManager
void Enemy::MovementLoop()
{
    while (!_shouldStop)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (_shouldStop)
            break;

        // El movimiento real se gestiona desde EntityManager
        // Este loop solo mantiene el thread activo
    }
}