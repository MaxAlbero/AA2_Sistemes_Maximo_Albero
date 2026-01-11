#pragma once
#include "../NodeMap/NodeMap.h"
#include "Wall.h"
#include "Portal.h"
#include "Enemy.h"
#include "Chest.h"
#include "Item.h"
#include "../Json/ICodable.h"

class Room : public ICodable
{
private:
    NodeMap* _map;
    Vector2 _size;
    bool _initialized;

    std::vector<Enemy*> _enemies;
    std::vector<Chest*> _chests;
    std::vector<Item*> _items;

public:
    Room();
    Room(Vector2 size, Vector2 offset);
    ~Room();

    NodeMap* GetMap() { return _map; }
    Vector2 GetSize() const { return _size; }

    bool IsInitialized() const { return _initialized; }
    void SetInitialized(bool value) { _initialized = value; }

    void AddEnemy(Enemy* enemy) { _enemies.push_back(enemy); }
    void AddChest(Chest* chest) { _chests.push_back(chest); }
    void AddItem(Item* item) { _items.push_back(item); }

    void RemoveEnemy(Enemy* enemy);
    void RemoveChest(Chest* chest);
    void RemoveItem(Item* item);

    std::vector<Enemy*>& GetEnemies() { return _enemies; }
    std::vector<Chest*>& GetChests() { return _chests; }
    std::vector<Item*>& GetItems() { return _items; }

    Enemy* GetEnemyAt(Vector2 pos);
    Chest* GetChestAt(Vector2 pos);
    Item* GetItemAt(Vector2 pos);

    // Activación / desactivación visual
    void ActivateEntities();
    void DeactivateEntities();
    void Draw();

    void GeneratePortals(int worldX, int worldY, int worldWidth, int worldHeight);
    Vector2 GetSpawnPositionFromPortal(PortalDir fromDirection);

    Json::Value Code() override;
    void Decode(Json::Value json) override;

private:
    template<typename T>
    void DrawEntities(std::vector<T*>& entities);
};
