#include "InputSystem.h"
#include "../Utils/ConsoleControl.h"

InputSystem::KeyBinding::KeyBinding(int key, OnKeyPress onKeyPress)
{
	_key = key;
	_onKeyPress = onKeyPress;
}

InputSystem::KeyBinding::~KeyBinding()
{
}

InputSystem::InputSystem()
{
}

InputSystem::~InputSystem()
{ //En el destructor de l'input system fem un bucle for utilitzant "pair",
	//per obtenir els valors que n'hi han dins de la llista per eliminar
	//tots els inputs associats a una acció
	for (std::pair<int, KeyBindingList> pair : _keyBindingMap)
	{
		KeyBindingList bindingList = pair.second;
		for (KeyBinding* binding : bindingList) {
			delete binding;
		}

		bindingList.clear();
	}

	_keyBindingMap.clear();
}

InputSystem::KeyBinding* InputSystem::AddListener(int key, KeyBinding::OnKeyPress onKeyPress)
{
	_classMutex.lock();

	//busco la key, sino la trobo la creo
	if (_keyBindingMap.find(key) == _keyBindingMap.end())
	{
		_keyBindingMap[key] = KeyBindingList();
	}

	KeyBinding* keyBinding = new KeyBinding(key, onKeyPress);
	_keyBindingMap[key].push_back(keyBinding);

	_classMutex.unlock();

	return keyBinding;
}

void InputSystem::RemoveAndDeleteListener(KeyBinding* keyBinding)
{
	int key = keyBinding->_key;
	_classMutex.lock();

	if (_keyBindingMap.find(key) != _keyBindingMap.end()) {
		KeyBindingList list = _keyBindingMap[key];
		list.remove(keyBinding);
		delete keyBinding;

		if (list.size() <= 0) {
			_keyBindingMap.erase(key);
		}
		{

		}
	}

	_classMutex.unlock();
}

void InputSystem::StartListen()
{
	_classMutex.lock();

	if (_state != Stopped)
	{
		_classMutex.unlock();
		return;
	}

	_state = Starting;

	std::thread* listenLoopThread = new std::thread(&InputSystem::ListenLoop, this);
	listenLoopThread->detach();

	_classMutex.unlock();
}

void InputSystem::StopListen()
{
	_classMutex.lock();

	if (_state != Listening)
	{
		_classMutex.unlock();
		return;
	}

	_state = Stopping;

	_classMutex.unlock();
}

void InputSystem::ListenLoop()
{
	_classMutex.lock();

	_state = Listening;
	State currentState = _state;
	CC::ClearKeyBuffer();

	_classMutex.unlock();

	while (currentState == Listening)
	{
		int key = CC::ReadNextKey(); //Llegeixo quina tecla s'ha apetat

		if (key != 0) {
			_classMutex.lock();

			if (_keyBindingMap.find(key) != _keyBindingMap.end())
			{
				KeyBindingList list = _keyBindingMap[key];

				for (KeyBinding* binding : list) {
					std::thread* onKeyPressThread = new std::thread(binding->_onKeyPress);
					onKeyPressThread->detach();
				}
			}

			_classMutex.unlock();
		}


		_classMutex.lock();
		currentState = _state;
		_classMutex.unlock();
	}

	_classMutex.lock();
	if (_state == Stopping)
	{
		_state = Stopped;
	}
	_classMutex.unlock();
}
