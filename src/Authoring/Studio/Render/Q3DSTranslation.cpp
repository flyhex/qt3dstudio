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

#include "Q3DSTranslation.h"
#include "Q3DStudioRenderer.h"
#include "Q3DSStringTable.h"
#include "Q3DSGraphObjectTranslator.h"
#include "Q3DSEditCamera.h"
#include "Q3DSInputStreamFactory.h"
#include "Q3DSTranslators.h"

#include "StudioApp.h"
#include "Core.h"
#include "ClientDataModelBridge.h"
#include "StudioFullSystem.h"
#include "IDocumentReader.h"
#include "StudioProjectSettings.h"
#include "SlideSystem.h"

#include <QtCore/qmath.h>

namespace Q3DStudio {

Q3DSTranslation::Q3DSTranslation(Q3DStudioRenderer &inRenderer,
                                 const QSharedPointer<Q3DSUipPresentation> &presentation)
    : m_studioRenderer(inRenderer)
    , m_doc(*g_StudioApp.GetCore()->GetDoc())
    , m_reader(m_doc.GetDocumentReader())
    , m_objectDefinitions(
          m_doc.GetStudioSystem()->GetClientDataModelBridge()->GetObjectDefinitions())
    , m_studioSystem(*m_doc.GetStudioSystem())
    , m_fullSystem(*m_doc.GetStudioSystem()->GetFullSystem())
    , m_assetGraph(*m_doc.GetAssetGraph())
    , m_engine(inRenderer.engine())
    , m_presentation(presentation)
{
    qt3dsdm::Qt3DSDMInstanceHandle sceneRoot = m_assetGraph.GetRoot(0);
    m_graphIterator.ClearResults();
    m_assetGraph.GetDepthFirst(m_graphIterator, sceneRoot);
    for (; !m_graphIterator.IsDone(); ++m_graphIterator) {
        qt3dsdm::Qt3DSDMInstanceHandle instance(m_graphIterator.GetCurrent());
        getOrCreateTranslator(instance);
    }

    std::shared_ptr<qt3dsdm::ISlideCore> slideCore = m_fullSystem.GetSlideCore();
    std::shared_ptr<qt3dsdm::ISlideSystem> slideSystem = m_fullSystem.GetSlideSystem();
    qt3dsdm::TSlideHandleList slideList;
    slideCore->GetSlides(slideList);

    // Pass 1. Create slide translators
    for (qt3dsdm::TSlideHandleList::iterator it = slideList.begin(); it < slideList.end(); ++it) {
        qt3dsdm::Qt3DSDMInstanceHandle instance(slideCore->GetSlideInstance(*it));
        qt3dsdm::Qt3DSDMInstanceHandle component(m_reader.GetComponentForSlide(*it));
        Q3DSGraphObjectTranslator *translator = getOrCreateTranslator(instance);
        Q3DSSlide &slide = static_cast<Q3DSSlide&>(translator->graphObject());

        if (component.Valid()) {
            m_slideTranslatorMap.insert(instance, translator);
            Q3DSGraphObjectTranslator *componentTranslator = getOrCreateTranslator(component);

            if (m_reader.IsMasterSlide(*it)) {
                m_masterSlideMap.insert(component, translator);
                if (componentTranslator->graphObject().type() == Q3DSGraphObject::Scene) {
                    m_presentation->setMasterSlide(&slide);
                } else {
                    static_cast<Q3DSComponentNode &>(componentTranslator->graphObject())
                        .setMasterSlide(&slide);
                }
            }
        }
        qt3dsdm::TInstanceHandleList slideInstances;
        slideSystem->GetAssociatedInstances(*it, slideInstances);
        for (unsigned int i = 0; i < slideInstances.size(); ++i) {
            qt3dsdm::Qt3DSDMInstanceHandle inst(slideInstances[i]);
            if (m_translatorMap.contains(inst)) {
                THandleTranslatorPairList &theList = *m_translatorMap.find(inst);
                ThandleTranslatorOption t
                        = findTranslator(theList, qt3dsdm::Qt3DSDMInstanceHandle());
                if (t.hasValue() && t->second->graphObject().type() != Q3DSGraphObject::Slide)
                    slide.addObject(&t->second->graphObject());
            }
        }
    }

    // Pass 2. Add child slides to master slides
    for (qt3dsdm::TSlideHandleList::iterator it = slideList.begin(); it < slideList.end(); ++it) {
        qt3dsdm::Qt3DSDMInstanceHandle instance(slideCore->GetSlideInstance(*it));
        qt3dsdm::Qt3DSDMInstanceHandle component(m_reader.GetComponentForSlide(*it));
        Q3DSGraphObjectTranslator *translator = getOrCreateTranslator(instance);
        Q3DSSlide &slide = static_cast<Q3DSSlide &>(translator->graphObject());

        if (component.Valid() && getOrCreateTranslator(component)) {
            m_slideTranslatorMap.insert(instance, translator);
            if (!m_reader.IsMasterSlide(*it)) {
                Q3DSGraphObjectTranslator *slideTranslator = m_masterSlideMap[component];
                slideTranslator->graphObject().appendChildNode(&slide);
            }
        }
    }

    qt3dsdm::IStudioFullSystemSignalProvider *theProvider = m_fullSystem.GetSignalProvider();
    m_signalConnections.push_back(
        theProvider->ConnectInstanceCreated(
                    std::bind(static_cast<void(Q3DSTranslation::*)(qt3dsdm::Qt3DSDMInstanceHandle)>
                              (&Q3DSTranslation::markDirty), this, std::placeholders::_1)));
    m_signalConnections.push_back(theProvider->ConnectInstanceDeleted(
        std::bind(&Q3DSTranslation::releaseTranslation, this, std::placeholders::_1)));
    m_signalConnections.push_back(
        theProvider->ConnectInstancePropertyValue(std::bind(&Q3DSTranslation::markPropertyDirty,
                                                            this, std::placeholders::_1,
                                                            std::placeholders::_2)));
    m_signalConnections.push_back(m_assetGraph.ConnectChildAdded(
        std::bind(&Q3DSTranslation::markGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_signalConnections.push_back(m_assetGraph.ConnectChildMoved(
        std::bind(&Q3DSTranslation::markGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_signalConnections.push_back(m_assetGraph.ConnectChildRemoved(
        std::bind(&Q3DSTranslation::markGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_signalConnections.push_back(theProvider->ConnectBeginComponentSeconds(
        std::bind(&Q3DSTranslation::markBeginComponentSeconds, this, std::placeholders::_1)));
    m_signalConnections.push_back(theProvider->ConnectComponentSeconds(
        std::bind(&Q3DSTranslation::markComponentSeconds, this, std::placeholders::_1)));

    clearDirtySet();
}

Q3DSTranslation::THandleTranslatorPairList &Q3DSTranslation::getTranslatorsForInstance(
        qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    TInstanceToTranslatorMap::iterator theTranslatorList;
    if (!m_translatorMap.contains(instance))
        theTranslatorList = m_translatorMap.insert(instance, THandleTranslatorPairList());
    else
        theTranslatorList = m_translatorMap.find(instance);
    return *theTranslatorList;
}

void Q3DSTranslation::markDirty(qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    // Anchor points are not handled individually.
    if (m_reader.GetObjectTypeName(instance) == QLatin1String("PathAnchorPoint"))
        instance = m_assetGraph.GetParent(instance);
    getOrCreateTranslator(instance);

    THandleTranslatorPairList &theTranslators = getTranslatorsForInstance(instance);
    for (int idx = 0, end = theTranslators.size(); idx < end; ++idx)
        m_dirtySet.insert(*theTranslators[idx].second);

    m_studioRenderer.RequestRender();
}

void Q3DSTranslation::recompileShadersIfRequired(Q3DSGraphObjectTranslator *translator,
                                                 const SValue &value, const QString &name,
                                                 qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                 qt3dsdm::Qt3DSDMPropertyHandle property)
{
    qt3dsdm::IPropertySystem *prop = m_doc.GetPropertySystem();
    if (((translator->isMaterial(translator->graphObject())
         && translator->graphObject().type() != Q3DSGraphObject::ReferencedMaterial)
            || translator->graphObject().type() == Q3DSGraphObject::Effect)
            && translator->shaderRequiresRecompilation(*this, value, name,
                prop->GetAdditionalMetaDataType(instance, property))) {
        QByteArray newId = getInstanceObjectId(instance);
        Q3DSGraphObject *oldObject = &translator->graphObject();
        Q3DSGraphObject *newObject = &translator->graphObject();
        switch (oldObject->type()) {
        case Q3DSGraphObject::DefaultMaterial:
            newObject = m_presentation->newObject<Q3DSDefaultMaterial>(newId);
            break;
        case Q3DSGraphObject::CustomMaterial:
            newObject = m_presentation->newObject<Q3DSCustomMaterialInstance>(newId);
            break;
        case Q3DSGraphObject::Effect:
            newObject = m_presentation->newObject<Q3DSEffectInstance>(newId);
            break;
        default:
            break;
        }
        if (!newObject) {
            Q_ASSERT_X(newObject, __FUNCTION__, "GraphObject creation failed");
            return;
        }
        Q3DSGraphObjectTranslator *slideTranslator = nullptr;
        m_instanceIdHash[instance] = newId;
        qt3dsdm::Qt3DSDMSlideHandle slideHandle(m_reader.GetAssociatedSlide(instance));
        if (slideHandle.Valid()) {
            std::shared_ptr<qt3dsdm::ISlideCore> slideCore = m_fullSystem.GetSlideCore();
            qt3dsdm::Qt3DSDMInstanceHandle slideInstance(slideCore->GetSlideInstance(slideHandle));
            if (slideInstance.Valid() && m_slideTranslatorMap.contains(slideInstance))
                slideTranslator = m_slideTranslatorMap[slideInstance];
        }
        QVector<Q3DSReferencedMaterial *> updateList;
        for (auto &refMat : qAsConst(m_refMatTranslators)) {
            if (refMat->referenced()->referencedMaterial() == oldObject)
                updateList << refMat->referenced();
        }
        Q3DSGraphObject *parent = oldObject->parent();
        translator->copyProperties(newObject, false);
        oldObject->reparentChildNodesTo(newObject);
        m_presentation->unlinkObject(oldObject);
        parent->appendChildNode(newObject);
        delete oldObject;
        for (auto &refMat : qAsConst(updateList)) {
            Q3DSPropertyChangeList list;
            list.append(refMat->setReferencedMaterial(newObject));
            refMat->notifyPropertyChanges(list);
        }
        if (slideTranslator) {
            Q3DSSlide *slide = slideTranslator->graphObject<Q3DSSlide>();
            slide->addObject(newObject);
        }
        translator->setGraphObject(newObject);
    }
}

void Q3DSTranslation::markPropertyDirty(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                        qt3dsdm::Qt3DSDMPropertyHandle property)
{
    SValue value;
    qt3dsdm::IPropertySystem *prop = m_doc.GetPropertySystem();
    prop->GetInstancePropertyValue(instance, property, value);
    QString name = prop->GetName(property);

    // ignore these properties
    if (name == QLatin1String("shy"))
        return;

    TInstanceToTranslatorMap::iterator theTranslatorList;
    if (!m_translatorMap.contains(instance))
        theTranslatorList = m_translatorMap.insert(instance, THandleTranslatorPairList());
    else
        theTranslatorList = m_translatorMap.find(instance);
    THandleTranslatorPairList &theList = *theTranslatorList;
    ThandleTranslatorOption t = findTranslator(theList, qt3dsdm::Qt3DSDMInstanceHandle());
    if (!t.isEmpty() && t->second->isAutoUpdateEnabled()) {
        Q3DSGraphObjectTranslator *translator = t->second;
        translator->updateProperty(*this, instance, property, value, name);
        recompileShadersIfRequired(translator, value, name, instance, property);
        m_studioRenderer.RequestRender();
    }
}

void Q3DSTranslation::releaseTranslation(qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    THandleTranslatorPairList &theTranslators = getTranslatorsForInstance(instance);
    for (int idx = 0, end = theTranslators.size(); idx < end; ++idx)
        m_releaseSet.insert(*theTranslators[idx].second);

    m_studioRenderer.RequestRender();
}

void Q3DSTranslation::markGraphInstanceDirty(int instance, int)
{
    markDirty(instance);
}

void Q3DSTranslation::markBeginComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle slide)
{

}

void Q3DSTranslation::markComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle)
{

}

QByteArray Q3DSTranslation::getInstanceObjectId(qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    QByteArray ret;
    QString theId = m_reader.GetFileId(instance);
    if (theId.isEmpty())
        theId = m_reader.GetName(instance);
    if (theId.isEmpty())
        theId = qt3dsdm::ComposerObjectTypes::Convert(m_objectDefinitions.GetType(instance));

    if (!theId.isEmpty())
        ret = theId.toLatin1();

    int index = 1;
    QByteArray testId = ret;
    while (m_instanceIdHash.values().contains(testId))
        testId = ret + (QStringLiteral("_%1").arg(index++, 3, 10, QLatin1Char('0'))).toLatin1();
    ret = testId;

    // slides require component name prepended
    if (m_objectDefinitions.GetType(instance) == qt3dsdm::ComposerObjectTypes::Slide) {
        qt3dsdm::Qt3DSDMSlideHandle slide(m_fullSystem.GetSlideSystem()
                                          ->GetSlideByInstance(instance));
        qt3dsdm::Qt3DSDMInstanceHandle component(m_reader.GetComponentForSlide(slide));
        if (component.Valid()) {
            QString cId = m_reader.GetFileId(component);
            if (theId.isEmpty())
                cId = m_reader.GetName(component);
            cId.append(QLatin1String("-"));
            ret = cId.toLatin1().append(ret);
        } else if (ret.isEmpty()) {
            ret = QByteArrayLiteral("Master Slide");
        } else if (ret != QByteArrayLiteral("Master Slide")){
            ret = QByteArrayLiteral("Master-").append(ret);
        }
    }

    return ret;
}

Q3DSTranslation::ThandleTranslatorOption Q3DSTranslation::findTranslator(
        Q3DSTranslation::THandleTranslatorPairList &list,
        qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    for (THandleTranslatorPairList::Iterator it = list.begin();
         it < list.end(); ++it) {
        if (it->first == instance) {
            return Q3DSTranslation::ThandleTranslatorOption(THandleTranslatorPair(instance,
                                                                                  it->second));
        }
    }
    return Q3DSTranslation::ThandleTranslatorOption();
}

Q3DSGraphObjectTranslator *Q3DSTranslation::createEffectTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance, qt3dsdm::Qt3DSDMInstanceHandle parentClass,
        const QByteArray &id)
{
    const QString instanceName = m_reader.GetName(parentClass);
    const QString instancePath = m_reader.GetSourcePath(parentClass);
    const QString assetPath = m_presentation->assetFileName(instancePath, nullptr);
    qt3dsdm::IMetaData &metadata(*m_studioSystem.GetActionMetaData());

    if (!metadata.IsEffectInstanceRegistered(qPrintable(assetPath))) {
        auto inputStreamFactory = IInputStreamFactory::Create();
        std::vector<qt3dsdm::SMetaDataLoadWarning> warnings;
        IRefCountedInputStream stream = inputStreamFactory->getStreamForFile(assetPath);
        if (stream.isNull()) {
            qWarning() << __FUNCTION__ << " Unable to open effect: " << instancePath;
            return nullptr;
        }
        metadata.LoadEffectXMLFromSourcePath(instancePath,
                                             instance, instanceName, warnings, *stream);
    }

    Q3DSEffect effect = m_presentation->effect(assetPath);
    if (effect.isNull())
        return nullptr;

    Q3DSEffectInstance *effectInstance = m_presentation->newObject<Q3DSEffectInstance>(id);
    effectInstance->setName(instanceName);
    effectInstance->setSourcePath(instancePath);
    effectInstance->resolveReferences(*m_presentation.data());
    return new Q3DSEffectTranslator(instance, *effectInstance);
}

Q3DSGraphObjectTranslator *Q3DSTranslation::createCustomMaterialTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance, qt3dsdm::Qt3DSDMInstanceHandle parentClass,
        const QByteArray &id)
{
    const QString instanceName = m_reader.GetName(parentClass);
    const QString instancePath = m_reader.GetSourcePath(parentClass);
    const QString assetPath = m_presentation->assetFileName(instancePath, nullptr);
    qt3dsdm::IMetaData &metadata(*m_studioSystem.GetActionMetaData());

    if (!metadata.IsMaterialClassRegistered(assetPath)) {
        auto inputStreamFactory = IInputStreamFactory::Create();
        std::vector<qt3dsdm::SMetaDataLoadWarning> warnings;
        IRefCountedInputStream stream = inputStreamFactory->getStreamForFile(assetPath);
        if (stream.isNull()) {
            qWarning() << __FUNCTION__ << " Unable to open custom material: " << instancePath;
            return nullptr;
        }
        metadata.LoadMaterialClassFromSourcePath(instancePath,
                                                 instance, instanceName, warnings, *stream);
    }

    Q3DSCustomMaterial material = m_presentation->customMaterial(assetPath);
    if (material.isNull())
        return nullptr;

    Q3DSCustomMaterialInstance *materialInstance
            = m_presentation->newObject<Q3DSCustomMaterialInstance>(id);
    materialInstance->setName(instanceName);
    materialInstance->setSourcePath(instancePath);
    materialInstance->resolveReferences(*m_presentation.data());
    return new Q3DSCustomMaterialTranslator(instance, *materialInstance);
}

void Q3DSTranslation::setPresentationData()
{
    CStudioProjectSettings *settings = m_doc.GetCore()->GetStudioProjectSettings();
    m_presentation_data.m_author = settings->getAuthor();
    m_presentation_data.m_company = settings->getCompany();
    m_presentation_data.m_width = settings->getPresentationSize().width();
    m_presentation_data.m_height = settings->getPresentationSize().height();
    m_presentation_data.m_srcPath = m_doc.GetDocumentPath();

    m_presentation->setSourceFile(m_presentation_data.m_srcPath);
    m_presentation->setAuthor(m_presentation_data.m_author);
    m_presentation->setCompany(m_presentation_data.m_author);
    m_presentation->setPresentationWidth(m_presentation_data.m_width);
    m_presentation->setPresentationHeight(m_presentation_data.m_height);
    m_presentation->setMaintainAspectRatio(settings->getMaintainAspect());
    m_presentation->setPresentationRotation(settings->getRotatePresentation()
                                            ? Q3DSUipPresentation::Clockwise90
                                            : Q3DSUipPresentation::NoRotation);
}

Q3DSGraphObject *Q3DSTranslation::createAliasGraphObject(qt3dsdm::ComposerObjectTypes::Enum type,
                                                         const QByteArray &id)
{
    Q3DSGraphObject *object = nullptr;
    switch (type) {
    case qt3dsdm::ComposerObjectTypes::Group: {
        object = m_presentation->newObject<Q3DSGroupNode>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Component: {
        object = m_presentation->newObject<Q3DSComponentNode>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Scene: {
        object = m_presentation->newObject<Q3DSScene>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Layer: {
        object = m_presentation->newObject<Q3DSLayerNode>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Slide: {
        object = m_presentation->newObject<Q3DSSlide>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Camera: {
        object = m_presentation->newObject<Q3DSCameraNode>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Light: {
        object = m_presentation->newObject<Q3DSLightNode>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Model: {
        object = m_presentation->newObject<Q3DSModelNode>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Material: {
        object = m_presentation->newObject<Q3DSDefaultMaterial>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Image: {
        object = m_presentation->newObject<Q3DSImage>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Text: {
        object = m_presentation->newObject<Q3DSTextNode>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Effect: {
        object = m_presentation->newObject<Q3DSEffectInstance>(id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::CustomMaterial: {
        object = m_presentation->newObject<Q3DSCustomMaterialInstance>(id);
        break;
    }
    default:
        break;
    }
    return object;
}

Q3DSGraphObjectTranslator *Q3DSTranslation::createTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSGraphObjectTranslator *aliasTranslator)
{
    Q3DSGraphObjectTranslator *translator = nullptr;
    qt3dsdm::ComposerObjectTypes::Enum type = m_objectDefinitions.GetType(instance);
    qt3dsdm::Qt3DSDMInstanceHandle parentClass = m_reader.GetFirstBaseClass(instance);
    if (type == qt3dsdm::ComposerObjectTypes::Unknown && parentClass.Valid())
        type = m_objectDefinitions.GetType(parentClass);

    if (type == qt3dsdm::ComposerObjectTypes::Unknown)
        return nullptr;

    QByteArray id = getInstanceObjectId(instance);
    Q_ASSERT_X(!m_instanceIdHash.contains(instance), __FUNCTION__,
               "Instance translator already created");
    if (aliasTranslator) {
        // We are creating graph object for alias node tree
        // prepend id with alias id
        id.prepend(QByteArrayLiteral("_"));
        id.prepend(aliasTranslator->graphObject().id());
        translator = new Q3DSAliasedTranslator(aliasTranslator, instance,
                                               *createAliasGraphObject(type, id));
        if (translator)
            m_instanceIdHash.insert(instance, id);
        return translator;
    }

    // For the subset of possible instances, pick out the valid translators.
    switch (type) {
    case qt3dsdm::ComposerObjectTypes::Group: {
        translator = new Q3DSGroupNodeTranslator(instance,
                                                 *m_presentation->newObject<Q3DSGroupNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Component: {
        Q3DSComponentNode &component = *m_presentation->newObject<Q3DSComponentNode>(id);
        translator = new Q3DSComponentNodeTranslator(instance, component);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Scene: {
        translator = new Q3DSSceneTranslator(instance, *m_presentation->newObject<Q3DSScene>(id));
        m_scene = static_cast<Q3DSScene *>(&translator->graphObject());
        m_presentation->setScene(m_scene);
        setPresentationData();
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Layer: {
        translator = new Q3DSLayerTranslator(instance,
                                             *m_presentation->newObject<Q3DSLayerNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Slide: {
        translator = new Q3DSSlideTranslator(instance, *m_presentation->newObject<Q3DSSlide>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Camera: {
        Q3DSCameraTranslator *t
            = new Q3DSCameraTranslator(instance, *m_presentation->newObject<Q3DSCameraNode>(id));
        m_cameraTranslators.push_back(t);
        translator = t;
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Light: {
        translator = new Q3DSLightTranslator(instance,
                                             *m_presentation->newObject<Q3DSLightNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Model: {
        translator = new Q3DSModelTranslator(instance,
                                             *m_presentation->newObject<Q3DSModelNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Material: {
        Q3DSDefaultMaterial &material = *m_presentation->newObject<Q3DSDefaultMaterial>(id);
        translator = new Q3DSDefaultMaterialTranslator(instance, material);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Image: {
        translator = new Q3DSImageTranslator(instance, *m_presentation->newObject<Q3DSImage>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Text: {
        translator = new Q3DSTextTranslator(instance, *m_presentation->newObject<Q3DSTextNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::ReferencedMaterial: {
        Q3DSReferencedMaterialTranslator *t = new Q3DSReferencedMaterialTranslator(
                            instance, *m_presentation->newObject<Q3DSReferencedMaterial>(id));
        m_refMatTranslators.push_back(t);
        translator = t;
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Effect: {
        if (parentClass.Valid())
            translator = createEffectTranslator(instance, parentClass, id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::CustomMaterial: {
        if (parentClass.Valid())
            translator = createCustomMaterialTranslator(instance, parentClass, id);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Alias: {
        translator = new Q3DSAliasTranslator(instance, *m_presentation->newObject<Q3DSGroupNode>(id));
        break;
    }
    default:
        break;
    }
    if (translator)
        m_instanceIdHash.insert(instance, id);
    return translator;
}

Q3DSGraphObjectTranslator *Q3DSTranslation::getOrCreateTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    return getOrCreateTranslator(instance, qt3dsdm::Qt3DSDMInstanceHandle());
}

Q3DSGraphObjectTranslator *Q3DSTranslation::getOrCreateTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance, qt3dsdm::Qt3DSDMInstanceHandle aliasInstance,
        Q3DSGraphObjectTranslator *aliasTranslator)
{
    TInstanceToTranslatorMap::iterator theTranslatorList;
    if (!m_translatorMap.contains(instance))
        theTranslatorList = m_translatorMap.insert(instance, THandleTranslatorPairList());
    else
        theTranslatorList = m_translatorMap.find(instance);
    THandleTranslatorPairList &theList = *theTranslatorList;
    ThandleTranslatorOption theExistingTranslator = findTranslator(theList, aliasInstance);

    if (theExistingTranslator.hasValue())
        return theExistingTranslator->second;
    if (m_reader.IsInstance(instance) == false)
        return nullptr;

    Q3DSGraphObjectTranslator *theNewTranslator = createTranslator(instance, aliasTranslator);
    if (theNewTranslator != nullptr) {
        theNewTranslator->setAliasInstanceHandle(aliasInstance);
        m_dirtySet.insert(*theNewTranslator);
        theList.push_back(THandleTranslatorPair(aliasInstance, theNewTranslator));

        qt3dsdm::Qt3DSDMSlideHandle slideHandle(m_reader.GetAssociatedSlide(instance));
        if (slideHandle.Valid()) {
            std::shared_ptr<qt3dsdm::ISlideCore> slideCore = m_fullSystem.GetSlideCore();
            qt3dsdm::Qt3DSDMInstanceHandle slideInstance(slideCore->GetSlideInstance(slideHandle));
            if (slideInstance.Valid() && m_slideTranslatorMap.contains(slideInstance)) {
                Q3DSGraphObjectTranslator *translator = m_slideTranslatorMap[slideInstance];
                Q3DSSlide *slide = translator->graphObject<Q3DSSlide>();
                if (slide)
                    slide->addObject(&theNewTranslator->graphObject());
            }
        }
    }

    return theNewTranslator;
}

void Q3DSTranslation::releaseTranslator(Q3DSGraphObjectTranslator *translator)
{
    qt3dsdm::Qt3DSDMInstanceHandle instance = translator->instanceHandle();
    Q3DSGraphObject *graphObject = &translator->graphObject();

    if (static_cast<Q3DSCameraTranslator *>(translator))
        m_cameraTranslators.removeAll(static_cast<Q3DSCameraTranslator *>(translator));
    if (static_cast<Q3DSReferencedMaterialTranslator *>(translator))
        m_refMatTranslators.removeAll(static_cast<Q3DSReferencedMaterialTranslator *>(translator));

    qt3dsdm::Qt3DSDMSlideHandle slideHandle(m_reader.GetAssociatedSlide(instance));
    if (slideHandle.Valid()) {
        std::shared_ptr<qt3dsdm::ISlideCore> slideCore = m_fullSystem.GetSlideCore();
        qt3dsdm::Qt3DSDMInstanceHandle slideInstance(slideCore->GetSlideInstance(slideHandle));
        if (slideInstance.Valid() && m_slideTranslatorMap.contains(slideInstance)) {
            Q3DSGraphObjectTranslator *translator = m_slideTranslatorMap[slideInstance];
            Q3DSSlide *slide = translator->graphObject<Q3DSSlide>();
            if (slide)
                slide->removeObject(graphObject);
        }
    }

    m_instanceIdHash.remove(instance);
    m_translatorMap.remove(instance);
    m_presentation->unlinkObject(graphObject);
    delete translator;
    delete graphObject;
}

void Q3DSTranslation::clearDirtySet()
{
    for (unsigned int idx = 0; idx < m_releaseSet.size(); ++idx) {
        Q3DSGraphObjectTranslator *translator = m_releaseSet[idx];
        translator->releaseGraphObjectsRecursive(*this);
        releaseTranslator(translator);
    }
    for (unsigned int idx = 0; idx < m_dirtySet.size(); ++idx) {
        if (m_reader.IsInstance(m_dirtySet[idx]->instanceHandle())
                && m_dirtySet[idx]->dirty()) {
            m_dirtySet[idx]->pushTranslation(*this);
        }
    }
    m_releaseSet.clear();
    m_dirtySet.clear();
}

void Q3DSTranslation::prepareRender(const QRect &rect, const QSize &size)
{
    if (!m_scene)
        return;
    clearDirtySet();
    if (!m_presentationInit) {
        m_engine->setPresentation(m_presentation.data());

        const bool profileui = CStudioApp::hasProfileUI();
        m_engine->setProfileUiVisible(profileui, profileui);

        m_studioRenderer.SetViewRect(m_studioRenderer.viewRect(), size);
        m_engine->sceneManager()->slidePlayer()->setMode(Q3DSSlidePlayer::PlayerMode::Editor);
        m_engine->sceneManager()->slidePlayer()->stop();
        m_presentationInit = true;
    }
    if (m_editCameraEnabled) {
        const auto values = m_editCameras.values();
        for (auto camera : values)
            m_editCameraInfo.applyToCamera(*camera, QSizeF(m_size));
    }
    if (rect != m_rect || size != m_size) {
        m_engine->sceneManager()->updateSizes(size, 1.0, rect, true);
        m_rect = rect;
        m_size = size;
    }
}

void Q3DSTranslation::enableEditCamera(const SEditCameraPersistentInformation &info)
{
    m_editCameraInfo = info;
    // loop through layers and create edit camera for each
    Q3DSGraphObject *object = m_scene->firstChild();
    while (object) {
        if (object->type() != Q3DSGraphObject::Layer) {
            object = object->nextSibling();
            continue;
        }
        Q3DSLayerNode *layer = static_cast<Q3DSLayerNode *>(object);
        QByteArray editCameraId = QByteArrayLiteral("StudioEditCamera_");
        editCameraId.append(layer->id());

        Q3DSCameraNode *editCamera = nullptr;
        if (!m_editCameras.contains(editCameraId)) {
            editCamera = m_presentation->newObject<Q3DSCameraNode>(editCameraId);
            layer->appendChildNode(editCamera);
            m_presentation->masterSlide()->addObject(editCamera);
            m_editCameras.insert(editCameraId, editCamera);
        } else {
            editCamera = m_editCameras[editCameraId];
            if (layer != editCamera->parent()) {
                editCamera->parent()->removeChildNode(editCamera);
                layer->appendChildNode(editCamera);
            }
        }

        m_editCameraInfo.applyToCamera(*editCamera, QSizeF(m_size));

        Q3DSPropertyChangeList list;
        list.append(editCamera->setEyeballEnabled(true));
        list.append(editCamera->setName(info.m_name));
        editCamera->notifyPropertyChanges(list);

        object = object->nextSibling();
    }
    enableSceneCameras(false);
    m_editCameraEnabled = true;
}

void Q3DSTranslation::disableEditCamera()
{
    const auto values = m_editCameras.values();
    for (auto camera : values) {
        Q3DSPropertyChangeList list;
        list.append(camera->setEyeballEnabled(false));
        camera->notifyPropertyChanges(list);
    }
    enableSceneCameras(true);
    m_editCameraEnabled = false;
}

SEditCameraPersistentInformation Q3DSTranslation::editCameraInfo() const
{
    return m_editCameraInfo;
}

void Q3DSTranslation::enableSceneCameras(bool enable)
{
    for (auto translator : qAsConst(m_cameraTranslators))
        translator->setEditCameraEnabled(!enable);
}

void Q3DSTranslation::wheelZoom(qreal factor)
{
    m_editCameraInfo.m_viewRadius = qMax(.0001, m_editCameraInfo.m_viewRadius * factor);
}

Q3DSCameraNode *Q3DSTranslation::cameraForNode(Q3DSGraphObject *node)
{
    if (node->type() == Q3DSGraphObject::Camera)
        return static_cast<Q3DSCameraNode *>(node);

    while (node && node->parent() && node->type() != Q3DSGraphObject::Layer)
        node = node->parent();
    Q3DSLayerNode *layer = static_cast<Q3DSLayerNode *>(node);
    Q3DSGraphObject *child = layer->firstChild();
    while (child) {
        if (child->type() == Q3DSGraphObject::Camera
                && static_cast<Q3DSNode *>(child)->eyeballEnabled()) {
            break;
        }
        child = child->nextSibling();
    }
    if (child)
        return static_cast<Q3DSCameraNode *>(child);

    return nullptr;
}

void Q3DSTranslation::prepareDrag(Q3DSGraphObjectTranslator *selected)
{
    m_dragTranslator = selected;
    selected->enableAutoUpdates(false);
    Q3DSNode &node = static_cast<Q3DSNode &>(m_dragTranslator->graphObject());
    m_beginDragState.t = node.position();
    m_beginDragState.s = node.scale();
    m_beginDragState.r = node.rotation();
    m_currentDragState = m_beginDragState;
    m_dragCamera = cameraForNode(&node);
}

void Q3DSTranslation::endDrag(bool dragReset, CUpdateableDocumentEditor &inEditor)
{
    m_dragTranslator->enableAutoUpdates(true);
    if (!dragReset) {
        // send drag state to document
        IDocumentEditor &editor = inEditor.EnsureEditor(QObject::tr("Set Transformation"),
                                                        __FILE__, __LINE__);
        editor.SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                      objectDefinitions().m_Node.m_Position,
                                      qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.t)));
        editor.SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                      objectDefinitions().m_Node.m_Rotation,
                                      qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.r)));
        editor.SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                      objectDefinitions().m_Node.m_Scale,
                                      qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.s)));
        inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
    } else {
        // reset node to beginning
        Q3DSNode &node = static_cast<Q3DSNode &>(m_dragTranslator->graphObject());
        Q3DSPropertyChangeList list;
        list.append(node.setPosition(m_beginDragState.t));
        list.append(node.setScale(m_beginDragState.s));
        list.append(node.setRotation(m_beginDragState.r));
        node.notifyPropertyChanges(list);
    }
    m_dragTranslator = nullptr;
}

void Q3DSTranslation::translateAlongCameraDirection(const QPoint &inOriginalCoords,
                                                    const QPoint &inMouseCoords,
                                                    CUpdateableDocumentEditor &inEditor)
{
    float theYDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    if (qFuzzyIsNull(theYDistance))
        return;

    Q3DSCameraNode *cameraNode = m_dragCamera;
    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSNodeAttached *cameraAttached = cameraNode->attached<Q3DSNodeAttached>();
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;
    QMatrix4x4 parentMatrix = parentAttached->globalTransform;

    float distanceMultiplier = theYDistance * 0.5f + 1.f;
    QVector3D cameraDirection = (cameraMatrix * QVector4D(0.f, 0.f, 1.f, 0.f)).toVector3D();
    QVector3D diff = cameraDirection * distanceMultiplier;
    diff = (parentMatrix.inverted() * QVector4D(diff.x(), diff.y(), -diff.z(), 0.f)).toVector3D();
    m_currentDragState.t = m_beginDragState.t + diff;
    Q3DSPropertyChangeList list;
    list.append(node->setPosition(m_currentDragState.t));
    node->notifyPropertyChanges(list);
    inEditor.EnsureEditor(QObject::tr("Set Position"), __FILE__, __LINE__)
        .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                  objectDefinitions().m_Node.m_Position,
                                  qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.t)));
    inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
}

void Q3DSTranslation::translate(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                                CUpdateableDocumentEditor &inEditor, bool inLockToAxis)
{
    float theXDistance = float(inMouseCoords.x() - inOriginalCoords.x());
    float theYDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    if (qFuzzyIsNull(theXDistance) && qFuzzyIsNull(theYDistance))
        return;

    if (inLockToAxis) {
        if (qAbs(theXDistance) > qAbs(theYDistance))
            theYDistance = 0;
        else
            theXDistance = 0;
    }

    Q3DSCameraNode *cameraNode = m_dragCamera;
    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSNodeAttached *cameraAttached = cameraNode->attached<Q3DSNodeAttached>();
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;
    QMatrix4x4 parentMatrix = parentAttached->globalTransform;

    float xMultiplier = theXDistance * 0.5f + 1.f;
    float yMultiplier = theYDistance * 0.5f + 1.f;

    QVector3D cameraLeft = (cameraMatrix * QVector4D(1.f, 0.f, 0.f, 0.f)).toVector3D();
    QVector3D cameraUp = (cameraMatrix * QVector4D(0.f, -1.f, 0.f, 0.f)).toVector3D();

    QVector3D diff = cameraLeft * xMultiplier + cameraUp * yMultiplier;
    diff = (parentMatrix.inverted() * QVector4D(diff.x(), diff.y(), -diff.z(), 0.f)).toVector3D();
    m_currentDragState.t = m_beginDragState.t + diff;
    Q3DSPropertyChangeList list;
    list.append(node->setPosition(m_currentDragState.t));
    node->notifyPropertyChanges(list);
    inEditor.EnsureEditor(QObject::tr("Set Position"), __FILE__, __LINE__)
        .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                  objectDefinitions().m_Node.m_Position,
                                  qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.t)));
    inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
}

void Q3DSTranslation::scaleZ(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                             CUpdateableDocumentEditor &inEditor)
{
    // Scale scales uniformly and responds to mouse Y only.
    float theYDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    if (qFuzzyIsNull(theYDistance))
        return;

    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    float theScaleMultiplier = 1.0f + theYDistance * (1.0f / 40.0f);
    m_currentDragState.s = QVector3D(m_beginDragState.s.x(), m_beginDragState.s.y(),
                                     m_beginDragState.s.z() * theScaleMultiplier);

    Q3DSPropertyChangeList list;
    list.append(node->setScale(m_currentDragState.s));
    node->notifyPropertyChanges(list);
    inEditor.EnsureEditor(QObject::tr("Set Scale"), __FILE__, __LINE__)
        .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                  objectDefinitions().m_Node.m_Scale,
                                  qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.s)));
    inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
}

void Q3DSTranslation::scale(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                            CUpdateableDocumentEditor &inEditor)
{
    // Scale scales uniformly and responds to mouse Y only.
    float theYDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    if (qFuzzyIsNull(theYDistance))
        return;

    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    float theScaleMultiplier = 1.0f + theYDistance * (1.0f / 40.0f);
    m_currentDragState.s = m_beginDragState.s * theScaleMultiplier;

    Q3DSPropertyChangeList list;
    list.append(node->setScale(m_currentDragState.s));
    node->notifyPropertyChanges(list);
    inEditor.EnsureEditor(QObject::tr("Set Scale"), __FILE__, __LINE__)
        .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                  objectDefinitions().m_Node.m_Scale,
                                  qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.s)));
    inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
}

void Q3DSTranslation::rotateAboutCameraDirectionVector(const QPoint &inOriginalCoords,
                                                       const QPoint &inMouseCoords,
                                                       CUpdateableDocumentEditor &inEditor)
{
    float theYDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    if (qFuzzyIsNull(theYDistance))
        return;

    Q3DSCameraNode *cameraNode = m_dragCamera;
    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    Q3DSNodeAttached *cameraAttached = cameraNode->attached<Q3DSNodeAttached>();
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;

    QVector3D cameraDirection = (cameraMatrix * QVector4D(0.f, 0.f, -1.f, 0.f)).toVector3D();
    QQuaternion yrotation = QQuaternion::fromAxisAndAngle(cameraDirection, .1f * theYDistance
                                                          * float(g_rotationScaleFactor));
    QQuaternion origRotation = QQuaternion::fromEulerAngles(m_beginDragState.r);
    yrotation = origRotation * yrotation;
    m_currentDragState.r = yrotation.toEulerAngles();
    Q3DSPropertyChangeList list;
    list.append(node->setRotation(m_currentDragState.r));
    node->notifyPropertyChanges(list);
    inEditor.EnsureEditor(QObject::tr("Set Rotation"), __FILE__, __LINE__)
        .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                  objectDefinitions().m_Node.m_Rotation,
                                  qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.r)));
    inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
}

void Q3DSTranslation::rotate(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                             CUpdateableDocumentEditor &inEditor, bool inLockToAxis)
{
    float theXDistance = float(inMouseCoords.x() - inOriginalCoords.x());
    float theYDistance = float(inMouseCoords.y() - inOriginalCoords.y());

    if (qFuzzyIsNull(theXDistance) && qFuzzyIsNull(theYDistance))
        return;

    if (inLockToAxis) {
        if (inLockToAxis) {
            if (qAbs(theXDistance) > qAbs(theYDistance))
                theYDistance = 0;
            else
                theXDistance = 0;
        }
    }

    Q3DSCameraNode *cameraNode = m_dragCamera;
    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    Q3DSNodeAttached *cameraAttached = cameraNode->attached<Q3DSNodeAttached>();
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;

    QVector3D cameraLeft = (cameraMatrix * QVector4D(1.f, 0.f, 0.f, 0.f)).toVector3D();
    QVector3D cameraUp = (cameraMatrix * QVector4D(0.f, -1.f, 0.f, 0.f)).toVector3D();

    QVector3D axis = theXDistance * cameraUp - theYDistance * cameraLeft;
    float distance = axis.length();
    axis.normalize();
    QQuaternion yrotation = QQuaternion::fromAxisAndAngle(axis, .1f * distance
                                                          * float(g_rotationScaleFactor));
    QQuaternion origRotation = QQuaternion::fromEulerAngles(m_beginDragState.r);
    yrotation = origRotation * yrotation;
    m_currentDragState.r = yrotation.toEulerAngles();

    Q3DSPropertyChangeList list;
    list.append(node->setRotation(m_currentDragState.r));
    node->notifyPropertyChanges(list);
    inEditor.EnsureEditor(QObject::tr("Set Rotation"), __FILE__, __LINE__)
        .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                  objectDefinitions().m_Node.m_Rotation,
                                  qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.r)));
    inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
}

}
