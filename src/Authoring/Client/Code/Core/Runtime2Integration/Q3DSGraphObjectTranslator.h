/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef Q3DS_GRAPH_OBJECT_TRANSLATOR_H
#define Q3DS_GRAPH_OBJECT_TRANSLATOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "q3dsruntime2api_p.h"
#include "Qt3DSDMMetaData.h"

/* This class replace SGraphObjectTranslator */

namespace Q3DStudio {

class Q3DSTranslation;

class Q3DSGraphObjectTranslator
{
public:
    Q3DSGraphObjectTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, Q3DSGraphObject &inObj);

    virtual ~Q3DSGraphObjectTranslator() {}

    virtual void PushTranslation(Q3DSTranslation &inTranslatorContext);
    virtual void AfterRenderGraphIsBuilt(Q3DSTranslation &) {}
    virtual void SetActive(bool inActive) = 0;
    virtual void ClearChildren() = 0;
    virtual void AppendChild(Q3DSGraphObject &inChild) = 0;
    virtual void ResetEffect() {}
    virtual Q3DSGraphObject &GetGraphObject() { return *m_graphObject; }
    virtual Q3DSGraphObject &GetNonAliasedGraphObject() { return *m_graphObject; }
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetInstanceHandle() { return m_instanceHandle; }
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetSceneGraphInstanceHandle()
    {
        return m_instanceHandle;
    }
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetPossiblyAliasedInstanceHandle()
    {
        if (m_aliasInstanceHandle.Valid())
            return m_aliasInstanceHandle;
        return GetInstanceHandle();
    }

private:

    // This will never be null.  The reason it is a pointer is because
    // alias translators need to switch which graph object they point to
    Q3DSGraphObject *m_graphObject;
    qt3dsdm::Qt3DSDMInstanceHandle m_instanceHandle;
    qt3dsdm::Qt3DSDMInstanceHandle m_aliasInstanceHandle;

    unsigned int m_dirtyIndex;
    static QMap<Q3DSGraphObjectTranslator *, Q3DSGraphObject *> s_translatorMap;
};

}

#endif
