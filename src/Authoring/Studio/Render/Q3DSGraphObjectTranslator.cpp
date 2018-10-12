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

#include "Q3DSGraphObjectTranslator.h"
#include "Q3DSTranslation.h"
#include "Q3DSStringTable.h"

#include "Qt3DSString.h"
#include "IDocumentReader.h"

namespace Q3DStudio
{

QMap<Q3DSGraphObject *, Q3DSGraphObjectTranslator *> Q3DSGraphObjectTranslator::s_translatorMap;

Q3DSGraphObjectTranslator::Q3DSGraphObjectTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                     Q3DSGraphObject &inObj)
    : m_graphObject(&inObj)
    , m_instanceHandle(inInstance)
    , m_dirtyIndex(std::numeric_limits<unsigned int>::max())
{
    s_translatorMap.insert(m_graphObject, this);
}

void Q3DSGraphObjectTranslator::pushTranslation(Q3DSTranslation &translation)
{
    Q3DStudio::CString theId = translation.reader().GetFileId(instanceHandle());
    if (theId.size() && theId.toQString() != graphObject().id()) {
        qt3ds::foundation::CRegisteredString rid
                = Q3DSStringTable::instance()->GetRenderStringTable()
                    .RegisterStr(theId.toQString());
        QByteArray data = rid.qstring().toLatin1();
        translation.presentation()->registerObject(data, &graphObject());
    }
    setDirty(false);
}

bool Q3DSGraphObjectTranslator::updateProperty(Q3DSTranslation &context,
                                               qt3dsdm::Qt3DSDMInstanceHandle instance,
                                               qt3dsdm::Qt3DSDMPropertyHandle property,
                                               qt3dsdm::SValue &value,
                                               const QString &name)
{
    Q_UNUSED(context)
    Q_UNUSED(instance)
    Q_UNUSED(property)
    Q3DSPropertyChangeList changeList;
    if (name == QLatin1String("name"))
        changeList.append(m_graphObject->setName(value.toQVariant().toString()));
    else if (name == QLatin1String("starttime"))
        changeList.append(m_graphObject->setStartTime(value.getData<qint32>()));
    else if (name == QLatin1String("endtime"))
        changeList.append(m_graphObject->setEndTime(value.getData<qint32>()));
    if (changeList.count())
        m_graphObject->notifyPropertyChanges(changeList);
    return changeList.count() > 0;
}

void Q3DSGraphObjectTranslator::copyProperties(Q3DSGraphObjectTranslator *targetTranslator)
{
    Q3DSPropertyChangeList changeList;
    Q3DSGraphObject *target = &targetTranslator->graphObject();
    changeList.append(target->setName(graphObject().name()));
    changeList.append(target->setStartTime(graphObject().startTime()));
    changeList.append(target->setEndTime(graphObject().endTime()));
    target->notifyPropertyChanges(changeList);
}

Q3DSGraphObjectTranslator *Q3DSGraphObjectTranslator::translatorForObject(Q3DSGraphObject *object)
{
    return s_translatorMap.value(object, nullptr);
}

void Q3DSAliasedTranslator::pushTranslation(Q3DSTranslation &translation)
{
    // copy object values from the referenced graph object
    Q3DSGraphObjectTranslator *reference = translation.getOrCreateTranslator(instanceHandle());

    reference->copyProperties(this);

    setDirty(false);
}

}
