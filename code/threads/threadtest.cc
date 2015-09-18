// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "thread.h"
#include "synch.h"
#include <sysdep.h>

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// checkCMD
// check user's command, only support 'ts' currently
//----------------------------------------------------------------------
void
checkCMD()
{
    char cmd[100];
    printf("Please input the command:\n");
    scanf("%s", cmd);
    if ((cmd[0] == 't' || cmd[0] == 'T') && (cmd[1] == 's' || cmd[1] == 'S'))
    {
        for (int i = 0; i < MAX_THREAD_NUM; ++i)
            if (threadTable[i] != NULL)
                threadTable[i]->Print();
    }
    else if ((cmd[0] == 'l' || cmd[0] == 'L') && (cmd[1] == 'T' || cmd[1] == 't'))
    {
        printf("lastScheduleTime: %d, currentTime: %d, main's tickCount: %d, TimeQuantum: %d\n",
            lastScheduleTime, stats->totalTicks, currentThread->getTickCount(), currentThread->getTimeQuantum());
        if (threadTable[1] != NULL)
            printf("Priority: main %d, t1 %d\n", currentThread->getPriority(),
                threadTable[1]->getPriority());
    }
    else if ((cmd[0] == 'l' || cmd[0] == 'L') && (cmd[1] == 'P' || cmd[1] == 'p'))
    {
        for (int i = 0; i < MAX_THREAD_NUM; ++i)
            if (threadTable[i] != NULL)
                printf("Thread \"%s\" (tid: %d) has priority %d\n", 
                    threadTable[i]->getName(), threadTable[i]->getTID(), threadTable[i]->getPriority());
    }
}

//----------------------------------------------------------------------
// SimpleThread
//  Loop 5 times, yielding the CPU to another ready thread 
//  each iteration.
//
//  "which" is simply a number identifying the thread, for debugging
//  purposes.
//----------------------------------------------------------------------
void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 10; num++) {
        printf("*** thread \"%s\" (uid: %d, tid: %d) looped %d times\n", currentThread->getName(),
            currentThread->getUID(), currentThread->getTID(), num);
        interrupt->OneTick();
        //currentThread->Yield();
    }
}


//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2
//  Set up 200 threads to test the limitation of threads' number
//----------------------------------------------------------------------
void
ThreadTest2()
{
    for (int i = 0; i < 200; ++i)
    {
        Thread *t = new Thread("testcase");
        if (t->getTID() == -1)
        {
            printf("Error: Cannot create new thread (Exceed the threshold)\n");
            delete t;
        }
        else
        {
            printf("Create thread successfully (uid: %d, tid: %d)\n", 
                t->getUID(), t->getTID());
            t->Fork(SimpleThread, 0);
        }
    }

    // main thread loop infinitely, checking user's command
    // while (1)
    // {
    //     checkCMD();
    //     currentThread->Yield();
    // }
}

void
ThreadTest3()
{
    Thread *t1 = new Thread("t1");
    Thread *t2 = new Thread("t2");
    Thread *t3 = new Thread("t3");
    t1->setPriority(6);
    t2->setPriority(30);
    t3->setPriority(100);
    t1->Fork(SimpleThread, 0);
    t2->Fork(SimpleThread, 0);
    t3->Fork(SimpleThread, 0);

    currentThread->setTimeQuantum(8);
    // main thread loop infinitely, checking user's command
    while (1)
    {
        checkCMD();
        interrupt->OneTick();
    }
}

int readcnt = 0;
Semaphore *mutex, *w;
int sharedValue;

void Reader(int which)
{
    for (int i = 0; i < 40; ++i)
    {
        mutex->P();
        readcnt++;
        if (readcnt == 1)
            w->P();
        mutex->V();
        // read the data
        printf("Thread \"%s\" (tid: %d) is reading. sharedValue: %d\n",
            currentThread->getName(), currentThread->getTID(), sharedValue);
        mutex->P();
        readcnt--;
        if (readcnt == 0)
            w->V();
        mutex->V();
        currentThread->Yield();
    }
}

void Writer(int which)
{
    for (int i = 0; i < 20; ++i)
    {
        w->P();
        // write the data
        sharedValue = Random() % 1000;
        printf("Thread \"%s\" (tid: %d) is writing. sharedValue: %d\n",
            currentThread->getName(), currentThread->getTID(), sharedValue);
        w->V();
        currentThread->Yield();
    }
}
void ReaderAndWriter()
{
    mutex = new Semaphore("mutex", 1);
    w = new Semaphore("w", 1);
    sharedValue = 100;
    Thread *t;
    for (int i = 0; i < 3; ++i)
    {
        t = new Thread("Writer");
        t->setPriority(Random() % 20);
        t->Fork(Writer, 0);
    }
    for (int i = 0; i < 10; ++i)
    {
        t = new Thread("Reader");
        t->setPriority(Random() % 20);
        t->Fork(Reader, 0);
    }
}

#define PRODUCER_NUM 10
#define CONSUMER_NUM 10
#define MAX_NUM 4
Lock *lock;
Condition *full, *empty;
int buffer[MAX_NUM] = {0};
int sum;


void Producer(int which)
{
    lock->Acquire();
    while (sum == MAX_NUM)
    {
        printf("Thread \"%s\" (tid: %d) sleeping. (No available slot)\n",
            currentThread->getName(), currentThread->getTID());
        empty->Wait(lock);
    }
    for (int i = 0; i < MAX_NUM; ++i)
        if (buffer[i] == 0)
        {
            buffer[i] = 1;
            printf("Thread \"%s\" (tid: %d) put an item at buffer %d.\n",
            currentThread->getName(), currentThread->getTID(), i);
            break;
        }
    sum++;
    full->Signal(lock);
    lock->Release();
}

void Consumer(int which)
{
    lock->Acquire();
    while (sum == 0)
    {
        printf("Thread \"%s\" (tid: %d) sleeping. (No available item)\n",
            currentThread->getName(), currentThread->getTID());
        full->Wait(lock);
    }
    for (int i = 0; i < MAX_NUM; ++i)
        if (buffer[i] == 1)
        {
            buffer[i] = 0;
            printf("Thread \"%s\" (tid: %d) consume an item at buffer %d.\n",
                currentThread->getName(), currentThread->getTID(), i);
            break;
        }
    sum--;
    empty->Signal(lock);
    lock->Release();
}

void ProducerAndConsumer()
{
    lock = new Lock("lock");
    full = new Condition("full");
    empty = new Condition("empty");
    sum = 0;
    Thread *t;
    currentThread->setTimeQuantum(200);
    for (int i = 0; i < PRODUCER_NUM; ++i)
    {
        t = new Thread("Producer");
        t->setPriority(Random() % 20);
        t->Fork(Producer, 0);
    }
    for (int i = 0; i < CONSUMER_NUM; ++i)
    {
        t = new Thread("Consumer");
        t->setPriority(Random() % 20);
        t->Fork(Consumer, 0);
    }
}

Barrier *b;
void NaiveThread(int which)
{
    b->Wait();
    printf("Thread \"%s\" (tid: %d) running\n",
                currentThread->getName(), currentThread->getTID());
}
void TestBarrier()
{
    b = new Barrier("test", 5);
    Thread *t;
    for (int i = 0; i < 20; ++i)
    {
        t = new Thread("Naive");
        t->Fork(NaiveThread, 0);
        t->setPriority(Random() % 20);
    }
}

RWlock *rwlock;

void NewReader(int which)
{
    for (int i = 0; i < 40; ++i)
    {
        rwlock->rdlock();
        // read the data
        printf("Thread \"%s\" (tid: %d) is reading. sharedValue: %d\n",
            currentThread->getName(), currentThread->getTID(), sharedValue);
        rwlock->unlock();
        currentThread->Yield();
    }
}

void NewWriter(int which)
{
    for (int i = 0; i < 20; ++i)
    {
        rwlock->wrlock();
        // write the data
        sharedValue = Random() % 1000;
        printf("Thread \"%s\" (tid: %d) is writing. sharedValue: %d\n",
            currentThread->getName(), currentThread->getTID(), sharedValue);
        rwlock->unlock();
        currentThread->Yield();
    }
}
void
NewReaderAndWriter()
{
    rwlock = new RWlock("rwlock");
    sharedValue = 100;
    Thread *t;
    for (int i = 0; i < 3; ++i)
    {
        t = new Thread("Writer");
        t->setPriority(Random() % 20);
        t->Fork(NewWriter, 0);
    }
    for (int i = 0; i < 10; ++i)
    {
        t = new Thread("Reader");
        t->setPriority(Random() % 20);
        t->Fork(NewReader, 0);
    }
    // while (1)
    // {
    //     checkCMD();
    //     currentThread->Yield();
    // }
}
#ifdef USER_PROGRAM
extern void StartProcess(char *filename);
void usertest1(int which)
{
    char *filename = "test/d";
    StartProcess(filename);
}
void usertest2(int which)
{
    char *filename = "test/e";
    StartProcess(filename);
}
void doubleUserTest()
{
    Thread *t1 = new Thread("t1");
    Thread *t2 = new Thread("t2");
    t1->Fork(usertest1, 1);
    t2->Fork(usertest2, 1);
}
#endif
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void postMsgThread(int which)
{
    int qid = currentThread->applyMessageQueue();
    currentThread->postMsg(qid, "hello");
    currentThread->postMsg(qid, "world!");
}

void getMsgThread(int which)
{
    char msg[100];
    currentThread->getMsg(0, msg);
    printf("%s\n", msg);
    currentThread->getMsg(0, msg);
    printf("%s\n", msg);
}

void
ThreadTest4()
{
    Thread *t1 = new Thread("t1");
    Thread *t2 = new Thread("t2");
    t1->setPriority(30);
    t1->Fork(postMsgThread, 0);
    t2->Fork(getMsgThread, 0);
}

Lock *lock1, *lock2, *lock3;

void getLock1First(int which)
{
    lock1->Acquire();
    printf("%s get Lock1\n", currentThread->getName());
    currentThread->Yield();
    printf("%s trying to get lock2\n", currentThread->getName());
    lock2->Acquire();
    printf("%s get Lock2\n", currentThread->getName());
    lock2->Release();
    lock1->Release();
}

void getLock2First(int which)
{
    lock2->Acquire();
    printf("%s get Lock2\n", currentThread->getName());
    currentThread->Yield();
    printf("%s trying to get lock3\n", currentThread->getName());
    lock3->Acquire();
    printf("%s get Lock3\n", currentThread->getName());
    lock3->Release();
    lock2->Release();
}

void getLock3First(int which)
{
    lock3->Acquire();
    printf("%s get Lock3\n", currentThread->getName());
    currentThread->Yield();
    printf("%s trying to get lock1\n", currentThread->getName());
    lock1->Acquire();
    printf("%s get Lock1\n", currentThread->getName());
    lock1->Release();
    // lock3->Release();
}

void DeadLockTest()
{
    lock1 = new Lock("lock1");
    lock2 = new Lock("lock2");
    lock3 = new Lock("lock3");
    Thread *t1 = new Thread("t1");
    Thread *t2 = new Thread("t2");
    Thread *t3 = new Thread("t3");
    t1->setPriority(30);
    t2->setPriority(30);
    t3->setPriority(30);
    t1->Fork(getLock1First, 0);
    t2->Fork(getLock2First, 0);
    t3->Fork(getLock3First, 0);
    // currentThread->Yield();
    // currentThread->Yield();
    // if (DeadLockDetect())
    //     printf("DeadLock occurs!!\n");
    // else
    //     printf("No DeadLock\n");
}
void
ThreadTest()
{
    switch (testnum) {
    case 1:
        DeadLockTest();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}

