#include "Player.h"

Player::~Player() {

}

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
    Lock();
    Vector2 pos = _position;
    Unlock();
    return pos;
}

void Player::SetPosition(Vector2 newPos) {
    _position = newPos;
}

bool Player::CanPerformAction()
{
    Lock();

    // Get current time
    auto currentTime = std::chrono::steady_clock::now();

    // Calculate how much time has passed since the last action
    auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - _lastActionTime
    ).count();

    // Check if cooldown time has passed (default 500ms)
    bool canAct = timeSinceLastAction >= _actionCooldownMs;

    Unlock();
    return canAct;
}

void Player::UpdateActionTime()
{
    Lock();
    _lastActionTime = std::chrono::steady_clock::now();
    Unlock();
}

void Player::SetActionCooldown(int milliseconds)
{
    Lock();
    _actionCooldownMs = milliseconds;
    Unlock();
}

void Player::Attack(IDamageable* entity) const {
    if (entity != nullptr)
        entity->ReceiveDamage(20);
}

void Player::ReceiveDamage(int damageToReceive) {
    Lock();
    _hp -= damageToReceive;

    if (_hp <= 0)
    {
        _hp = 0;
    }

    Unlock();
}

void Player::AddCoin()
{
    Lock();
    _coins += 10;
    Unlock();
}

void Player::AddPotion()
{
    Lock();
    _potionCount++;
    Unlock();
}

void Player::ChangeWeapon()
{
    Lock();

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

    Unlock();
}

void Player::UsePotion()
{
    Lock();

    // Do nothing if the player is dead
    if (_hp <= 0)
    {
        Unlock();
        return;
    }

    if (_potionCount > 0)
    {
        _potionCount--;
        _hp += 20;

        if (_hp > _maxHp)
            _hp = _maxHp;

        // Visual feedback
        CC::Lock();
        CC::SetPosition(0, 18);
        if (_messages != nullptr)
        {
            _messages->PushMessage("¡Pocion usada!", 2);
        }
        CC::Unlock();
    }

    Unlock();
}


//   - Sword (_weapon == 0): Range 1 (adjacent tile only)
//   - Spear (_weapon == 1): Range 2 (can attack from 2 tiles away)
int Player::GetAttackRange()
{
    Lock();

    // Determinar rango según arma equipada
    int range = (_weapon == 0) ? 1 : 2;

    Unlock();
    return range;
}

int Player::GetHP()
{
    Lock();
    int hp = _hp;
    Unlock();
    return hp;
}

int Player::GetCoins()
{
    Lock();
    int coins = _coins;
    Unlock();
    return coins;
}

int Player::GetPotionCount()
{
    Lock();
    int potions = _potionCount;
    Unlock();
    return potions;
}

int Player::GetWeapon()
{
    Lock();
    int weapon = _weapon;
    Unlock();
    return weapon;
}

bool Player::IsAlive()
{
    Lock();
    bool alive = _hp > 0;
    Unlock();
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