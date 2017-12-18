/****************************************************************************
**
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

#ifndef QT3DSRENDERDATAINPUT_H
#define QT3DSRENDERDATAINPUT_H

#include "Qt3DSRenderGraphObject.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSLogging.h"

namespace qt3ds {
namespace render {

    struct SScene;
    struct SDataInput : public SGraphObject
    {
        SDataInput();

        // DataInput should always be at the root of the scene
        SScene *m_Scene;

        QT3DSF32 m_Value;
        CRegisteredString m_ValueStr;
        CRegisteredString m_ControlledElemProp;

        QT3DSI32 m_TimeFrom;
        QT3DSI32 m_TimeTo;

        SDataInput *m_NextSibling;

        // List of "target element" - "property"
        // - pairs that this DataInput controls
        QVector<QPair<CRegisteredString, CRegisteredString>> m_ControlledElementsProperties;

        void AddControlledElementProperty(QPair<CRegisteredString, CRegisteredString> pair);
        void SetControlledElementProperties(
            QVector<QPair<CRegisteredString, CRegisteredString>> vec)
        {
            m_ControlledElementsProperties = vec;
        }
        QVector<QPair<CRegisteredString, CRegisteredString>> GetControlledElementsProperties();

        // Generic method used during serialization
        // to remap string and object pointers
        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SGraphObject::Remap(inRemapper);
            inRemapper.Remap(m_Scene);
            inRemapper.Remap(m_ValueStr);
            inRemapper.Remap(m_NextSibling);
            inRemapper.Remap(m_ControlledElemProp);
        }
    };
}
}
#endif // QT3DSRENDERDATAINPUT_H
