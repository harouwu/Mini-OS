#include "synchconsole.h"
#include "synch.h"

static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) {readAvail->V();}
static void WriteDone(int arg) {writeDone->V();}

SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
	readLock = new Lock("readLock");
	writeLock = new Lock("writeLock");
	readAvail = new Semaphore("readAvail", 0);
	writeDone = new Semaphore("writeDone", 0);
	console = new Console(readFile, writeFile, ReadAvail, WriteDone, 0);
}

SynchConsole::~SynchConsole()
{
	delete console;
	delete readLock;
	delete writeLock;
	delete readAvail;
	delete writeDone;
}

void
SynchConsole::PutChar(char ch)
{
	writeLock->Acquire();
	console->PutChar(ch);
	writeDone->P();
	writeLock->Release();
}

char
SynchConsole::GetChar()
{
	char ch;
	readLock->Acquire();
	readAvail->P();
	ch = console->GetChar();
	readLock->Release();
	return ch;
}