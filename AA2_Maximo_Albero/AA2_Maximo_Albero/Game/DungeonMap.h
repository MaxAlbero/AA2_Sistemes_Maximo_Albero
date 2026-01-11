#pragma once
#include <vector>
#include "Room.h"
#include "Portal.h"

class DungeonMap
{
private:
    std::vector<std::vector<Room*>> _rooms; // Grid de rooms
    int _currentX;
    int _currentY;
    int _worldWidth;
    int _worldHeight;

public:
    DungeonMap(int width, int height)
        : _currentX(0), _currentY(0), _worldWidth(width), _worldHeight(height)
    {
        // Crear grid de rooms
        _rooms.resize(height);
        for (int y = 0; y < height; y++)
        {
            _rooms[y].resize(width, nullptr);
        }
    }

    ~DungeonMap()
    {
        for (auto& row : _rooms)
        {
            for (Room* r : row)
            {
                if (r != nullptr)
                    delete r;
            }
        }
        _rooms.clear();
    }

    void SetRoom(int x, int y, Room* room);
    Room* GetRoom(int x, int y);
    void SetActiveRoom(int x, int y);
    Room* GetActiveRoom();

    int GetCurrentX() const { return _currentX; }
    int GetCurrentY() const { return _currentY; }
    int GetWorldWidth() const { return _worldWidth; }
    int GetWorldHeight() const { return _worldHeight; }

    bool HasRoomInDirection(PortalDir direction);
    void Draw();
};