#include "NodeMap.h"

NodeMap::NodeMap(Vector2 size, Vector2 offset)
{
	_size = size;
	_offset = offset;

	for (int x = 0; x < _size.X; x++) {
		NodeColumn* column = new NodeColumn();

		for (int y = 0; y < _size.Y; y++) {
			column->push_back(new Node(Vector2(x, y)));
		}

		_grid.push_back(column);
	}
}

Vector2 NodeMap::GetSize()
{
	_sizeMutex.lock();
	Vector2 size = _size;
	_sizeMutex.unlock();

	return size;
}

void NodeMap::UnSafeDraw() //Este draw es unsafe ya que no usa mutex para proteger los nodes
{
	for (NodeColumn* column : _grid) {
		for (Node* node : *column) {
			node->DrawContent(_offset);
		}
	}
}

void NodeMap::SafePickNode(Vector2 position, SafePick safePickAction)
{
	_sizeMutex.lock();
	_gridMutex.lock();

	Node* node = UnSafeGetNode(position);


	_gridMutex.unlock();
	_sizeMutex.unlock();

	node->Lock();
	safePickAction(node);
	node->Unlock();

}

void NodeMap::SafeMultiPickNode(std::list<Vector2> positions, SafeMultiPick safeMultiPickAction)
{
	std::list<Node*> nodes = std::list<Node*>();

	_sizeMutex.lock();
	_gridMutex.lock();

	for (Vector2 pos : positions) {
		//Node* node = UnSafeGetNode(pos);
		nodes.push_back(UnSafeGetNode(pos));
	}

	_gridMutex.unlock();
	_sizeMutex.unlock();

	_safeMultiPickNodeMutex.lock();

	for (Node* node : nodes) {
		if (node != nullptr) {
			node->Lock();
		}
	}

	_safeMultiPickNodeMutex.unlock();

	safeMultiPickAction(nodes);

	for (Node* node : nodes) {
		if (node != nullptr) {
			node->Unlock();
		}
	}
}


Node* NodeMap::UnSafeGetNode(Vector2 position)
{
	if (position.X >= _size.X || position.Y >= _size.Y ||position.X < 0 || position.Y < 0) {
		return nullptr;
	}

	NodeColumn* column = _grid[position.X]; //a la grid le cojo la posición X para las columnas
	Node* node = (*column)[position.Y]; //acceder a los datos de position.Y des de la lista de verdad

	return node;
	//return (*(_grid[position.X]))[position.Y];
}
