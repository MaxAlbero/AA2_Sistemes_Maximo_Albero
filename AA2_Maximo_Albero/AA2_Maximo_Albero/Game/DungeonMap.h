#pragma once
#include <vector>
#include "Room.h"

class DungeonMap
{
private:
    std::vector<Room*> _rooms;
    int _activeRoom = 0;

public:
    DungeonMap() {}

    ~DungeonMap()
    {
        for (Room* r : _rooms)
            delete r;
        _rooms.clear();
    }

    void AddRoom(Room* room)
    {
        _rooms.push_back(room);
    }

    void SetActiveRoom(int index)
    {
        if (index >= 0 && index < _rooms.size())
            _activeRoom = index;
    }

    Room* GetActiveRoom()
    {
        if (_rooms.size() == 0) return nullptr;
        return _rooms[_activeRoom];
    }

    void Draw()
    {
        Room* room = GetActiveRoom();
        if (room != nullptr)
            room->Draw();
    }
};
