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

#ifndef Q3DS_TRANSLATORS_H
#define Q3DS_TRANSLATORS_H

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

#include "Q3DSGraphObjectTranslator.h"

namespace Q3DStudio {

class Q3DSTranslation;

class Q3DSNodeTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSNode &node);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void appendChild(Q3DSGraphObject &inChild) override;
    void clearChildren() override;
    void setActive(bool inActive) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value, const QString &name) override;
};

class Q3DSGroupNodeTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSGroupNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSGroupNode &node);
};

class Q3DSComponentNodeTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSComponentNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                Q3DSComponentNode &component);
};

class Q3DSSceneTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSSceneTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSScene &scene);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void appendChild(Q3DSGraphObject &inChild) override;
    void clearChildren() override;
    void setActive(bool) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
};

class Q3DSCameraTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSCameraTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSCameraNode &camera);
    void pushTranslation(Q3DSTranslation &inContext) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
    void setActive(bool inActive) override;
    void setEditCameraEnabled(bool enabled);
private:
    bool m_editCameraEnabled = false;
    bool m_activeState = false;
};

class Q3DSLightTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSLightTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSLightNode &light);
    void pushTranslation(Q3DSTranslation &inContext) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
};

class Q3DSModelTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSModelTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSModelNode &model);
    bool canAddChild(Q3DSGraphObjectTranslator *child);
    void pushTranslation(Q3DSTranslation &inContext) override;
    bool isMaterial(const Q3DSGraphObject &inChild) const;
    void appendChild(Q3DSGraphObject &inChild) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
};

class Q3DSImageTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSImageTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSImage &image);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void appendChild(Q3DSGraphObject &) override;
    void setActive(bool) override;
    void clearChildren() override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
};

class Q3DSDefaultMaterialTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSDefaultMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                  Q3DSDefaultMaterial &material);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void setActive(bool) override;
    void clearChildren() override;
    void appendChild(Q3DSGraphObject &) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
};

class Q3DSReferencedMaterialTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSReferencedMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                     Q3DSReferencedMaterial &material);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void setActive(bool) override;
    void clearChildren() override;
    void appendChild(Q3DSGraphObject &) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
};

class Q3DSLayerTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSLayerTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSLayerNode &layer);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void appendChild(Q3DSGraphObject &inChild) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
};

class Q3DSSlideTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSSlideTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSSlide &slide);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void appendChild(Q3DSGraphObject &inChild) override;
    void clearChildren() override;
    void setActive(bool) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
    bool masterSlide() const;
    void setMasterSlide(bool master);

private:
    bool m_isMaster;
};

class Q3DSTextTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSTextTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSTextNode &node);
    void pushTranslation(Q3DSTranslation &inContext) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
};

}

#endif
