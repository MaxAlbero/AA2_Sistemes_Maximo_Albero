#include "Room.h"

void Room::RemoveEnemy(Enemy* enemy)
{
    auto it = std::find(_enemies.begin(), _enemies.end(), enemy);
    if (it != _enemies.end())
    {
        _enemies.erase(it);
    }
}

void Room::RemoveChest(Chest* chest)
{
    auto it = std::find(_chests.begin(), _chests.end(), chest);
    if (it != _chests.end())
    {
        _chests.erase(it);
    }
}

void Room::RemoveItem(Item* item)
{
    auto it = std::find(_items.begin(), _items.end(), item);
    if (it != _items.end())
    {
        _items.erase(it);
    }
}

// Places all room entities into the visual map
// Called when entering a room (ChangeRoom)
// Entities already exist in memory (_enemies, _chests, _items),
// this function only makes them visible and places them in the NodeMap
// Allows enemies/chests to persist between room visits
void Room::ActivateEntities()
{
    // Put enemies in the map
    for (Enemy* enemy : _enemies)
    {
        if (enemy != nullptr)
        {
            Vector2 pos = enemy->GetPosition();
            _map->SafePickNode(pos, [enemy](Node* node) {
                if (node != nullptr)
                {
                    node->SetContent(enemy);
                }
                });

        }
    }

    // Put chests in the map
    for (Chest* chest : _chests)
    {
        if (chest != nullptr)
        {
            Vector2 pos = chest->GetPosition();
            _map->SafePickNode(pos, [chest](Node* node) {
                if (node != nullptr)
                {
                    node->SetContent(chest);
                }
                });
        }
    }

    // Put items in the map
    for (Item* item : _items)
    {
        if (item != nullptr)
        {
            Vector2 pos = item->GetPosition();
            _map->SafePickNode(pos, [item](Node* node) {
                if (node != nullptr)
                {
                    node->SetContent(item);
                }
                });
        }
    }
}

// Removes all entities from the visual map
// Called when leaving a room (ChangeRoom)
// Does NOT delete the entities from memory, only removes them from the NodeMap
// PURPOSE:
//   - Prevent them from being drawn when the room is not active
//   - Prevent race conditions with threads accessing the map
//   - Preserve entity state when returning to the room
void Room::DeactivateEntities()
{
    // Remove enemies from the map
    for (Enemy* enemy : _enemies)
    {
        if (enemy != nullptr)
        {
            Vector2 pos = enemy->GetPosition();
            _map->SafePickNode(pos, [](Node* node) {
                if (node != nullptr)
                {
                    node->SetContent(nullptr);
                }
                });
        }
    }

    // Remove chests from the map
    for (Chest* chest : _chests)
    {
        if (chest != nullptr)
        {
            Vector2 pos = chest->GetPosition();
            _map->SafePickNode(pos, [](Node* node) {
                if (node != nullptr)
                {
                    node->SetContent(nullptr);
                }
                });
        }
    }

    // Remove items from the map
    for (Item* item : _items)
    {
        if (item != nullptr)
        {
            Vector2 pos = item->GetPosition();
            _map->SafePickNode(pos, [](Node* node) {
                if (node != nullptr)
                {
                    node->SetContent(nullptr);
                }
                });
        }
    }
}

void Room::Draw()
{
    _map->UnSafeDraw();
}

// Creates portals on the room borders based on world position
// PARAMETERS:
//   - worldX, worldY: This room's coordinates in the world map
//   - worldWidth, worldHeight: Total world map size (3x3)
// LOGIC: Only generates portals if there is an adjacent room in that direction
void Room::GeneratePortals(int worldX, int worldY, int worldWidth, int worldHeight)
{
    int centerX = _size.X / 2;
    int centerY = _size.Y / 2;

    // Left portal
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

    // Right portal
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

    // Upper portal
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

    // Bottom portal
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

// Calculates where the player should spawn when entering from a portal
// LOGIC: Spawn on the OPPOSITE side of the portal used
// Prevents the player from spawning on the portal and teleporting again instantly
Vector2 Room::GetSpawnPositionFromPortal(PortalDir fromDirection)
{
    int centerX = _size.X / 2;
    int centerY = _size.Y / 2;

    switch (fromDirection)
    {
    case PortalDir::Left:
        // Came from the left, spawn a the right side of the left portal
        return Vector2(1, centerY);
    case PortalDir::Right:
        // Came from the right, spawn a the left side of the right portal
        return Vector2(_size.X - 2, centerY);
    case PortalDir::Up:
        // Came from up, spawn a the bottom of the upper portal
        return Vector2(centerX, 1);
    case PortalDir::Down:
        // Came from the bottom, spawn a the upper side of the bottom portal
        return Vector2(centerX, _size.Y - 2);
    }

    return Vector2(1, 1); // Fallback
}

Json::Value Room::Code() {
    Json::Value json;
    CodeSubClassType<Room>(json);
    json["initialized"] = _initialized;

    // Guardar enemigos
    Json::Value enemiesJson(Json::arrayValue);
    for (Enemy* enemy : _enemies) {
        enemiesJson.append(enemy->Code());
    }
    json["enemies"] = enemiesJson;

    // Guardar cofres
    Json::Value chestsJson(Json::arrayValue);
    for (Chest* chest : _chests) {
        chestsJson.append(chest->Code());
    }
    json["chests"] = chestsJson;

    // Guardar items
    Json::Value itemsJson(Json::arrayValue);
    for (Item* item : _items) {
        itemsJson.append(item->Code());
    }
    json["items"] = itemsJson;

    return json;
}

void Room::Decode(Json::Value json) {
    _initialized = json["initialized"].asBool();

    // Limpiar entidades existentes
    for (Enemy* enemy : _enemies) delete enemy;
    for (Chest* chest : _chests) delete chest;
    for (Item* item : _items) delete item;
    _enemies.clear();
    _chests.clear();
    _items.clear();

    // Cargar enemigos
    Json::Value enemiesJson = json["enemies"];
    for (const auto& enemyJson : enemiesJson) {
        Enemy* enemy = ICodable::FromJson<Enemy>(enemyJson);
        _enemies.push_back(enemy);
    }

    // Cargar cofres
    Json::Value chestsJson = json["chests"];
    for (const auto& chestJson : chestsJson) {
        Chest* chest = ICodable::FromJson<Chest>(chestJson);
        _chests.push_back(chest);
    }

    // Cargar items
    Json::Value itemsJson = json["items"];
    for (const auto& itemJson : itemsJson) {
        Item* item = ICodable::FromJson<Item>(itemJson);
        _items.push_back(item);
    }
}