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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
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
    void setActive(bool inActive) override;
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
    void setEditLightEnabled(bool enabled);
private:
    bool m_editLightEnabled = false;
    bool m_activeState = false;
};

class Q3DSModelTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSModelTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSModelNode &model);
    bool canAddChild(Q3DSGraphObjectTranslator *child);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void appendChild(Q3DSGraphObject &inChild) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
    bool shaderRequiresRecompilation(Q3DSTranslation &inContext,
                                     const qt3dsdm::SValue &value,
                                     const QString &name,
                                     qt3dsdm::AdditionalMetaDataType::Value type) override;
private:
    static const QSet<QString> s_recompileProperties;
    QSet<QString> m_specifiedImageMaps;
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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;

    Q3DSReferencedMaterial *referenced() const
    {
        return graphObject<Q3DSReferencedMaterial>();
    }
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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
    void setEditLayerEnabled(Q3DSTranslation &inContext, bool enabled);
private:
    bool m_editLayerEnabled = false;

    bool m_activeDepthTestDisabled = false;
    bool m_activeDepthPrePassDisabled = false;
    Q3DSLayerNode::ProgressiveAA m_activeProgressiveAA = Q3DSLayerNode::NoPAA;
    Q3DSLayerNode::MultisampleAA m_activeMultisampleAA = Q3DSLayerNode::NoMSAA;
    bool m_activeTemporalAAEnabled = false;
    QColor m_activeBackgroundColor;
    Q3DSLayerNode::BlendType m_activeBlendType = Q3DSLayerNode::Normal;
    Q3DSLayerNode::LayerBackground m_activeLayerBackground = Q3DSLayerNode::Transparent;
    QString m_activeSourcePath;
    Q3DSLayerNode::HorizontalFields m_activeHorizontalFields = Q3DSLayerNode::LeftWidth;
    float m_activeLeft = 0.f;
    Q3DSLayerNode::Units m_activeLeftUnits = Q3DSLayerNode::Percent;
    float m_activeWidth = 0.f;
    Q3DSLayerNode::Units m_activeWidthUnits = Q3DSLayerNode::Percent;
    float m_activeRight = 0.f;
    Q3DSLayerNode::Units m_activeRightUnits = Q3DSLayerNode::Percent;
    float m_activeTop = 0.f;
    Q3DSLayerNode::VerticalFields m_activeVerticalFields = Q3DSLayerNode::TopHeight;
    Q3DSLayerNode::Units m_activeTopUnits = Q3DSLayerNode::Percent;
    float m_activeHeight = 0.f;
    Q3DSLayerNode::Units m_activeHeightUnits = Q3DSLayerNode::Percent;
    float m_activeBottom = 0.f;
    Q3DSLayerNode::Units m_activeBottomUnits = Q3DSLayerNode::Percent;
    float m_activeAoStrength = 0.f;
    float m_activeAoDistance = 0.f;
    float m_activeAoSoftness = 0.f;
    float m_activeAoBias = 0.f;
    int m_activeAoSampleRate = 0;
    bool m_activeAoDither = false;
    float m_activeShadowStrength = 0.f;
    float m_activeShadowDist = 0.f;
    float m_activeShadowSoftness = 0.f;
    float m_activeShadowBias = 0.f;
    Q3DSImage *m_activeLightProbe = nullptr;
    float m_activeProbeBright = 0.f;
    bool m_activeFastIBLEnabled = false;
    float m_activeProbeHorizon = 0.f;
    float m_activeProbeFov = 0.f;
    Q3DSImage *m_activeLightProbe2 = nullptr;
    float m_activeProbe2Fade = 0.f;
    float m_activeProbe2Window = 0.f;
    float m_activeProbe2Pos = 0.f;
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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
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
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
};

class Q3DSDynamicObjectTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSDynamicObjectTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                Q3DSGraphObject &graphObject);
    void pushTranslation(Q3DSTranslation &inContext) override;
    void setActive(bool) override;
    void clearChildren() override;
    void appendChild(Q3DSGraphObject &) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
};

class Q3DSEffectTranslator : public Q3DSDynamicObjectTranslator
{
public:
    Q3DSEffectTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSEffectInstance &effect);
    void pushTranslation(Q3DSTranslation &inContext) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
    bool shaderRequiresRecompilation(Q3DSTranslation &inContext,
                                     const  qt3dsdm::SValue &value,
                                     const QString &name,
                                     qt3dsdm::AdditionalMetaDataType::Value type) override;
};

class Q3DSCustomMaterialTranslator : public Q3DSDynamicObjectTranslator
{
public:
    Q3DSCustomMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                 Q3DSCustomMaterialInstance &material);
    void pushTranslation(Q3DSTranslation &inContext) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
    bool shaderRequiresRecompilation(Q3DSTranslation &inContext,
                                     const qt3dsdm::SValue &value,
                                     const QString &name,
                                     qt3dsdm::AdditionalMetaDataType::Value type) override;
};

class Q3DSAliasTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSAliasTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSNode &aliasNode);
    void pushTranslation(Q3DSTranslation &inContext) override;
    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override;
    void copyProperties(Q3DSGraphObject *target, bool ignoreReferenced) override;
    void addToSlide(Q3DSSlide *slide);
private:
    void createTranslatorsRecursive(Q3DSTranslation &inContext,
                                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                                    Q3DSGraphObjectTranslator *translator);
    void addToSlideRecursive(Q3DSGraphObjectTranslator *translator, Q3DSSlide *slide);

    Q3DSGraphObjectTranslator *m_referencedTree = nullptr;
    qt3dsdm::Qt3DSDMInstanceHandle m_referencedInstance = qt3dsdm::Qt3DSDMInstanceHandle();
};

}

#endif
