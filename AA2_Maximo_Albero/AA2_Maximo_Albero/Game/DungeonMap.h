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

    void SetRoom(int x, int y, Room* room)
    {
        if (x >= 0 && x < _worldWidth && y >= 0 && y < _worldHeight)
        {
            _rooms[y][x] = room;
        }
    }

    Room* GetRoom(int x, int y)
    {
        if (x >= 0 && x < _worldWidth && y >= 0 && y < _worldHeight)
        {
            return _rooms[y][x];
        }
        return nullptr;
    }

    void SetActiveRoom(int x, int y)
    {
        if (x >= 0 && x < _worldWidth && y >= 0 && y < _worldHeight)
        {
            _currentX = x;
            _currentY = y;
        }
    }

    Room* GetActiveRoom()
    {
        return GetRoom(_currentX, _currentY);
    }

    int GetCurrentX() const { return _currentX; }
    int GetCurrentY() const { return _currentY; }
    int GetWorldWidth() const { return _worldWidth; }
    int GetWorldHeight() const { return _worldHeight; }

    // Verifica si hay un room en una dirección específica
    bool HasRoomInDirection(PortalDir direction)
    {
        switch (direction)
        {
        case PortalDir::Left:
            return GetRoom(_currentX - 1, _currentY) != nullptr;
        case PortalDir::Right:
            return GetRoom(_currentX + 1, _currentY) != nullptr;
        case PortalDir::Up:
            return GetRoom(_currentX, _currentY - 1) != nullptr;
        case PortalDir::Down:
            return GetRoom(_currentX, _currentY + 1) != nullptr;
        }
        return false;
    }

    void Draw()
    {
        Room* room = GetActiveRoom();
        if (room != nullptr)
            room->Draw();
    }
};