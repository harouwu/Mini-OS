#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "copyright.h"
#include "console.h"
#include "synch.h"

class SynchConsole
{
public:
	SynchConsole(char *readFile, char *writeFile);
	~SynchConsole();
	void PutChar(char ch);
	char GetChar();


private:
	Console *console;
	Lock *readLock;
	Lock *writeLock;
};

#endif