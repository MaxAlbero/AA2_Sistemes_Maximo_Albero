//#include "Game.h"
//#include <iostream>
//#include <thread>
//#include <chrono>
//
//int main()
//{
//	DungeonMap dungeon;
//
//	Room* room1 = new Room(Vector2(20, 10), Vector2(0, 0));
//	Room* room2 = new Room(Vector2(15, 15), Vector2(30, 0));
//
//	dungeon.AddRoom(room1);
//	dungeon.AddRoom(room2);
//
//	dungeon.SetActiveRoom(0);
//	dungeon.Draw();
//
//}

#include "Game.h"
#include "../Utils/ConsoleControl.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

int main()
{
    //srand(time(NULL));

    Game game;
    game.Start();

    // Mantener el programa corriendo
    // Puedes añadir una condición de salida si lo deseas
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Opcional: añadir condición de salida con ESC
        // if (tecla ESC presionada) break;
    }

    game.Stop();

    return 0;
}