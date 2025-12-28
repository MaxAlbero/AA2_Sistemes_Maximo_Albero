#pragma once
#include "../NodeMap/NodeMap.h"
#include "Wall.h"

class Room
{
private:
    NodeMap* _map;

public:
    Room(Vector2 size, Vector2 offset)
    {
        _map = new NodeMap(size, offset);

        for (int x = 0; x < size.X; x++)
        {
            for (int y = 0; y < size.Y; y++)
            {
                // Verificar si estamos en un borde
                bool isTopEdge = (y == 0);
                bool isBottomEdge = (y == size.Y - 1);
                bool isLeftEdge = (x == 0);
                bool isRightEdge = (x == size.X - 1);

                bool isBorder = isTopEdge || isBottomEdge || isLeftEdge || isRightEdge;

                _map->SafePickNode(Vector2(x, y), [&, isBorder](Node* n) {
                    if (isBorder)
                    {
                        n->SetContent(new Wall());
                    }
                    // Si no es borde, no ponemos nada (nullptr)
                    });
            }
        }
    }

    ~Room()
    {
        delete _map;
    }

    NodeMap* GetMap() { return _map; }

    void Draw()
    {
        _map->UnSafeDraw();
    }
};