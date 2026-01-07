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

    // Obtener tiempo actual
    auto currentTime = std::chrono::steady_clock::now();

    // Calcular cuánto tiempo ha pasado desde la última acción
    auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - _lastActionTime
    ).count();

    // Verificar si ha pasado el tiempo de cooldown (por defecto 500ms)
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

    //std::cout << "¡El jugador recibe " << damageToReceive << " de daño! HP: " << _hp << std::endl;

    if (_hp <= 0)
    {
        _hp = 0;
        //std::cout << "¡GAME OVER!" << std::endl;
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

    if (_potionCount > 0)
    {
        _potionCount--;
        _hp += 20; // Recupera 20 HP por poción

        // Limitar HP al máximo
        if (_hp > _maxHp)
            _hp = _maxHp;

        // Mostrar feedback visual
        CC::Lock();
        CC::SetPosition(0, 18);  // Posición fija debajo del mapa
        if (_messages != nullptr)
        {
            _messages->PushMessage("¡Pocion usada!", 200);
        }
        CC::Unlock();
    }
    else
    {
        // No hay pociones disponibles
        CC::Lock();
        CC::SetPosition(0, 18);
        if (_messages != nullptr) {
            _messages->PushMessage("No tienes pociones", 200);
        }
        CC::Unlock();
    }

    Unlock();
}


//   - Espada (_weapon == 0): Rango 1 (solo casilla adyacente)
//   - Lanza (_weapon == 1): Rango 2 (puede atacar 2 casillas de distancia)
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