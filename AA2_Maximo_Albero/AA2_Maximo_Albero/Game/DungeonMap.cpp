#include "DungeonMap.h"

void DungeonMap::SetRoom(int x, int y, Room* room)
{
    if (x >= 0 && x < _worldWidth && y >= 0 && y < _worldHeight)
    {
        _rooms[y][x] = room;
    }
}

Room* DungeonMap::GetRoom(int x, int y)
{
    if (x >= 0 && x < _worldWidth && y >= 0 && y < _worldHeight)
    {
        return _rooms[y][x];
    }
    return nullptr;
}

void DungeonMap::SetActiveRoom(int x, int y)
{
    if (x >= 0 && x < _worldWidth && y >= 0 && y < _worldHeight)
    {
        _currentX = x;
        _currentY = y;
    }
}

Room* DungeonMap::GetActiveRoom()
{
    return GetRoom(_currentX, _currentY);
}

// Verify if there's a room in a specific direction
bool DungeonMap::HasRoomInDirection(PortalDir direction)
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

void DungeonMap::Draw()
{
    Room* room = GetActiveRoom();
    if (room != nullptr)
        room->Draw();
}