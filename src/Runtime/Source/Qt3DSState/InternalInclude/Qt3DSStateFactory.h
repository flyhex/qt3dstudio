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

namespace qt3ds {
namespace state {
    class IVisualStateContext;
}
}

namespace qt3ds {
namespace evt {
    class IEventSystem;
}
}

namespace qt3ds {
class NVFoundationBase;
namespace foundation {
    class IStringTable;
    class IPerfTimer;
    class CRegisteredString;
}
}

namespace Q3DStudio {
class ITimeProvider;
}

namespace qt3ds {
namespace state {

    struct SNDDStateContext;
    class INDDStateScriptBridge;
    class IInputStreamFactory;
    class INDDStateApplication;
    class IVisualStateInterpreterFactory;
    class IStateInterpreter;

    class INDDStateFactory : public qt3ds::foundation::NVRefCounted
    {
    public:
        class IStateInterpreterCreateCallback
        {
        public:
            virtual ~IStateInterpreterCreateCallback() {}
            virtual void OnCreate(const qt3ds::foundation::CRegisteredString &inId,
                                  IStateInterpreter &inStateInterpreter) = 0;
        };
        virtual qt3ds::state::INDDStateScriptBridge &GetScriptEngine() = 0;
        virtual qt3ds::state::IVisualStateContext &GetVisualStateContext() = 0;
        virtual qt3ds::evt::IEventSystem &GetEventSystem() = 0;
        virtual qt3ds::NVFoundationBase &GetFoundation() = 0;
        virtual qt3ds::foundation::IStringTable &GetStringTable() = 0;
        virtual Q3DStudio::ITimeProvider &GetTimeProvider() = 0;
        virtual qt3ds::state::IInputStreamFactory &GetInputStreamFactory() = 0;

        virtual qt3ds::state::INDDStateApplication *GetApplication() = 0;
        virtual void SetApplication(qt3ds::state::INDDStateApplication *inApplication) = 0;
        virtual void SetStateInterpreterCreateCallback(
            IStateInterpreterCreateCallback &inStateInterpreterCreateCallback) = 0;

        static INDDStateFactory &Create(SNDDStateContext &inContext);
    };
}
}
