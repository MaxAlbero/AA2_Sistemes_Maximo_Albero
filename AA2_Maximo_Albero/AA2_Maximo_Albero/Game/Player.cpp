#include "Player.h"

void Player::Draw(Vector2 pos)
{
    CC::Lock();
    CC::SetColor(CC::WHITE);
    CC::SetPosition(pos.X, pos.Y);
    std::cout << "J";
    CC::SetColor(CC::WHITE);
    CC::Unlock();
}

Vector2 Player::GetPosition() {
    _playerMutex.lock();
    Vector2 pos = _position;
    _playerMutex.unlock();
    return pos;
}


bool Player::CanPerformAction()
{
    _playerMutex.lock();
    auto currentTime = std::chrono::steady_clock::now();
    auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - _lastActionTime
    ).count();
    bool canAct = timeSinceLastAction >= _actionCooldownMs;
    _playerMutex.unlock();
    return canAct;
}

void Player::UpdateActionTime()
{
    _playerMutex.lock();
    _lastActionTime = std::chrono::steady_clock::now();
    _playerMutex.unlock();
}

void Player::SetActionCooldown(int milliseconds)
{
    _playerMutex.lock();
    _actionCooldownMs = milliseconds;
    _playerMutex.unlock();
}

void Player::Attack(IDamageable* entity) const {
    if (entity != nullptr)
        entity->ReceiveDamage(20);
}

void Player::ReceiveDamage(int damageToReceive) {
    _playerMutex.lock();
    _hp -= damageToReceive;

    //std::cout << "¡El jugador recibe " << damageToReceive << " de daño! HP: " << _hp << std::endl;

    if (_hp <= 0)
    {
        _hp = 0;
        //std::cout << "¡GAME OVER!" << std::endl;
    }

    _playerMutex.unlock();
}

void Player::AddCoin()
{
    _playerMutex.lock();
    _coins += 10;
    _playerMutex.unlock();
}

void Player::AddPotion()
{
    _playerMutex.lock();
    _potionCount++;
    _playerMutex.unlock();
}

void Player::ChangeWeapon()
{
    _playerMutex.lock();

    switch (_weapon) {
    case 0:
        _weapon = 1;
        break;
    case 1:
        _weapon = 0;
        break;
    default:
        _weapon = 0;
    }

    _playerMutex.unlock();
}

void Player::UsePotion()
{
    _playerMutex.lock();

    if (_potionCount > 0)
    {
        _potionCount--;
        _hp += 20; // Recupera 20 HP

        if (_hp > _maxHp)
            _hp = _maxHp;

        CC::Lock();
        CC::SetPosition(0, 18);  // Posición fija debajo del mapa
        std::cout << "¡Poción usada! HP: " << _hp << "    " << std::endl;
        CC::Unlock();
    }
    else
    {
        CC::Lock();
        CC::SetPosition(0, 18);
        std::cout << "No tienes pociones" << std::endl;
        CC::Unlock();
    }

    _playerMutex.unlock();
}


int Player::GetAttackRange()
{
    _playerMutex.lock();
    int range = (_weapon == 0) ? 1 : 2; // Espada = 1, Lanza = 2
    _playerMutex.unlock();
    return range;
}

int Player::GetHP()
{
    _playerMutex.lock();
    int hp = _hp;
    _playerMutex.unlock();
    return hp;
}

int Player::GetCoins()
{
    _playerMutex.lock();
    int coins = _coins;
    _playerMutex.unlock();
    return coins;
}

int Player::GetPotionCount()
{
    _playerMutex.lock();
    int potions = _potionCount;
    _playerMutex.unlock();
    return potions;
}

int Player::GetWeapon()
{
    _playerMutex.lock();
    int weapon = _weapon;
    _playerMutex.unlock();
    return weapon;
}

bool Player::IsAlive()
{
    _playerMutex.lock();
    bool alive = _hp > 0;
    _playerMutex.unlock();
    return alive;
}

Json::Value Player::Code() {
    Json::Value json;
    CodeSubClassType<Player>(json);
    json["posX"] = _position.X;
    json["posY"] = _position.Y;
    json["hp"] = _hp;
    json["maxHp"] = _maxHp;
    json["coins"] = _coins;
    json["potions"] = _potionCount;
    json["weapon"] = _weapon;
    return json;
}

void Player::Decode(Json::Value json) {
    _position.X = json["posX"].asInt();
    _position.Y = json["posY"].asInt();
    _hp = json["hp"].asInt();
    _maxHp = json["maxHp"].asInt();
    _coins = json["coins"].asInt();
    _potionCount = json["potions"].asInt();
    _weapon = json["weapon"].asInt();
}