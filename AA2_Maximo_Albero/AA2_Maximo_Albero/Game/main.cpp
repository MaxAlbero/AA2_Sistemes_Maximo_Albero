#include "Game.h"
#include "../Utils/ConsoleControl.h"
#include "../Utils/HideConsoleCursor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

int main()
{
    srand((unsigned int)time(NULL));
    HideConsoleCursor();

    ICodable::SaveDecodeProcess<Player>();
    ICodable::SaveDecodeProcess<Enemy>();
    ICodable::SaveDecodeProcess<Chest>();
    ICodable::SaveDecodeProcess<Item>();
    ICodable::SaveDecodeProcess<Room>();

    Game game;
    game.Start();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // VERIFY IF THE GAME IS FINISHED
        if (game.IsGameOver())
        {
            std::this_thread::sleep_for(std::chrono::seconds(3));

            // Stop the game
            game.Stop();

            CC::Lock();
            CC::SetPosition(0, 18);
            std::cout << "Presiona cualquier tecla para salir..." << std::endl;
            CC::Unlock();

            std::this_thread::sleep_for(std::chrono::seconds(2));

            break;
        }
    }

    return 0;
}