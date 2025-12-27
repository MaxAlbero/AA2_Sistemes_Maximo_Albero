#include "ConsoleControl.h"


ConsoleControl ConsoleControl::GetInstance()
{
	static ConsoleControl instance;

	return instance;
}

HANDLE ConsoleControl::GetConsole()
{
	return GetInstance()._console;
}

void ConsoleControl::SetColor(ConsoleColor textColor, ConsoleColor backgroundColor) {
	WORD color = (backgroundColor << 4) | textColor;
	SetConsoleTextAttribute(GetConsole(), color);
}

void ConsoleControl::SetPosition(short int x, short int y) {
	COORD pos{ x, y };
	SetConsoleCursorPosition(GetConsole(), pos);
}

void ConsoleControl::Clear() {
	std::cout << "\033[2J\033[1;1H"; //Comanda directa a la consola que neteja la pantalla
}

void ConsoleControl::FillWithCharacter(char character, ConsoleColor textColor, ConsoleColor backgroundColor)
{
	COORD topLeft = { 0,0 };
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;
	HANDLE console = GetConsole();

	WORD color = (backgroundColor << 4) | textColor;
	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, character, screen.dwSize.X * screen.dwSize.Y, topLeft,	&written);
	FillConsoleOutputAttribute(
		console, color, screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
	SetConsoleCursorPosition(console, topLeft);
}

void ConsoleControl::ClearKeyBuffer()
{
	while (_kbhit())
	{
		_getch();
	}
}

int ConsoleControl::ReadNextKey()
{
	int KB_code = 0;

	while (KB_code == 0)
	{
		if (_kbhit())
		{
			KB_code = _getch();
		}
	}
	

	return KB_code;
}

char ConsoleControl::WaitForReadNextChar()
{
	char c = 0;

	while (c == 0)
	{
		if (_kbhit())
		{
			c = _getch();
		}
	}


	return c;
}

void ConsoleControl::Lock()
{
	GetInstance()._consoleMutex->lock();
}

void ConsoleControl::Unlock()
{
	GetInstance()._consoleMutex->unlock();
}