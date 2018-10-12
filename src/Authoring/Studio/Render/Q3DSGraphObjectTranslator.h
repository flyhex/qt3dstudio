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

    virtual void pushTranslation(Q3DSTranslation &inTranslatorContext);
    virtual void setActive(bool inActive) = 0;
    virtual void clearChildren() = 0;
    virtual void appendChild(Q3DSGraphObject &inChild) = 0;
    virtual void resetEffect() {}
    virtual Q3DSGraphObject &graphObject() { return *m_graphObject; }
    virtual Q3DSGraphObject &nonAliasedGraphObject() { return *m_graphObject; }
    virtual qt3dsdm::Qt3DSDMInstanceHandle instanceHandle() { return m_instanceHandle; }
    virtual qt3dsdm::Qt3DSDMInstanceHandle sceneGraphInstanceHandle()
    {
        return m_instanceHandle;
    }
    virtual qt3dsdm::Qt3DSDMInstanceHandle possiblyAliasedInstanceHandle()
    {
        if (m_aliasInstanceHandle.Valid())
            return m_aliasInstanceHandle;
        return instanceHandle();
    }
    virtual void copyProperties(Q3DSGraphObjectTranslator *targetTranslator);
    void enableAutoUpdates(bool enable)
    {
        m_autoUpdate = enable;
    }
    bool isAutoUpdateEnabled() const
    {
        return m_autoUpdate;
    }
    quint32 dirtyIndex() const
    {
        return m_dirtyIndex;
    }
    void setDirtyIndex(quint32 index)
    {
        m_dirtyIndex = index;
    }
    void setAliasInstanceHandle(qt3dsdm::Qt3DSDMInstanceHandle a)
    {
        m_aliasInstanceHandle = a;
    }
    qt3dsdm::Qt3DSDMInstanceHandle aliasInstanceHandle() const
    {
        return m_aliasInstanceHandle;
    }
    virtual bool updateProperty(Q3DSTranslation &inContext,
                                qt3dsdm::Qt3DSDMInstanceHandle instance,
                                qt3dsdm::Qt3DSDMPropertyHandle property,
                                qt3dsdm::SValue &value,
                                const QString &name);
    bool dirty() const
    {
        return m_dirty;
    }
    void setDirty(bool dirty)
    {
        m_dirty = dirty;
    }
    bool ignoreReferenced() const
    {
        return m_ignoreReferenced;
    }
    void setIgnoreReferenced(bool ignore)
    {
        m_ignoreReferenced = ignore;
    }
    static Q3DSGraphObjectTranslator *translatorForObject(Q3DSGraphObject *object);

    template <typename T>
    T *graphObject() const
    {
        return static_cast<T *>(m_graphObject);
    }

    void pushTranslationIfDirty(Q3DSTranslation &inTranslatorContext)
    {
        if (m_dirty)
            pushTranslation(inTranslatorContext);
    }

private:

    // This will never be null.  The reason it is a pointer is because
    // alias translators need to switch which graph object they point to
    Q3DSGraphObject *m_graphObject;
    qt3dsdm::Qt3DSDMInstanceHandle m_instanceHandle;
    qt3dsdm::Qt3DSDMInstanceHandle m_aliasInstanceHandle;

    bool m_dirty = true;
    bool m_autoUpdate = true;
    bool m_ignoreReferenced = false;
    quint32 m_dirtyIndex;
    static QMap<Q3DSGraphObject *, Q3DSGraphObjectTranslator *> s_translatorMap;
};

class Q3DSAliasedTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSAliasedTranslator(Q3DSGraphObjectTranslator *aliasTranslator,
                          qt3dsdm::Qt3DSDMInstanceHandle inInstance, Q3DSGraphObject &inObj)
        : Q3DSGraphObjectTranslator(inInstance, inObj), m_aliasTranslator(aliasTranslator)
    {
        setAliasInstanceHandle(m_aliasTranslator->instanceHandle());
    }

    Q3DSGraphObjectTranslator *aliasTranslator() const
    {
        return m_aliasTranslator;
    }

    void setActive(bool) override
    {
    }
    void clearChildren() override
    {
    }
    void appendChild(Q3DSGraphObject &) override
    {
    }

    void pushTranslation(Q3DSTranslation &inTranslatorContext) override;

private:
    Q3DSGraphObjectTranslator *m_aliasTranslator = nullptr;
};

}

#endif
