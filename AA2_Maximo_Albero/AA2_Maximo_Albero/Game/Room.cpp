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

std::vector<Enemy*>& GetEnemies() { return _enemies; }
std::vector<Chest*>& GetChests() { return _chests; }
std::vector<Item*>& GetItems() { return _items; }

void Room::ActivateEntities()
{
    // Colocar todas las entidades en el mapa y dibujarlas
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

            _map->SafePickNode(pos, [](Node* node) {
                if (node != nullptr)
                {
                    node->DrawContent(Vector2(0, 0));
                }
                });
        }
    }

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

            _map->SafePickNode(pos, [](Node* node) {
                if (node != nullptr)
                {
                    node->DrawContent(Vector2(0, 0));
                }
                });
        }
    }

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

            _map->SafePickNode(pos, [](Node* node) {
                if (node != nullptr)
                {
                    node->DrawContent(Vector2(0, 0));
                }
                });
        }
    }
}

void Room::DeactivateEntities()
{
    // Quitar todas las entidades del mapa visual (pero mantenerlas en memoria)
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

// Genera portales basándose en la posición del room en el mundo
void Room::GeneratePortals(int worldX, int worldY, int worldWidth, int worldHeight)
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
Vector2 Room::GetSpawnPositionFromPortal(PortalDir fromDirection)
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