// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName)
{
    name = debugName;
    value = 1; // FREE
    queue = new List;
}

Lock::~Lock()
{
    delete queue;
}

void clearAllMarked()
{
    for (int i = 0; i < MAX_THREAD_NUM; ++i)
        if (threadTable[i] != NULL)
            threadTable[i]->clearMarked();
}
bool DeadLockDetect()
{
    int i;
    for (i = 0; i < MAX_THREAD_NUM; ++i)
        if (threadTable[i] != NULL && threadTable[i]->getWaitingLock() != NULL) {
            Thread *cur = threadTable[i];
            Lock *lock = (Lock *)cur->getWaitingLock();
            cur->mark();
            while (lock != NULL) {
                cur = lock->getOwner();
                if (cur == NULL)
                    break;
                if (cur->isMarked())
                    return true;
                cur->mark();
                lock = (Lock *)cur->getWaitingLock();
            }
            clearAllMarked();
        }
    return false;
}

void Lock::Acquire()
{
    // disable interrupts
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    while (value == 0) {            // lock is BUSY
        queue->Append((void *)currentThread);   // so go to sleep
        currentThread->setWaitingLock((void *)this);
        if (DeadLockDetect()) {
            Lock *lock = (Lock *)currentThread->getHoldLock();
            if (lock != NULL) {
                printf("%s has to release %s and finish\n", currentThread->getName(), lock->getName());
                lock->Release();
            }
        }
        currentThread->Sleep();
    }
    currentThread->setWaitingLock(NULL);
    currentThread->setHoldLock(this);
    owner = currentThread;   // lock is FREE, get the lock
    value = 0;                 // and set the value to be BUSY
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

void Lock::Release()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(isHeldByCurrentThread());
    Thread *thread;
    thread = (Thread *)queue->Remove();
    if (thread != NULL)    // make thread ready
        scheduler->ReadyToRun(thread);
    value = 1; // set the value to FREE
    (void) interrupt->SetLevel(oldLevel);
}

bool Lock::isHeldByCurrentThread()
{
    return currentThread == owner;
}

Condition::Condition(char* debugName)
{
    name = debugName;
    queue = new List;
}

Condition::~Condition()
{
    delete queue;
}

void Condition::Wait(Lock* conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock->isHeldByCurrentThread());
    conditionLock->Release();
    queue->Append((void *)currentThread);
    currentThread->Sleep();   //go to sleep
    conditionLock->Acquire();
    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock* conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock->isHeldByCurrentThread());
    Thread *thread;
    thread = (Thread *)queue->Remove();
    if (thread != NULL)    // make thread ready
        scheduler->ReadyToRun(thread);
    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock* conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock->isHeldByCurrentThread());
    Thread *thread;
    thread = (Thread *)queue->Remove();
    while (thread != NULL)    // make all threads in queue ready
    {
        scheduler->ReadyToRun(thread);
        thread = (Thread *)queue->Remove();
    }
    (void) interrupt->SetLevel(oldLevel);
}

Barrier::Barrier(char *debugName, int num)
{
    name = debugName;
    initNum = num;
    waitNum = num;
    queue = new List;
}

Barrier::~Barrier()
{
    delete queue;
}

void Barrier::Wait()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    waitNum--;
    if (waitNum == 0)
    {
        Thread *thread;
        thread = (Thread *)queue->Remove();
        while (thread != NULL)    // make all threads in queue ready
        {
            scheduler->ReadyToRun(thread);
            thread = (Thread *)queue->Remove();
        }
        waitNum = initNum;
    }
    else
    {
        printf("Thread \"%s\" (tid: %d) is blocked by barrier %s\n",
                currentThread->getName(), currentThread->getTID(), name);
        queue->Append((void *)currentThread);   // go to sleep
        currentThread->Sleep();
    }
    (void) interrupt->SetLevel(oldLevel);
}

RWlock::RWlock(char *debugName)
{
    name = debugName;
    readerQueue = new List;
    writerQueue = new List;
    status = NOP;
}

RWlock::~RWlock()
{
    delete readerQueue;
    delete writerQueue;
}

void RWlock::rdlock()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    while (status == WRITING || !writerQueue->IsEmpty())
    {
        printf("Thread \"%s\" (tid: %d) is blocked.\n",
            currentThread->getName(), currentThread->getTID());
        readerQueue->Append((void *)currentThread);   // go to sleep
        currentThread->Sleep();
    }
    status = READING;
    (void) interrupt->SetLevel(oldLevel);
}

void RWlock::wrlock()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    while (status == READING || status == WRITING)
    {
        printf("Thread \"%s\" (tid: %d) is blocked.\n",
            currentThread->getName(), currentThread->getTID());
        writerQueue->Append((void *)currentThread);   // go to sleep
        currentThread->Sleep();
    }
    status = WRITING;
    (void) interrupt->SetLevel(oldLevel);
}

void RWlock::unlock()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    Thread *thread;
    if (!writerQueue->IsEmpty()) // if any writer
    {
        thread = (Thread *)writerQueue->Remove();
        scheduler->ReadyToRun(thread);
    }
    else
    {
        thread = (Thread *)readerQueue->Remove();
        while (thread != NULL)
        {
            scheduler->ReadyToRun(thread);
            thread = (Thread *)readerQueue->Remove();
        }
    }
    status = NOP;
    (void) interrupt->SetLevel(oldLevel);
}