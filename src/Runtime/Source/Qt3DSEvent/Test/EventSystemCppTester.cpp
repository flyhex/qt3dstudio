/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <Windows.h>
#include <iostream>
#include <ctime>

#include "foundation/Qt3DSFoundation.h"
#include "foundation/ErrorStream.h"
#include "foundation/TrackingAllocator.h"
#include "foundation/Qt3DSRefCounted.h"

#include "EventSystem.h"
#include "EventPollingSystem.h"

#include "CanProviderDemo.h"
#include "Qt3DSLuaIncludes.h"
#include "EventPollingSystemLuaBinding.h"

void TestCPPBindings()
{
    srand(clock());

    qt3ds::foundation::SErrorStream errors;
    qt3ds::foundation::CAllocator allocator(errors);
    qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> theFoundation =
        NVCreateFoundation(QT3DS_FOUNDATION_VERSION, allocator, errors);

    qt3ds::foundation::NVScopedRefCounted<IEventSystem> theEventSystem =
        IEventSystem::Create(*theFoundation);

    const int theProviderCount = 128;
    qt3ds::evt::IEventProvider *theProviders[theProviderCount];
    for (int i = 0; i < theProviderCount; ++i) {
        theProviders[i] = new CCanProviderDemo();
        theEventSystem->AddProvider(*theProviders[i]);
    }
    unsigned long theCount = 0;
    while (theCount < 500) {
        Qt3DSEventSystemEvent *evtBuffer[64];
        size_t numEvents = theEventSystem->GetNextEvents(evtBuffer, 64);
        for (size_t evtIdx = 0; evtIdx < numEvents; ++evtIdx) {
            Qt3DSEventSystemEvent *theEvent = evtBuffer[evtIdx];
            if (theEvent) {
                std::cout << "************ event " << ++theCount << " received ************"
                          << std::endl;
                std::cout << "number of data: " << theEvent->m_NumData << std::endl;
                Qt3DSEventSystemEventData *theData = theEvent->m_Data;
                for (int i = 0; i < theEvent->m_NumData; ++i) {
                    std::cout << "data " << i << ":\t";
                    if (QT3DSEventSystemEventTypesString == theData->m_Value.m_Type) {
                        std::cout << theData->m_Value.m_String;
                    } else {
                        std::cout << theData->m_Value.m_Number;
                    }
                    std::cout << std::endl;
                    ++theData;
                }
            } else {
                // std::cout << "xxxxxxxxxxxx no event xxxxxxxxxxxx" << std::endl;
            }
            unsigned int theRand = (unsigned int)rand() % 10000;
            if (theRand < 20) { // Ignore
                unsigned int theIndex = rand() % theProviderCount;
                theEventSystem->IgnoreProvider(*theProviders[theIndex]);
                std::cout << "Ignore " << theIndex << std::endl;
            } else if (theRand < 35) { // Activate
                unsigned int theIndex = rand() % theProviderCount;
                theEventSystem->ActivateProvider(*theProviders[theIndex]);
                std::cout << "Activate " << theIndex << std::endl;
            }
        }
    }
}

const char *luaScript =
    //"require( \"table\" )\n"
    "registerCANProvider( eventSystem:getCInterface() )\n"
    "local theCount = 0\n"
    "while theCount < 500 do\n"
    "    local theEvents = eventSystem:getNextEvents()\n"
    "    for _, theEvent in ipairs( theEvents ) do\n"
    "        output( \"************ event \" .. theCount .. \" received ************\" )\n"
    "        for theName, theValue in pairs( theEvent ) do\n"
    "            output( theName .. \":\" )\n"
    "            if type( theValue ) == \"table\" then\n"
    "                output( table.concat( theValue, \" \" ) )\n"
    "            else\n"
    "                output( \"\" .. theValue )\n"
    "            end\n"
    "        end\n"
    "        theCount = theCount + 1\n"
    "    end\n"
    "end";

struct SCToCppFactory : public IEventFactory
{
    Qt3DSEventSystemEventFactory m_Factory;
    SCToCppFactory(Qt3DSEventSystemEventFactory inFactory)
        : m_Factory(inFactory)
    {
    }

    virtual Qt3DSEventSystemEvent &CreateEvent(int inNumData)
    {
        return *m_Factory.createEvent(m_Factory.m_Factory, inNumData);
    }
    virtual size_t GetMaxNumEventData()
    {
        return m_Factory.getMaxNumEventData(m_Factory.m_Factory);
    }
    virtual size_t GetMaxStrLength() { return m_Factory.getMaxStrLength(m_Factory.m_Factory); }

    virtual Qt3DSEventSystemRegisteredStr RegisterStr(TEventStr inSrc)
    {
        return m_Factory.registerStr(m_Factory.m_Factory, inSrc);
    }
    // Null terminates if strlen(inSrc) > getMaxStrLength
    virtual TEventStr AllocateStr(TEventStr inSrc)
    {
        return m_Factory.allocateStr(m_Factory.m_Factory, inSrc);
    }

    // Returns null if inLength > getMaxStrLength
    virtual TEventStr AllocateStr(int inLength)
    {
        return m_Factory.allocateStrLen(m_Factory.m_Factory, inLength);
    }
};

static size_t GetNextEventsFn(void *inProvider, Qt3DSEventSystemEventFactory inFactory,
                              Qt3DSEventSystemEvent **outBuffer, size_t outBufLen)
{
    IEventProvider *provider = reinterpret_cast<IEventProvider *>(inProvider);
    SCToCppFactory theFactory(inFactory);
    return provider->GetNextEvents(theFactory, outBuffer, outBufLen);
}

static void ReleaseFn(void *inProvider)
{
    IEventProvider *provider = reinterpret_cast<IEventProvider *>(inProvider);
    provider->Release();
}

static int registerCANProvider(lua_State *inState)
{
    luaL_checktype(inState, 1, LUA_TLIGHTUSERDATA);
    Qt3DSEventSystemEventPoller *thePoller =
        reinterpret_cast<Qt3DSEventSystemEventPoller *>(lua_touserdata(inState, 1));
    if (thePoller) {
        Qt3DSEventSystemEventProvider provider;
        provider.m_Provider = static_cast<IEventProvider *>(new CCanProviderDemo());
        provider.getNextEvents = GetNextEventsFn;
        provider.release = ReleaseFn;
        thePoller->addProvider(thePoller->m_Poller, provider);
    }
    return 0;
}

static void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void)ud;
    (void)osize; /* not used */
    if (nsize == 0) {
        free(ptr);
        return NULL;
    } else
        return realloc(ptr, nsize);
}

static int output(lua_State *inState)
{
    const char *theStr = lua_tostring(inState, -1);
    puts(theStr);
    return 0;
}

void TestLuaBindings()
{
    lua_State *theState = lua_newstate(l_alloc, NULL);
    luaL_openlibs(theState);
    lua_pushcfunction(theState, registerCANProvider);
    lua_setfield(theState, LUA_GLOBALSINDEX, "registerCANProvider");

    qt3ds::foundation::SErrorStream errors;
    qt3ds::foundation::CAllocator allocator(errors);
    qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> theFoundation =
        NVCreateFoundation(QT3DS_FOUNDATION_VERSION, allocator, errors);

    qt3ds::foundation::NVScopedRefCounted<IEventSystem> theEventSystem =
        IEventSystem::Create(*theFoundation);
    SLuaEventPollerBinding::WrapEventPoller(theState, *theEventSystem);
    lua_setfield(theState, LUA_GLOBALSINDEX, "eventSystem");
    lua_pushcfunction(theState, output);
    lua_setfield(theState, LUA_GLOBALSINDEX, "output");

    luaL_loadstring(theState, luaScript);
    int failure = lua_pcall(theState, 0, 0, 0);
    if (failure) {
        const char *errorMsg = lua_tostring(theState, -1);
        puts(errorMsg);
    }
}

int main()
{
    // TestCPPBindings();
    TestLuaBindings();
    return 0;
}
