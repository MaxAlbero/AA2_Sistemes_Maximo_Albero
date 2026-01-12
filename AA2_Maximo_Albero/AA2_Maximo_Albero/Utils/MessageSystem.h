#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <thread>

struct Message {
    std::string text;
    int durationMs;
};

class MessageSystem {
public:
    MessageSystem();
    ~MessageSystem();

    void Start();
    void Stop();
    void PushMessage(const std::string& text, int durationMs = 20);

private:
    std::vector<Message> _messages;
    std::mutex _mutex;
    std::thread* _thread;
    bool _needsRedraw;
    bool _running;
    const int _maxLines = 5;

    void ThreadLoop();
    void DrawMessages();
};
