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

#pragma once

#include "foundation/Qt3DSRefCounted.h"

#include "Qt3DSStateConfig.h"

namespace qt3ds {

class NVFoundation;

namespace foundation {
    class IStringTable;
}
}

namespace Q3DStudio {
class ITimeProvider;
}

namespace qt3ds {
namespace state {

    class IInputStreamFactory;

    class INDDStateEngine
    {
    public:
        enum EStateEvent {
            STATE_EVENT_UNKNOWN = 0,
            STATE_EVENT_STATE_ENTER,
            STATE_EVENT_STATE_EXIT,
            STATE_EVENT_TRANSITION,
        };
        class IStateEventHandler
        {
        public:
            virtual ~IStateEventHandler() {}
            virtual void OnEvent() = 0;
        };
        class IStateEventHandlerConnection : public qt3ds::foundation::NVRefCounted
        {
        protected:
            virtual ~IStateEventHandlerConnection() {}
        public:
        };

        typedef qt3ds::foundation::NVScopedRefCounted<IStateEventHandlerConnection>
            TStateEventHandlerConnectionPtr;

    public:
        virtual ~INDDStateEngine() {}

        virtual bool Load(const char *inApplicationPath) = 0;
        virtual bool Step() = 0;
        virtual TStateEventHandlerConnectionPtr
        RegisterEventHandler(EStateEvent inEvent, const char *inEventId,
                             IStateEventHandler &inHandler) = 0;
        virtual void FireEvent(const char *inEventName, unsigned long long inDelay,
                               const char *inCancelId, bool inIsExternal = true) = 0;
        virtual void FireEvent(const char *inEventName, bool inIsExternal = true) = 0;
        virtual void CancelEvent(const char *inCancelId) = 0;

        QT3DS_STATE_API static INDDStateEngine *
        Create(qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> inFoundation,
               qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> inStringTable,
               qt3ds::foundation::NVScopedRefCounted<qt3ds::state::IInputStreamFactory>
                   inInputStreamFactory,
               Q3DStudio::ITimeProvider &inTimeProvider);

        QT3DS_STATE_API static INDDStateEngine *Create();
    };
}
}
