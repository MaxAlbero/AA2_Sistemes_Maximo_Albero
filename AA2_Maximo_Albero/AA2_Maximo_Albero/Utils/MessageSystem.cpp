#include "MessageSystem.h"
#include "ConsoleControl.h"
#include "GameConstants.h"
#include <iostream>
#include <chrono>
#include <thread>

MessageSystem::MessageSystem() : _running(false), _thread(nullptr), _needsRedraw(false) {}

MessageSystem::~MessageSystem() 
{
    Stop();
}

void MessageSystem::Start() 
{
    _running = true;
    _thread = new std::thread(&MessageSystem::ThreadLoop, this);
}

void MessageSystem::Stop() 
{
    _running = false;
    if (_thread) 
    {
        _thread->join();
        delete _thread;
        _thread = nullptr;
    }
}

void MessageSystem::PushMessage(const std::string& text, int durationMs) 
{
    _mutex.lock();

    _messages.insert(_messages.begin(), { text, durationMs });

    if (_messages.size() > _maxLines)
    {
        _messages.pop_back();
    }

    _needsRedraw = true;
    _mutex.unlock();
}

void MessageSystem::ThreadLoop() 
{
    while (_running) 
    {
        if (_needsRedraw) 
        {
            DrawMessages();
            _needsRedraw = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void MessageSystem::DrawMessages()
{
    _mutex.lock();
    for (size_t i = 0; i < _messages.size(); ++i) 
    {
        CC::Lock();
        CC::SetPosition(0, TEXT_AREA_Y + i);
        std::cout << _messages[i].text << "                                ";
        CC::Unlock();
    }

    for (size_t i = _messages.size(); i < _maxLines; ++i) {
        CC::Lock();
        CC::SetPosition(0, TEXT_AREA_Y + i);
        std::cout << "                                        ";
        CC::Unlock();
    }
    _mutex.unlock();
}
