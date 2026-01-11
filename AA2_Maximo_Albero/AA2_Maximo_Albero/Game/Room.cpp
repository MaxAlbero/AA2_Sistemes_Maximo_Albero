#include "Room.h"

Room::Room() : Room(Vector2(20, 10), Vector2(0, 0)) {}

Room::Room(Vector2 size, Vector2 offset)
    : _size(size), _initialized(false)
{
    _map = new NodeMap(size, offset);

    // Crear paredes en los bordes
    for (int x = 0; x < size.X; x++)
    {
        for (int y = 0; y < size.Y; y++)
        {
            bool isBorder = (x == 0 || y == 0 || x == size.X - 1 || y == size.Y - 1);

            _map->SafePickNode(Vector2(x, y), [isBorder](Node* n) {
                if (isBorder)
                    n->SetContent(new Wall());
                });
        }
    }
}

Room::~Room()
{
    delete _map;
}

// ======================
// BÚSQUEDAS
// ======================

Enemy* Room::GetEnemyAt(Vector2 pos)
{
    for (Enemy* e : _enemies)
        if (e && e->GetPosition().X == pos.X && e->GetPosition().Y == pos.Y)
            return e;
    return nullptr;
}

Chest* Room::GetChestAt(Vector2 pos)
{
    for (Chest* c : _chests)
        if (c && c->GetPosition().X == pos.X && c->GetPosition().Y == pos.Y)
            return c;
    return nullptr;
}

Item* Room::GetItemAt(Vector2 pos)
{
    for (Item* i : _items)
        if (i && i->GetPosition().X == pos.X && i->GetPosition().Y == pos.Y)
            return i;
    return nullptr;
}


// ======================
// ELIMINACIÓN
// ======================

void Room::RemoveEnemy(Enemy* enemy)
{
    _enemies.erase(std::remove(_enemies.begin(), _enemies.end(), enemy), _enemies.end());
}

void Room::RemoveChest(Chest* chest)
{
    _chests.erase(std::remove(_chests.begin(), _chests.end(), chest), _chests.end());
}

void Room::RemoveItem(Item* item)
{
    _items.erase(std::remove(_items.begin(), _items.end(), item), _items.end());
}

// ======================
// DIBUJADO UNIFICADO
// ======================

template<typename T>
void Room::DrawEntities(std::vector<T*>& entities)
{
    for (T* e : entities)
    {
        if (!e) continue;

        Vector2 pos = e->GetPosition();
        _map->SafePickNode(pos, [e](Node* n) { if (n) n->SetContent(e); });
        _map->SafePickNode(pos, [](Node* n) { if (n) n->DrawContent(Vector2(0, 0)); });
    }
}

// Coloca todas las entidades de la sala en el mapa visual
void Room::ActivateEntities()
{
    DrawEntities(_enemies);
    DrawEntities(_chests);
    DrawEntities(_items);
}

// Quita todas las entidades del mapa visual
void Room::DeactivateEntities()
{
    auto clear = [&](Vector2 pos) {
        _map->SafePickNode(pos, [](Node* n) {
            if (n) n->SetContent(nullptr);
            });
        };

    for (Enemy* e : _enemies) if (e) clear(e->GetPosition());
    for (Chest* c : _chests) if (c) clear(c->GetPosition());
    for (Item* i : _items) if (i) clear(i->GetPosition());
}

void Room::Draw()
{
    _map->UnSafeDraw();
}

void Room::GeneratePortals(int worldX, int worldY, int worldWidth, int worldHeight)
{
    int centerX = _size.X / 2;
    int centerY = _size.Y / 2;

    if (worldX > 0)
    {
        Vector2 pos(0, centerY);
        _map->SafePickNode(pos, [](Node* n) {
            if (n)
            {
                delete n->GetContent();
                n->SetContent(new Portal(PortalDir::Left));
            }
            });
    }

    if (worldX < worldWidth - 1)
    {
        Vector2 pos(_size.X - 1, centerY);
        _map->SafePickNode(pos, [](Node* n) {
            if (n)
            {
                delete n->GetContent();
                n->SetContent(new Portal(PortalDir::Right));
            }
            });
    }

    if (worldY > 0)
    {
        Vector2 pos(centerX, 0);
        _map->SafePickNode(pos, [](Node* n) {
            if (n)
            {
                delete n->GetContent();
                n->SetContent(new Portal(PortalDir::Up));
            }
            });
    }

    if (worldY < worldHeight - 1)
    {
        Vector2 pos(centerX, _size.Y - 1);
        _map->SafePickNode(pos, [](Node* n) {
            if (n)
            {
                delete n->GetContent();
                n->SetContent(new Portal(PortalDir::Down));
            }
            });
    }
}


Json::Value Room::Code()
{
    Json::Value json;
    json["initialized"] = _initialized;

    Json::Value enemies(Json::arrayValue);
    for (Enemy* e : _enemies)
        enemies.append(e->Code());
    json["enemies"] = enemies;

    Json::Value chests(Json::arrayValue);
    for (Chest* c : _chests)
        chests.append(c->Code());
    json["chests"] = chests;

    Json::Value items(Json::arrayValue);
    for (Item* i : _items)
        items.append(i->Code());
    json["items"] = items;

    return json;
}


void Room::Decode(Json::Value json)
{
    _initialized = json["initialized"].asBool();

    for (Enemy* e : _enemies) delete e;
    for (Chest* c : _chests) delete c;
    for (Item* i : _items) delete i;

    _enemies.clear();
    _chests.clear();
    _items.clear();

    for (auto& e : json["enemies"])
        _enemies.push_back(ICodable::FromJson<Enemy>(e));

    for (auto& c : json["chests"])
        _chests.push_back(ICodable::FromJson<Chest>(c));

    for (auto& i : json["items"])
        _items.push_back(ICodable::FromJson<Item>(i));
}
