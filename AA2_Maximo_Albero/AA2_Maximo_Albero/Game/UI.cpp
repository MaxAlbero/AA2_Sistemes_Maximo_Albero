#include "UI.h"

void UI::Start(Player* player)
{
    _player = player;
    _running = true;
    _uiThread = new std::thread(&UI::UpdateLoop, this);
}

void UI::Stop()
{
    _running = false;

    if (_uiThread && _uiThread->joinable())
    {
        _uiThread->join();
        delete _uiThread;
        _uiThread = nullptr;
    }
}

void UI::DrawSidebar()
{
    if (_player == nullptr)
        return;

    int xOffset = _mapSize.X + 5;
    int y = TOP_UI_TEXT_AREA;

    CC::Lock();

    // Monedas
    CC::SetPosition(xOffset, y++);
    std::cout << "Monedas: " << _player->GetCoins() << "  ";

    // Vida
    CC::SetPosition(xOffset, y++);
    int hp = _player->GetHP();
    std::cout << "Vida: ";
    if (hp < 10)
        std::cout << "0" << hp << "  ";
    else
        std::cout << hp << "  ";

    // Pociones
    CC::SetPosition(xOffset, y++);
    std::cout << "Pociones: " << _player->GetPotionCount() << "  ";

    // Arma
    CC::SetPosition(xOffset, y++);
    int weapon = _player->GetWeapon();
    std::cout << "Arma: " << (weapon == 0 ? "Espada" : "Lanza ") << "  ";

    y++;
    y = BOTTOM_UI_TEXT_AREA;

    // Leyenda

    CC::SetPosition(xOffset, y + 3);
    std::cout << "Usa WASD para moverte." << std::endl;
    CC::SetPosition(xOffset, y);
    std::cout << "# = Pared  J = Jugador  E = Enemigo";
    CC::SetPosition(xOffset, y + 1);
    std::cout << "O = Portal  C = Cofre  k = Moneda ";
    CC::SetPosition(xOffset, y + 2);
    std::cout << "p = Pocion  w = Arma                ";

    CC::Unlock();
}

void UI::UpdateLoop()
{
    while (_running)
    {
        DrawSidebar();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}