#pragma once
#include "../NodeMap/NodeMap.h"
#include "Wall.h"
#include "Portal.h"

class Room
{
private:
    NodeMap* _map;
    Vector2 _size;

public:
    Room(Vector2 size, Vector2 offset) : _size(size)
    {
        _map = new NodeMap(size, offset);

        // Crear paredes en los bordes
        for (int x = 0; x < size.X; x++)
        {
            for (int y = 0; y < size.Y; y++)
            {
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
                    });
            }
        }
    }

    ~Room()
    {
        delete _map;
    }

    NodeMap* GetMap() { return _map; }
    Vector2 GetSize() const { return _size; }

    void Draw()
    {
        _map->UnSafeDraw();
    }

    // Genera portales basándose en la posición del room en el mundo
    void GeneratePortals(int worldX, int worldY, int worldWidth, int worldHeight)
    {
        int centerX = _size.X / 2;
        int centerY = _size.Y / 2;

        // Portal Izquierda
        if (worldX > 0)
        {
            Vector2 portalPos(0, centerY);
            _map->SafePickNode(portalPos, [](Node* n) {
                if (n != nullptr)
                {
                    // Reemplazar la pared con un portal
                    delete n->GetContent();
                    n->SetContent(new Portal(PortalDir::Left));
                }
                });
        }

        // Portal Derecha
        if (worldX < worldWidth - 1)
        {
            Vector2 portalPos(_size.X - 1, centerY);
            _map->SafePickNode(portalPos, [](Node* n) {
                if (n != nullptr)
                {
                    delete n->GetContent();
                    n->SetContent(new Portal(PortalDir::Right));
                }
                });
        }

        // Portal Arriba
        if (worldY > 0)
        {
            Vector2 portalPos(centerX, 0);
            _map->SafePickNode(portalPos, [](Node* n) {
                if (n != nullptr)
                {
                    delete n->GetContent();
                    n->SetContent(new Portal(PortalDir::Up));
                }
                });
        }

        // Portal Abajo
        if (worldY < worldHeight - 1)
        {
            Vector2 portalPos(centerX, _size.Y - 1);
            _map->SafePickNode(portalPos, [](Node* n) {
                if (n != nullptr)
                {
                    delete n->GetContent();
                    n->SetContent(new Portal(PortalDir::Down));
                }
                });
        }
    }

    // Obtiene la posición de spawn al entrar por un portal
    Vector2 GetSpawnPositionFromPortal(PortalDir fromDirection)
    {
        int centerX = _size.X / 2;
        int centerY = _size.Y / 2;

        switch (fromDirection)
        {
        case PortalDir::Left:
            // Vino desde la izquierda, spawn a la derecha del portal izquierdo
            return Vector2(1, centerY);
        case PortalDir::Right:
            // Vino desde la derecha, spawn a la izquierda del portal derecho
            return Vector2(_size.X - 2, centerY);
        case PortalDir::Up:
            // Vino desde arriba, spawn debajo del portal superior
            return Vector2(centerX, 1);
        case PortalDir::Down:
            // Vino desde abajo, spawn arriba del portal inferior
            return Vector2(centerX, _size.Y - 2);
        }

        return Vector2(1, 1); // Fallback
    }
};