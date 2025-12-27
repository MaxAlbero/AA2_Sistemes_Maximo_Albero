#include "node.h"
#include "../Utils/ConsoleControl.h"

Node::Node(Vector2 position) {
	_position = position;
}

INodeContent* Node::GetContent() {
	return _content;
}

void Node::SetContent(INodeContent* newContent)
{
	_content = newContent;
}

void Node::DrawContent(Vector2 offset)
{
	Vector2 pos = offset + _position;
	if (_content == nullptr) {
		CC::Lock();
		CC::SetPosition(pos.X, pos.Y);
		std::cout << " ";
		CC::Unlock();
		return;
	}

	_content->Draw(pos);
}


void Node::Lock() {
	_classMutex.lock();
}
void Node::Unlock() {
	_classMutex.unlock();
}