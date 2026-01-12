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

        // VERIFICAR SI EL JUEGO TERMINÓ
        if (game.IsGameOver())
        {
            // Esperar un momento para mostrar el mensaje
            std::this_thread::sleep_for(std::chrono::seconds(3));

            // Detener el juego limpiamente
            game.Stop();

            // Mostrar mensaje final
            CC::Lock();
            CC::SetPosition(0, 18);
            std::cout << "Presiona cualquier tecla para salir..." << std::endl;
            CC::Unlock();

            // Esperar input final
            std::this_thread::sleep_for(std::chrono::seconds(2));

            break;  // Salir del loop
        }
    }

    return 0;
}