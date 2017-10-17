/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef QT3DS_EVENT_SYSTEM_C_H
#define QT3DS_EVENT_SYSTEM_C_H
#include <stddef.h> //size_t

// Interface we expect plugins to use in order to interace with the event system.
// This interface is design to avoid using c++ vtables and the C++ abi isn't stable
// across compilers.

#ifdef _cplusplus
extern "C" {
#endif

// Obnoxious long names to void namespace conflicts
enum EUICEventSystemEventTypes {
    EUICEventSystemEventTypesNoData = 0,
    EUICEventSystemEventTypesString = 1,
    EUICEventSystemEventTypesNumber = 2,
};

typedef const char *TUICEventSystemStr;

typedef struct _SUICEventSystemEventValue
{
    EUICEventSystemEventTypes m_Type;
    double m_Number;
    TUICEventSystemStr m_String;
} SUICEventSystemEventValue;

typedef struct _SUICEventSystemRegisteredStr
{
    TUICEventSystemStr m_Data;
} SUICEventSystemRegisteredStr;

typedef struct _SUICEventSystemEventData
{
    SUICEventSystemRegisteredStr m_Name;
    SUICEventSystemEventValue m_Value;
} SUICEventSystemEventData;

typedef struct _SUICEventSystemEvent
{
    SUICEventSystemEventData *m_Data; // contiguous block of event data.
    int m_NumData;
} SUICEventSystemEvent;

typedef SUICEventSystemEvent *(*TUICEventSystemEventFactoryCreateEventFn)(void *inFactory,
                                                                          int numData);
typedef size_t (*TUICEventSystemEventFactoryMaxNumEventDataFn)(void *inFactory);
typedef size_t (*TUICEventSystemEventFactoryMaxStrLenFn)(void *inFactory);
typedef SUICEventSystemRegisteredStr (*TUICEventSystemEventFactoryRegisterStrFn)(
    void *inFactory, TUICEventSystemStr inStr);
typedef TUICEventSystemStr (*TUICEventSystemEventFactoryAllocateStrFn)(void *inFactory,
                                                                       TUICEventSystemStr inStr);
typedef TUICEventSystemStr (*TUICEventSystemEventFactoryAllocateStrLenFn)(void *inFactory, int len);

// Factory from the provider's perspective.
typedef struct _SUICEventSystemEventFactory
{
    void *m_Factory;
    TUICEventSystemEventFactoryCreateEventFn createEvent;
    TUICEventSystemEventFactoryMaxNumEventDataFn getMaxNumEventData;
    TUICEventSystemEventFactoryMaxStrLenFn getMaxStrLength;
    TUICEventSystemEventFactoryRegisterStrFn registerStr;
    TUICEventSystemEventFactoryAllocateStrFn allocateStr;
    TUICEventSystemEventFactoryAllocateStrLenFn allocateStrLen;

} SUICEventSystemEventFactory;

typedef size_t (*TUICEventSystemProviderGetNextEventsFn)(void *inProvider,
                                                         SUICEventSystemEventFactory inFactory,
                                                         SUICEventSystemEvent **outBuffer,
                                                         size_t outBufLen);
typedef void (*TUICEventSystemProviderReleaseFn)(void *inProvider);

typedef struct _SUICEventSystemEventProvider
{
    void *m_Provider;
    TUICEventSystemProviderGetNextEventsFn getNextEvents;
    TUICEventSystemProviderReleaseFn release;
} SUICEventSystemEventProvider;

typedef void (*TUICEventSystemPollerAddProviderFn)(void *inPoller,
                                                   SUICEventSystemEventProvider inProvider);
typedef void (*TUICEventSystemPollerRemoveProviderFn)(void *inPoller,
                                                      SUICEventSystemEventProvider inProvider);
typedef int (*TUICEventSystemPollerignoreProviderFn)(void *inPoller,
                                                     SUICEventSystemEventProvider inProvider);
typedef int (*TUICEventSystemPolleractivateProviderFn)(void *inPoller,
                                                       SUICEventSystemEventProvider inProvider);

typedef struct _SUICEventSystemEventPoller
{
    void *m_Poller;
    // Providers, once added, will be released by the event poller unless removed
    TUICEventSystemPollerAddProviderFn addProvider;
    // Callers are responsible for releasing removed providers.
    TUICEventSystemPollerRemoveProviderFn removeProvider;
    TUICEventSystemPollerignoreProviderFn ignoreProvider;
    TUICEventSystemPolleractivateProviderFn activateProvider;

} SUICEventSystemEventPoller;

extern const char *PROVIDER_TABLE_ENTRY;
extern const char *POLLER_TABLE_ENTRY;

#ifdef _cplusplus
}
#endif

#endif