/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
C++ Portable Types Library (PTypes)

  Copyright (C) 2001-2007 Hovik Melikyan

  http://www.melikyan.com/ptypes/


  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software.  If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.
*/

#pragma once
#ifndef __QT3DS_SYNC_H__
#define __QT3DS_SYNC_H__

#ifdef WIN32
#define _WINSOCKAPI_ // prevent inclusion of winsock.h in windows.h
#include <windows.h>
#elif defined(__MACH__)
#include <pthread.h>
#else
#include <semaphore.h>
#endif

//
//  Summary of implementation:
//
//  atomic increment/decrement/exchange
//    Win32: internal, asm
//    GCC/i386: internal, asm
//    Other: internal, mutex hash table
//
//  mutex
//    Win32: Critical section
//    Other: POSIX mutex
//
//  trigger
//    Win32: Event
//    Other: internal, POSIX cond/mutex
//
//  rwlock:
//    Win32: internal, Event/mutex
//    MacOS: internal, POSIX cond/mutex
//    Other: POSIX rwlock
//
//  semaphore:
//    Win32: = tsemaphore
//    MacOS: = tsemaphore
//    Other: POSIX semaphore
//
//  tsemaphore (with timed waiting):
//    Win32: Semaphore
//    Other: internal, POSIX mutex/cond
//

#ifdef WIN32
typedef long pthread_id_t;
#ifndef __GNUC__
typedef HANDLE pthread_t;
#endif
#define __PFASTCALL __fastcall
#elif defined(__MACH__)
typedef pthread_t pthread_id_t;
#define __PFASTCALL
#else
typedef long pthread_id_t;
#define __PFASTCALL __attribute__((fastcall))
#endif

long __PFASTCALL pincrement(long *target);
long __PFASTCALL pdecrement(long *target);
long __PFASTCALL pexchange(long *target, long value);
void *__PFASTCALL pexchange(void **target, void *value);

void psleep(long milliseconds);
bool pthrequal(pthread_id_t id); // note: this is NOT the thread handle, use thread::get_id()
pthread_id_t pthrself(); // ... same

// -------------------------------------------------------------------- //
// --- mutex ---------------------------------------------------------- //
// -------------------------------------------------------------------- //

#ifdef WIN32

struct mutex
{
protected:
    CRITICAL_SECTION critsec;

public:
    mutex() { InitializeCriticalSection(&critsec); }
    ~mutex() { DeleteCriticalSection(&critsec); }
    void enter() { EnterCriticalSection(&critsec); }
    void leave() { LeaveCriticalSection(&critsec); }
    void lock() { enter(); }
    void unlock() { leave(); }
};

#else

struct mutex
{
protected:
    pthread_mutex_t mtx;

public:
    mutex() { pthread_mutex_init(&mtx, 0); }
    ~mutex() { pthread_mutex_destroy(&mtx); }
    void enter() { pthread_mutex_lock(&mtx); }
    void leave() { pthread_mutex_unlock(&mtx); }
    void lock() { enter(); }
    void unlock() { leave(); }
};

#endif

// -------------------------------------------------------------------- //
// --- trigger -------------------------------------------------------- //
// -------------------------------------------------------------------- //

#ifdef WIN32

class trigger
{
protected:
    HANDLE handle; // Event object
public:
    trigger(bool autoreset, bool state);
    ~trigger() { CloseHandle(handle); }
    void wait() { WaitForSingleObject(handle, INFINITE); }
    void post() { SetEvent(handle); }
    void signal() { post(); }
    void reset() { ResetEvent(handle); }
};

#else

class trigger
{
protected:
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    long state;
    bool autoreset;

public:
    trigger(bool autoreset, bool state);
    ~trigger();
    void wait();
    void post();
    void signal() { post(); }
    void reset();
};

#endif

// -------------------------------------------------------------------- //
// --- rwlock --------------------------------------------------------- //
// -------------------------------------------------------------------- //

#if defined(WIN32) || defined(__DARWIN__)
#define __PTYPES_RWLOCK__
#elif defined(linux)
// on Linux rwlocks are included only with -D_GNU_SOURCE.
// programs that don't use rwlocks, do not need to define
// _GNU_SOURCE either.
#if defined(_GNU_SOURCE) || defined(__USE_UNIX98)
#define __POSIX_RWLOCK__
#endif
#else
#define __POSIX_RWLOCK__
#endif

#ifdef __PTYPES_RWLOCK__

struct rwlock : protected mutex
{
protected:
#ifdef WIN32
    HANDLE reading; // Event object
    HANDLE finished; // Event object
    long readcnt;
    long writecnt;
#else
    pthread_mutex_t mtx;
    pthread_cond_t readcond;
    pthread_cond_t writecond;
    long locks;
    long writers;
    long readers;
#endif
public:
    rwlock();
    ~rwlock();
    void rdlock();
    void wrlock();
    void unlock();
    void lock() { wrlock(); }
};

#elif defined(__POSIX_RWLOCK__)

struct rwlock
{
protected:
    pthread_rwlock_t rw;

public:
    rwlock();
    ~rwlock() { pthread_rwlock_destroy(&rw); }
    void rdlock() { pthread_rwlock_rdlock(&rw); }
    void wrlock() { pthread_rwlock_wrlock(&rw); }
    void unlock() { pthread_rwlock_unlock(&rw); }
    void lock() { wrlock(); }
};

#endif

// -------------------------------------------------------------------- //
// --- semaphore ------------------------------------------------------ //
// -------------------------------------------------------------------- //

// use tsemaphore everywhere
#define __SEM_TO_TIMEDSEM__

#ifdef __SEM_TO_TIMEDSEM__

// map ordinary semaphore to timed semaphore

class tsemaphore;
typedef tsemaphore semaphore;

#else

class semaphore
{
protected:
    sem_t handle;

public:
    semaphore(long initvalue);
    virtual ~semaphore();

    void wait();
    void post();
    void signal() { post(); }
};

#endif

class tsemaphore
{
protected:
#ifdef WIN32
    HANDLE handle;
#else
    long count;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
#endif
public:
    tsemaphore(long initvalue);
    virtual ~tsemaphore();
    bool wait(long msecs = -1);
    void post();
    void signal() { post(); }
};

// -------------------------------------------------------------------- //
// --- thread --------------------------------------------------------- //
// -------------------------------------------------------------------- //

class thread
{
protected:
#ifdef WIN32
    long id;
#endif
    pthread_t handle;
    bool autofree;
    long running;
    long signaled;
    long freed;
    bool finished;
    tsemaphore relaxsem;

    virtual void execute() = 0;
    virtual void cleanup();

    bool relax(long msecs) { return relaxsem.wait(msecs); }

    friend void _threadepilog(thread *thr);

#ifdef WIN32
    friend long __stdcall _threadproc(void *arg);
#else
    friend void *_threadproc(void *arg);
#endif

public:
    thread(bool iautofree);
    virtual ~thread();

#ifdef WIN32
    pthread_id_t get_id() { return long(id); }
#else
    pthread_id_t get_id() { return handle; }
#endif

    bool get_running() { return running != 0; }
    bool get_finished() { return finished; }
    bool get_signaled() { return signaled != 0; }

    void start();
    void signal();
    void waitfor();
};

// -------------------------------------------------------------------- //
// --- msgqueue ------------------------------------------------------- //
// -------------------------------------------------------------------- //

const long MSG_USER = 0;
const long MSG_USER_MAX = 0xBFFFF;
const long MSG_LIB = 0xC0000;
const long MSG_QUIT = MSG_LIB;

class message
{
protected:
    message *next; // next in the message chain, used internally
    semaphore *sync; // used internally by msgqueue::send(), when called from a different thread
    friend class msgqueue; // my friend, message queue...
public:
    long id;
    long result;
    long param;
    message(long iid, long iparam = 0);
    virtual ~message();
};

class msgqueue
{
private:
    message *head; // queue head
    message *tail; // queue tail
    long qcount; // number of items in the queue
    semaphore sem; // queue semaphore
    mutex qlock; // critical sections in enqueue and dequeue
    mutex thrlock; // lock for the queue processing
    pthread_id_t owner; // thread ID of the queue processing thread

    void enqueue(message *msg);
    void push(message *msg);
    message *dequeue(bool safe = true);

    void purgequeue();
    long finishmsg(message *msg);
    void handlemsg(message *msg);
    void takeownership();

protected:
    bool quit;

    void defhandler(message &msg);
    virtual void msghandler(message &msg) = 0;

public:
    msgqueue();
    virtual ~msgqueue();

    // functions calling from the owner thread:
    long msgsavail() { return qcount; }
    void processone(); // process one message, may hang if no msgs in the queue
    void processmsgs(); // process all available messages and return
    void run(); // process messages until MSG_QUIT

    // functions calling from any thread:
    void post(message *msg);
    void post(long id, long param = 0);
    void posturgent(message *msg);
    void posturgent(long id, long param = 0);
    long send(message *msg);
    long send(long id, long param = 0);
};

#endif // __QT3DS_SYNC_H__
