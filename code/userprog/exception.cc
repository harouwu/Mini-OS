// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "sysdep.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
userFork(int addr)
{
    currentThread->space->ForkInitRegisters(addr);
    currentThread->space->RestoreState();
    printf("Forked userprog starts to run!\n");
    machine->Run();
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == SyscallException) {
        if ((type == SC_Halt)) {
        	DEBUG('a', "Shutdown, initiated by user program.\n");
        	interrupt->Halt();
        } else if (type == SC_Create) {
            int base = machine->ReadRegister(4);
            int value;
            int cnt = 0;
            do {
                machine->ReadMem(base++, 1, &value);
                cnt++;
            } while (value != 0);
            base -= cnt;
            char fileName[cnt];
            for (int i = 0; i < cnt; ++i) {
                machine->ReadMem(base+i, 1, &value);
                fileName[i] = (char)value;
            }
            bool result = fileSystem->Create(fileName, 128);
            if (result)
                printf("Create file %s success!\n", fileName);
            else
                printf("Create file %s failed!\n", fileName);
            machine->AdvancePC();
        } else if (type == SC_Open) {
            int base = machine->ReadRegister(4);
            int value;
            int cnt = 0;
            do {
                machine->ReadMem(base++, 1, &value);
                cnt++;
            } while (value != 0);
            base -= cnt;
            char fileName[cnt];
            for (int i = 0; i < cnt; ++i) {
                machine->ReadMem(base+i, 1, &value);
                fileName[i] = (char)value;
            }
            OpenFile* file = fileSystem->Open(fileName);
            int fd = file->getFd();
            if (file != NULL) {
                machine->WriteRegister(2, fd);
                printf("Open file %s success! fd: %d\n", fileName, fd);
            }
            else {
                machine->WriteRegister(2, -1);
                printf("Open file %s failed!\n", fileName);
            }
            machine->AdvancePC();
        } else if (type == SC_Close) {
            int fd = machine->ReadRegister(4);
            Close(fd);
            #ifdef FILESYS
            fileSystem->refCnt[fd]--;
            if (fileSystem->refCnt[fd] == 0)
                strcpy(fileSystem->openFileNames[fd], "");
            #endif
            printf("Close file (fd:%d) success!\n", fd);
            machine->AdvancePC();
        } else if (type == SC_Write) {
            int base = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);
            int value;
            int cnt = 0;
            do {
                machine->ReadMem(base++, 1, &value);
                cnt++;
            } while (value != 0);
            base -= cnt;
            char tmp[cnt];
            for (int i = 0; i < cnt; ++i) {
                machine->ReadMem(base+i, 1, &value);
                tmp[i] = (char)value;
            }
            OpenFile* file = new OpenFile(fd);
            file->Write(tmp, size);
            delete file;
            printf("Write file (fd: %d) success!\n", fd);
            machine->AdvancePC();
        } else if (type == SC_Read) {
            int base = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);
            char tmp[size];
            int readnum;
            OpenFile* file = new OpenFile(fd);
            readnum = file->Read(tmp, size);
            for (int i = 0; i < size; ++i)
                machine->WriteMem(base, 1, tmp[i]);
            delete file;
            machine->WriteRegister(2, readnum);
            printf("Read file (fd: %d) success!\n", fd);
            printf("Read content: %s\n", tmp);
            machine->AdvancePC();
        } else if (type == SC_Exit) {
            int arg = machine->ReadRegister(4);
            printf("Exit arg: %d\n", arg);
            currentThread->Finish(arg);
        } else if (type == SC_Exec) {
            int base = machine->ReadRegister(4);
            int value;
            int cnt = 0;
            do {
                machine->ReadMem(base++, 1, &value);
                cnt++;
            } while (value != 0);
            base -= cnt;
            char fileName[cnt];
            for (int i = 0; i < cnt; ++i) {
                machine->ReadMem(base+i, 1, &value);
                fileName[i] = (char)value;
            }
            OpenFile *executable = fileSystem->Open(fileName);
            AddrSpace *space;
            if (executable == NULL) {
            printf("Unable to open file %s\n", fileName);
                machine->AdvancePC();
                return;
            }
            space = new AddrSpace(executable);    
            currentThread->space = space;
            delete executable;          // close file
            space->InitRegisters();     // set the initial register values
            space->RestoreState();      // load page table register
            printf("Execute program %s!\n", fileName);
            machine->Run();         // jump to the user progama
            ASSERT(FALSE);          // machine->Run never returns;
        } else if (type == SC_Join) {
            int tid = machine->ReadRegister(4);
            ASSERT(tid < MAX_THREAD_NUM);
            ASSERT(threadTable[tid] != NULL);
            threadTable[tid]->addToJoinList(currentThread);
            machine->AdvancePC();
            currentThread->Sleep();
            int exitState = currentThread->getJoinState();
            machine->WriteRegister(2, exitState);
        } else if (type == SC_Fork) {
            int addr = machine->ReadRegister(4);
            Thread *t = new Thread("forked userprog");
            t->setPriority(8);
            currentThread->space->refCnt++;
            t->space = currentThread->space;
            t->Fork(userFork, addr);
            machine->AdvancePC();
        } else if (type == SC_Yield) {
            machine->AdvancePC();
            printf("thread \"%s\" is yielding\n", currentThread->getName());
            currentThread->Yield();
        }
    } else if (which == TLBPageFaultException) {
    	int NextPC = machine->ReadRegister(NextPCReg);
    	DEBUG('a', "TLBPageFaultException!!\n");
    	machine->TLBSwap();
	machine->WriteRegister(PCReg, NextPC);
    } else if (which == PTEPageFaultException) {
    	DEBUG('a', "PTEPageFaultException!!\n");
    	machine->PTESwap();
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}