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
#include "../Utils/HideConsoleCursor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

int main()
{
    //srand(time(NULL));
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
    }

    game.Stop();

    return 0;
}