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
#include "Q3DSWidgetUtils.h"

#include "StudioApp.h"
#include "Core.h"
#include "ClientDataModelBridge.h"
#include "StudioFullSystem.h"
#include "IDocumentReader.h"
#include "StudioProjectSettings.h"
#include "SlideSystem.h"

#include <QtCore/qmath.h>
#include <Qt3DRender/qcamera.h>
#include <Qt3DCore/qtransform.h>

namespace Q3DStudio {

// This function is copied from Qt3D/qmath3d_p.h
inline void decomposeQMatrix3x3(const QMatrix3x3 &m, QMatrix3x3 &Q, QVector3D &D, QVector3D &U)
{
    // Factor M = QR = QDU where Q is orthogonal, D is diagonal,
    // and U is upper triangular with ones on its diagonal.
    // Algorithm uses Gram-Schmidt orthogonalization (the QR algorithm).
    //
    // If M = [ m0 | m1 | m2 ] and Q = [ q0 | q1 | q2 ], then
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //   q2 = (m2-(q0*m2)q0-(q1*m2)q1)/|m2-(q0*m2)q0-(q1*m2)q1|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.  The matrix R has entries
    //
    //   r00 = q0*m0  r01 = q0*m1  r02 = q0*m2
    //   r10 = 0      r11 = q1*m1  r12 = q1*m2
    //   r20 = 0      r21 = 0      r22 = q2*m2
    //
    // so D = diag(r00,r11,r22) and U has entries u01 = r01/r00,
    // u02 = r02/r00, and u12 = r12/r11.

    // Q = rotation
    // D = scaling
    // U = shear

    // D stores the three diagonal entries r00, r11, r22
    // U stores the entries U[0] = u01, U[1] = u02, U[2] = u12

    // build orthogonal matrix Q
    float invLen = 1.0f / std::sqrt(m(0, 0) * m(0, 0) + m(1, 0) * m(1, 0) + m(2, 0) * m(2, 0));
    Q(0, 0) = m(0, 0) * invLen;
    Q(1, 0) = m(1, 0) * invLen;
    Q(2, 0) = m(2, 0) * invLen;

    float dot = Q(0, 0) * m(0, 1) + Q(1, 0) * m(1, 1) + Q(2, 0) * m(2, 1);
    Q(0, 1) = m(0, 1) - dot * Q(0, 0);
    Q(1, 1) = m(1, 1) - dot * Q(1, 0);
    Q(2, 1) = m(2, 1) - dot * Q(2, 0);
    invLen = 1.0f / std::sqrt(Q(0, 1) * Q(0, 1) + Q(1, 1) * Q(1, 1) + Q(2, 1) * Q(2, 1));
    Q(0, 1) *= invLen;
    Q(1, 1) *= invLen;
    Q(2, 1) *= invLen;

    dot = Q(0, 0) * m(0, 2) + Q(1, 0) * m(1, 2) + Q(2, 0) * m(2, 2);
    Q(0, 2) = m(0, 2) - dot * Q(0, 0);
    Q(1, 2) = m(1, 2) - dot * Q(1, 0);
    Q(2, 2) = m(2, 2) - dot * Q(2, 0);
    dot = Q(0, 1) * m(0, 2) + Q(1, 1) * m(1, 2) + Q(2, 1) * m(2, 2);
    Q(0, 2) -= dot * Q(0, 1);
    Q(1, 2) -= dot * Q(1, 1);
    Q(2, 2) -= dot * Q(2, 1);
    invLen = 1.0f / std::sqrt(Q(0, 2) * Q(0, 2) + Q(1, 2) * Q(1, 2) + Q(2, 2) * Q(2, 2));
    Q(0, 2) *= invLen;
    Q(1, 2) *= invLen;
    Q(2, 2) *= invLen;

    // guarantee that orthogonal matrix has determinant 1 (no reflections)
    const float det = Q(0, 0) * Q(1, 1) * Q(2, 2) + Q(0, 1) * Q(1, 2) * Q(2, 0) +
                      Q(0, 2) * Q(1, 0) * Q(2, 1) - Q(0, 2) * Q(1, 1) * Q(2, 0) -
                      Q(0, 1) * Q(1, 0) * Q(2, 2) - Q(0, 0) * Q(1, 2) * Q(2, 1);
    if (det < 0.0f)
        Q *= -1.0f;

    // build "right" matrix R
    QMatrix3x3 R(Qt::Uninitialized);
    R(0, 0) = Q(0, 0) * m(0, 0) + Q(1, 0) * m(1, 0) + Q(2, 0) * m(2, 0);
    R(0, 1) = Q(0, 0) * m(0, 1) + Q(1, 0) * m(1, 1) + Q(2, 0) * m(2, 1);
    R(1, 1) = Q(0, 1) * m(0, 1) + Q(1, 1) * m(1, 1) + Q(2, 1) * m(2, 1);
    R(0, 2) = Q(0, 0) * m(0, 2) + Q(1, 0) * m(1, 2) + Q(2, 0) * m(2, 2);
    R(1, 2) = Q(0, 1) * m(0, 2) + Q(1, 1) * m(1, 2) + Q(2, 1) * m(2, 2);
    R(2, 2) = Q(0, 2) * m(0, 2) + Q(1, 2) * m(1, 2) + Q(2, 2) * m(2, 2);

    // the scaling component
    D[0] = R(0, 0);
    D[1] = R(1, 1);
    D[2] = R(2, 2);

    // the shear component
    U[0] = R(0, 1) / D[0];
    U[1] = R(0, 2) / D[0];
    U[2] = R(1, 2) / D[1];
}

// This function is copied from Qt3D/qmath3d_p.h
inline bool hasScale(const QMatrix4x4 &m)
{
    // If the columns are orthonormal and form a right-handed system, then there is no scale
    float t(float(m.determinant()));
    if (!qFuzzyIsNull(t - 1.0f))
        return true;
    t = m(0, 0) * m(0, 0) + m(1, 0) * m(1, 0) + m(2, 0) * m(2, 0);
    if (!qFuzzyIsNull(t - 1.0f))
        return true;
    t = m(0, 1) * m(0, 1) + m(1, 1) * m(1, 1) + m(2, 1) * m(2, 1);
    if (!qFuzzyIsNull(t - 1.0f))
        return true;
    t = m(0, 2) * m(0, 2) + m(1, 2) * m(1, 2) + m(2, 2) * m(2, 2);
    if (!qFuzzyIsNull(t - 1.0f))
        return true;
    return false;
}

// This function is copied from Qt3D/qmath3d_p.h
inline void decomposeQMatrix4x4(const QMatrix4x4 &m, QVector3D &position, QQuaternion &orientation,
                                QVector3D &scale)
{
    Q_ASSERT(m.isAffine());

    const QMatrix3x3 m3x3(m.toGenericMatrix<3, 3>());

    QMatrix3x3 rot3x3(Qt::Uninitialized);
    if (hasScale(m)) {
        decomposeQMatrix3x3(m3x3, rot3x3, scale, position);
    } else {
        // we know there is no scaling part; no need for QDU decomposition
        scale = QVector3D(1.0f, 1.0f, 1.0f);
        rot3x3 = m3x3;
    }
    orientation = QQuaternion::fromRotationMatrix(rot3x3);
    position = QVector3D(m(0, 3), m(1, 3), m(2, 3));
}

// Need custom calculation for camera view matrix to properly handle top/bottom edit cameras,
// where default upvector doesn't work.
static QMatrix4x4 calculateCameraViewMatrix(const QMatrix4x4 &cameraWorldTransform)
{
    const QVector4D position = cameraWorldTransform * QVector4D(0.0f, 0.0f, 0.0f, 1.0f);
    const QVector4D viewDirection = cameraWorldTransform * QVector4D(0.0f, 0.0f, -1.0f, 0.0f);
    const QVector4D upVector = cameraWorldTransform * QVector4D(0.0f, 1.0f, 0.0f, 0.0f);

    QMatrix4x4 m;
    m.lookAt(QVector3D(position),
             QVector3D(position + viewDirection),
             QVector3D(upVector));
    return QMatrix4x4(m);
}

static QPointF normalizePointToRect(const QPoint &inPoint, const QRectF &rect)
{
    qreal x = qreal(inPoint.x() - rect.x());
    qreal y = qreal(inPoint.y() - rect.y());
    x = x / rect.width() * 2 - 1;
    y = y / rect.height() * 2 - 1;
    // OpenGL has inverted Y
    y = -y;

    return QPointF(x, y);
}

static QVector3D calcRay(const QPointF &point, const QMatrix4x4 &viewMatrix,
                         const QMatrix4x4 &projectionMatrix, QVector3D &outNearPos)
{
    QRect viewPort(-1, -1, 2, 2);
    outNearPos = QVector3D(float(point.x()), float(point.y()), 0.0f);
    outNearPos = outNearPos.unproject(viewMatrix, projectionMatrix, viewPort);
    QVector3D farPos(float(point.x()), float(point.y()), 1.0f);
    farPos = farPos.unproject(viewMatrix, projectionMatrix, viewPort);

    QVector3D ray = (farPos - outNearPos).normalized();

    return ray;
}

// Returns the intersection point of a plane and a ray.
// Parameter t returns the distance in ray lengths. If t is negative, intersection
// is behind rayOrigin.
// If there is no intersection, i.e. plane and the ray are paraller, t is set to -1 and
// rayOrigin is returned.
static QVector3D findIntersection(const QVector3D &rayOrigin, const QVector3D &ray,
                                  float planeOffset, const QVector3D &planeNormal, float &t)
{
    float divisor = QVector3D::dotProduct(ray, planeNormal);
    if (qFuzzyCompare(1.0f, 1.0f + divisor)) {
        t = -1.0f;
        return rayOrigin;
    }

    t = -(QVector3D::dotProduct(rayOrigin, planeNormal) - planeOffset) / divisor;

    return rayOrigin + ray * t;

}

// Qt3D and editor have mirrored Z-axes. This function can be used to convert coordinates
// between the two systems.
static void flipZTranslation(QVector3D &vec)
{
    vec.setZ(-vec.z());
}

// Calculates the intersection of a ray through camera position and mouse point with a plane
// that is paraller to the camera plane and goes through the given node if it has
// the given local position (which can be different than node's actual position).
static QVector3D mousePointToCameraPlaneIntersection(const QPoint &mousePos,
                                                     Q3DSCameraNode *cameraNode,
                                                     Q3DSNode *node,
                                                     const QVector3D &nodePosition)
{
    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSCameraAttached *cameraAttached = cameraNode->attached<Q3DSCameraAttached>();
    Q3DSNodeAttached *nodeAttached = node->attached<Q3DSNodeAttached>();
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    Q3DSLayerAttached *layerAttached = nodeAttached->layer3DS->attached<Q3DSLayerAttached>();

    QMatrix4x4 parentMatrix = parentAttached->globalTransform;
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;

    QRectF layerRect = QRectF(layerAttached->layerPos, layerAttached->layerSize);
    QPointF newPoint = normalizePointToRect(mousePos, layerRect);

    auto viewMatrix = calculateCameraViewMatrix(cameraMatrix);
    auto projectionMatrix = cameraAttached->camera->projectionMatrix();

    QVector3D cameraPos = cameraNode->position();
    flipZTranslation(cameraPos);

    // zeroRay is effectively the camera plane normal.
    QVector3D nearPos;
    QVector3D zeroRay = calcRay(QPointF(0., 0.), viewMatrix, projectionMatrix, nearPos);
    QVector3D newRay = calcRay(newPoint, viewMatrix, projectionMatrix, nearPos);

    // Find intersections of newRay and oldRay on camera plane that goes through oldPos
    // Operations are done in Qt3D space, i.e. Z-axis flipped
    QVector3D beginPos = nodePosition;
    flipZTranslation(beginPos);
    QVector3D nodeWorldPos = parentMatrix * beginPos;
    float distance = -1.f;
    float cosAngle = QVector3D::dotProduct(nodeWorldPos.normalized(), zeroRay);
    float planeOffset = nodeWorldPos.length() * cosAngle;

    QVector3D newIntersect = findIntersection(nearPos, newRay, planeOffset, zeroRay, distance);
    QVector3D endPos = parentMatrix.inverted() * newIntersect;
    flipZTranslation(endPos); // Flip back to editor coords

    return endPos;
}

// Projects a local node position to the scene view coordinate
static QPoint localPositionToMousePoint(Q3DSCameraNode *cameraNode,
                                        Q3DSNode *node,
                                        const QVector3D &position,
                                        const QSize &presSize)
{
    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    Q3DSCameraAttached *cameraAttached = cameraNode->attached<Q3DSCameraAttached>();
    Q3DSNodeAttached *nodeAttached = node->attached<Q3DSNodeAttached>();
    Q3DSLayerAttached *layerAttached = nodeAttached->layer3DS->attached<Q3DSLayerAttached>();
    QMatrix4x4 parentMatrix = parentAttached->globalTransform;
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;

    QVector3D pos = position;
    flipZTranslation(pos);
    QVector3D worldPos = parentMatrix * pos;

    auto viewMatrix = calculateCameraViewMatrix(cameraMatrix);
    auto projectionMatrix = cameraAttached->camera->projectionMatrix();

    QRect layerRect = QRect(layerAttached->layerPos.toPoint(), layerAttached->layerSize);
    QVector3D layerPoint = worldPos.project(viewMatrix, projectionMatrix, layerRect);

    // Convert layer coordinates to screen coordinates. X is already good, but Y needs bit of
    // mangling to get correct as OpenGL and mouse Y are flipped
    QPoint screenPoint(int(layerPoint.x()),
                       2 * layerRect.y() + layerRect.height() - int(layerPoint.y()));

    return screenPoint;
}

static QPoint getAxisLockedMousePos(const QPoint &currentMousePos, const QPoint &dragStartMousePos)
{
    // Lock to axis that is closest
    QPoint mousePos = currentMousePos;
    int xDiff = qAbs(mousePos.x() - dragStartMousePos.x());
    int yDiff = qAbs(mousePos.y() - dragStartMousePos.y());
    if (xDiff > yDiff)
        mousePos.setY(dragStartMousePos.y());
    else
        mousePos.setX(dragStartMousePos.x());
    return mousePos;
}

// Pulls the 1st column out of the global transform.
static QVector3D getXAxis(const QMatrix4x4 &matrix)
{
    const float *data = matrix.data();
    QVector3D retval(data[0], data[1], data[2]);
    retval.normalize();
    return retval;
}

// Pulls the 2nd column out of the global transform.
static QVector3D getYAxis(const QMatrix4x4 &matrix)
{
    const float *data = matrix.data();
    QVector3D retval(data[4], data[5], data[6]);
    retval.normalize();
    return retval;
}

// Pulls the 3rd column out of the global transform.
static QVector3D getDirection(const QMatrix4x4 &matrix)
{
    const float *data = matrix.data();
    QVector3D retval(data[8], data[9], data[10]);
    retval.normalize();
    return retval;
}
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

    enableBackgroundLayer();
    enableForegroundLayer();
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
        Q3DSLightTranslator *t = new Q3DSLightTranslator(
                    instance, *m_presentation->newObject<Q3DSLightNode>(id));
        m_lightTranslators.push_back(t);
        translator = t;
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

    updateForegroundLayerProperties();
    updateSelectionWidgetProperties();
    updateVisualAids();
}

void Q3DSTranslation::prepareRender(const QRect &rect, const QSize &size, qreal pixelRatio)
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
    if (rect != m_rect || size != m_size || pixelRatio != m_pixelRatio) {
        m_engine->sceneManager()->updateSizes(size, pixelRatio, rect, true);
        m_rect = rect;
        m_size = size;
        m_pixelRatio = pixelRatio;
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
    updateForegroundLayerProperties();
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
    m_oldCameraType = EditCameraTypes::SceneCamera;
    updateForegroundLayerProperties();
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

void Q3DSTranslation::enableBackgroundLayer()
{
    if (!m_backgroundLayer) {
        m_backgroundLayer = m_presentation->newObject<Q3DSLayerNode>("StudioBackgroundLayer_");
        m_scene->appendChildNode(m_backgroundLayer);
        m_presentation->masterSlide()->addObject(m_backgroundLayer);
        m_backgroundLayer->notifyPropertyChanges({
            m_backgroundLayer->setDepthPrePassDisabled(true)
        });
    }
}

void Q3DSTranslation::enableForegroundLayer()
{
    if (!m_foregroundLayer && !m_foregroundPickingLayer) {
        m_foregroundLayer = m_presentation->newObject<Q3DSLayerNode>("StudioForegroundLayer_");
        m_scene->prependChildNode(m_foregroundLayer);
        m_presentation->masterSlide()->addObject(m_foregroundLayer);

        m_foregroundLayer->notifyPropertyChanges({
            m_foregroundLayer->setDepthPrePassDisabled(true),
            m_foregroundLayer->setDepthTestDisabled(true)
        });

        m_foregroundPickingLayer = m_presentation->newObject<Q3DSLayerNode>(
                    "StudioForegroundPickingLayer_");
        m_scene->prependChildNode(m_foregroundPickingLayer);
        m_presentation->masterSlide()->addObject(m_foregroundPickingLayer);

        m_foregroundPickingLayer->notifyPropertyChanges({
            m_foregroundPickingLayer->setDepthPrePassDisabled(true),
        });
    }
}

void Q3DSTranslation::disableGradient()
{
    if (m_gradient) {
        m_presentation->unlinkObject(m_gradientMaterial);
        m_presentation->masterSlide()->removeObject(m_gradient);
        m_presentation->unlinkObject(m_gradient);
        delete m_gradient;
        m_gradient = nullptr;
    }
}

void Q3DSTranslation::enableGradient()
{
    if (m_oldCameraType != m_editCameraInfo.m_cameraType) {
        disableGradient();
        if (m_backgroundLayer) {
            m_gradient = m_presentation->newObject<Q3DSModelNode>("StudioGradient_");
            m_backgroundLayer->appendChildNode(m_gradient);
            m_presentation->masterSlide()->addObject(m_gradient);

            Q3DSPropertyChangeList list;
            list.append(m_gradient->setMesh(QStringLiteral(":/res/InvertedCube.mesh")));
            m_gradient->notifyPropertyChanges(list);

            QString matData;
            if (m_editCameraInfo.m_cameraType == EditCameraTypes::Perspective) {
                matData =
                        "<Material name=\"StudioGradientMaterial_\" version=\"1.0\">\n"
                        "<MetaData></MetaData>\n"
                        "<Shaders type=\"GLSL\" version=\"330\">\n"
                        "<Shader>\n"
                        "<VertexShader>\n"
                        "attribute vec3 attr_pos;\n"
                        "varying vec3 pos;\n"
                        "uniform mat4 projectionMatrix;\n"
                        "uniform mat3 modelViewNormal;\n"
                        "void main() {\n"
                        "pos = attr_pos * 1000.0;\n"
                        "gl_Position = projectionMatrix * vec4(modelViewNormal * pos, 1.0);\n"
                        "</VertexShader>\n"
                        "<FragmentShader>\n"
                        "varying vec3 pos;\n"
                        "void main() {\n"
                        "vec3 npos = normalize(pos);\n"
                        "vec3 color = vec3(0.0);\n"
                        "if (npos.y > 0.0)\n"
                        "color = mix(vec3(0.4, 0.4, 0.4), vec3(0.6, 0.6, 0.6),"
                        " pow(npos.y, 0.25));\n"
                        "else\n"
                        "color = mix(vec3(0.35, 0.35, 0.35), vec3(0.1, 0.1, 0.1),"
                        " pow(-npos.y, 0.5));\n"
                        "fragOutput = vec4(color, 1.0);\n"
                        "</FragmentShader>\n"
                        "</Shader>\n"
                        "</Shaders>\n"
                        "<Passes><Pass></Pass></Passes>\n"
                        "</Material>\n";
            } else {
                matData =
                        "<Material name=\"StudioGradientMaterial_\" version=\"1.0\">\n"
                        "<MetaData></MetaData>\n"
                        "<Shaders type=\"GLSL\" version=\"330\">\n"
                        "<Shader>\n"
                        "<VertexShader>\n"
                        "attribute vec3 attr_pos;\n"
                        "varying vec3 pos;\n"
                        "void main() {\n"
                        "pos = attr_pos * 1000.0;\n"
                        "gl_Position = vec4(pos.x > 0 ? 1.0 : -1.0, pos.y > 0"
                        " ? 1.0 : -1.0, 0.0, 1.0);\n"
                        "</VertexShader>\n"
                        "<FragmentShader>\n"
                        "varying vec3 pos;\n"
                        "void main() {\n"
                        "vec3 npos = normalize(pos);\n"
                        "vec3 color = vec3(0.0);\n"
                        "color = mix(vec3(0.6, 0.6, 0.6), vec3(0.35, 0.35, 0.35),"
                        " 0.5 * npos.y + 0.5);\n"
                        "fragOutput = vec4(color, 1.0);\n"
                        "</FragmentShader>\n"
                        "</Shader>\n"
                        "</Shaders>\n"
                        "<Passes><Pass></Pass></Passes>\n"
                        "</Material>\n";
            }

            const QByteArray matId = QByteArrayLiteral("#StudioGradientMaterial_");
            Q3DSCustomMaterial material = m_presentation->customMaterial(matId, matData.toUtf8());
            if (!material.isNull()) {
                m_gradientMaterial = m_presentation->newObject<Q3DSCustomMaterialInstance>(matId);
                m_gradientMaterial->setSourcePath(matId);
                m_gradientMaterial->resolveReferences(*m_presentation.data());
                m_gradient->appendChildNode(m_gradientMaterial);
            }
        }
        m_oldCameraType = m_editCameraInfo.m_cameraType;
    }
}

void Q3DSTranslation::disableSelectionWidget()
{
    m_selectionWidget.destroy(m_presentation.data());
    m_selectedObject = nullptr;
}

void Q3DSTranslation::enableSelectionWidget(Qt3DSDMInstanceHandle instance)
{
    Q3DSGraphObjectTranslator *translator = getOrCreateTranslator(instance);
    if (!translator)
        return;

    m_selectedObject = &translator->graphObject();

    if (!m_selectedObject || (m_selectedObject->type() != Q3DSGraphObject::Model
                              && m_selectedObject->type() != Q3DSGraphObject::Alias
                              && m_selectedObject->type() != Q3DSGraphObject::Group
                              && m_selectedObject->type() != Q3DSGraphObject::Light
                              && m_selectedObject->type() != Q3DSGraphObject::Camera
                              && m_selectedObject->type() != Q3DSGraphObject::Text
                              && m_selectedObject->type() != Q3DSGraphObject::Component)) {
        m_selectionWidget.setEyeballEnabled(false);
    }

    updateForegroundLayerProperties();
    updateSelectionWidgetProperties();
}

void Q3DSTranslation::disableVisualAids()
{
    if (!m_visualAids.empty()) {
        for (int i = 0; i < m_visualAids.size(); ++i)
            m_visualAids[i].destroy();
        m_visualAids.clear();
    }
}

void Q3DSTranslation::enableVisualAids()
{
    m_visualAids.reserve(m_cameraTranslators.size() + m_lightTranslators.size());
    for (auto &camera : qAsConst(m_cameraTranslators)) {
        if (m_selectedLayer != nullptr
                && layerForNode(&camera->graphObject()) == m_selectedLayer) {
            bool alreadyCreated = false;
            for (auto &visualAid : qAsConst(m_visualAids)) {
                if (visualAid.hasGraphObject(&camera->graphObject())) {
                    alreadyCreated = true;
                    break;
                }
            }
            if (alreadyCreated)
                continue;

            m_visualAids.append(Q3DSVisualAidWidget(m_presentation.data(), m_foregroundLayer,
                                                    m_foregroundPickingLayer,
                                                    VisualAidType::Camera, &camera->graphObject(),
                                                    m_visualAidIndex++));
        } else {
            for (int i = m_visualAids.size() - 1; i >= 0; --i) {
                if (m_visualAids[i].hasGraphObject(&camera->graphObject())) {
                    m_visualAids[i].destroy();
                    m_visualAids.remove(i);
                }
            }
        }
    }

    for (auto &light : qAsConst(m_lightTranslators)) {
        if (m_selectedLayer != nullptr
                && layerForNode(&light->graphObject()) == m_selectedLayer) {
            VisualAidType newVisualAidType = VisualAidType::DirectionalLight;

            Q3DSLightNode::LightType lightType
                    = static_cast<Q3DSLightNode *>(&light->graphObject())->lightType();
            if (lightType == Q3DSLightNode::LightType::Point)
                newVisualAidType = VisualAidType::PointLight;
            else if (lightType == Q3DSLightNode::LightType::Area)
                newVisualAidType = VisualAidType::AreaLight;

            bool alreadyCreated = false;
            for (int i = m_visualAids.size() - 1; i >= 0; --i) {
                if (m_visualAids[i].hasGraphObject(&light->graphObject())) {
                    if (m_visualAids[i].type() == newVisualAidType) {
                        alreadyCreated = true;
                    } else {
                        m_visualAids[i].destroy();
                        m_visualAids.remove(i);
                    }
                    break;
                }
            }

            if (alreadyCreated)
                continue;

            m_visualAids.append(Q3DSVisualAidWidget(m_presentation.data(), m_foregroundLayer,
                                                    m_foregroundPickingLayer, newVisualAidType,
                                                    &light->graphObject(), m_visualAidIndex++));
        } else {
            for (int i = m_visualAids.size() - 1; i >= 0; --i) {
                if (m_visualAids[i].hasGraphObject(&light->graphObject())) {
                    m_visualAids[i].destroy();
                    m_visualAids.remove(i);
                }
            }
        }
    }
}

Q3DSLayerNode *Q3DSTranslation::layerForNode(Q3DSGraphObject *node)
{
    while (node && node->parent() && node->type() != Q3DSGraphObject::Layer)
        node = node->parent();
    if (node && node->type() != Q3DSGraphObject::Layer)
        return nullptr;
    return static_cast<Q3DSLayerNode *>(node);
}

Q3DSCameraNode *Q3DSTranslation::cameraForNode(Q3DSGraphObject *node, bool ignoreSelfCamera)
{
    if (!ignoreSelfCamera && node->type() == Q3DSGraphObject::Camera)
        return static_cast<Q3DSCameraNode *>(node);

    Q3DSLayerNode *layer = layerForNode(node);
    if (layer) {
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
    }

    return nullptr;
}

void Q3DSTranslation::updateVisualAids()
{
    if (!m_gradient) {
        disableVisualAids();
        return;
    }

    enableVisualAids();

    if (m_foregroundCamera) {
        for (auto &visualAid : qAsConst(m_visualAids))
            visualAid.update(m_foregroundCamera);
    }
}

void Q3DSTranslation::updateForegroundLayerProperties()
{
    if (m_foregroundLayer && m_foregroundPickingLayer && !m_foregroundCamera
            && !m_foregroundPickingCamera) {
        m_foregroundCamera = m_presentation->newObject<Q3DSCameraNode>("StudioForegroundCamera_");
        m_foregroundLayer->appendChildNode(m_foregroundCamera);
        m_presentation->masterSlide()->addObject(m_foregroundCamera);

        m_foregroundCamera->notifyPropertyChanges({
            m_foregroundCamera->setClipNear(10),
            m_foregroundCamera->setClipFar(50000)
        });

        m_foregroundPickingCamera = m_presentation->newObject<Q3DSCameraNode>(
                    "StudioForegroundPickingCamera_");
        m_foregroundPickingLayer->appendChildNode(m_foregroundPickingCamera);
        m_presentation->masterSlide()->addObject(m_foregroundPickingCamera);

        m_foregroundPickingCamera->notifyPropertyChanges({
            m_foregroundPickingCamera->setClipNear(10),
            m_foregroundPickingCamera->setClipFar(50000)
        });

        Q3DSLayerAttached *pickingAttached = m_foregroundPickingLayer
                ->attached<Q3DSLayerAttached>();
        if (pickingAttached && !pickingAttached->layerRayCaster)
            pickingAttached->createRayCaster();
    }

    auto layer = layerForNode(m_selectedObject);
    if (m_selectedObject && layer) {
        m_selectedCamera = cameraForNode(m_selectedObject, true);
        m_selectedLayer = layerForNode(m_selectedObject);
    } else {
        Q3DSGraphObject *object = m_scene->firstChild();
        while (object) {
            if (object->type() == Q3DSGraphObject::Type::Layer && object != m_foregroundLayer
                    && object != m_foregroundPickingLayer && object != m_backgroundLayer) {
                break;
            }
            object = object->nextSibling();
        }
        if (object) {
            m_selectedCamera = cameraForNode(object, true);
            m_selectedLayer = layerForNode(object);
        }
    }

    if (m_selectedCamera && m_foregroundCamera && m_foregroundPickingCamera) {
        Q3DSPropertyChangeList list;
        list.append(m_foregroundCamera->setFov(m_selectedCamera->fov()));
        list.append(m_foregroundCamera->setZoom(m_selectedCamera->zoom()));
        list.append(m_foregroundCamera->setScaleMode(m_selectedCamera->scaleMode()));
        list.append(m_foregroundCamera->setScaleAnchor(m_selectedCamera->scaleAnchor()));
        list.append(m_foregroundCamera->setOrthographic(m_selectedCamera->orthographic()));
        list.append(m_foregroundCamera->setFovHorizontal(m_selectedCamera->fovHorizontal()));
        list.append(m_foregroundCamera->setPivot(m_selectedCamera->pivot()));
        list.append(m_foregroundCamera->setScale(m_selectedCamera->scale()));
        list.append(m_foregroundCamera->setPosition(m_selectedCamera->position()));
        list.append(m_foregroundCamera->setRotation(m_selectedCamera->rotation()));
        m_foregroundCamera->notifyPropertyChanges(list);

        list.clear();
        list.append(m_foregroundPickingCamera->setFov(m_selectedCamera->fov()));
        list.append(m_foregroundPickingCamera->setZoom(m_selectedCamera->zoom()));
        list.append(m_foregroundPickingCamera->setScaleMode(m_selectedCamera->scaleMode()));
        list.append(m_foregroundPickingCamera->setScaleAnchor(m_selectedCamera->scaleAnchor()));
        list.append(m_foregroundPickingCamera->setOrthographic(m_selectedCamera->orthographic()));
        list.append(m_foregroundPickingCamera->setFovHorizontal(m_selectedCamera->fovHorizontal()));
        list.append(m_foregroundPickingCamera->setPivot(m_selectedCamera->pivot()));
        list.append(m_foregroundPickingCamera->setScale(m_selectedCamera->scale()));
        list.append(m_foregroundPickingCamera->setPosition(m_selectedCamera->position()));
        list.append(m_foregroundPickingCamera->setRotation(m_selectedCamera->rotation()));
        m_foregroundPickingCamera->notifyPropertyChanges(list);
    }

    if (m_selectedLayer && m_foregroundLayer && m_foregroundPickingLayer) {
        Q3DSPropertyChangeList list;
        list.append(m_foregroundLayer->setHorizontalFields(m_selectedLayer->horizontalFields()));
        list.append(m_foregroundLayer->setVerticalFields(m_selectedLayer->verticalFields()));
        list.append(m_foregroundLayer->setTopUnits(m_selectedLayer->topUnits()));
        list.append(m_foregroundLayer->setLeftUnits(m_selectedLayer->leftUnits()));
        list.append(m_foregroundLayer->setRightUnits(m_selectedLayer->rightUnits()));
        list.append(m_foregroundLayer->setBottomUnits(m_selectedLayer->bottomUnits()));
        list.append(m_foregroundLayer->setWidthUnits(m_selectedLayer->widthUnits()));
        list.append(m_foregroundLayer->setHeightUnits(m_selectedLayer->heightUnits()));
        list.append(m_foregroundLayer->setTop(m_selectedLayer->top()));
        list.append(m_foregroundLayer->setLeft(m_selectedLayer->left()));
        list.append(m_foregroundLayer->setRight(m_selectedLayer->right()));
        list.append(m_foregroundLayer->setBottom(m_selectedLayer->bottom()));
        list.append(m_foregroundLayer->setWidth(m_selectedLayer->width()));
        list.append(m_foregroundLayer->setHeight(m_selectedLayer->height()));
        m_foregroundLayer->notifyPropertyChanges(list);

        list.clear();
        list.append(m_foregroundPickingLayer->setHorizontalFields(
                        m_selectedLayer->horizontalFields()));
        list.append(m_foregroundPickingLayer->setVerticalFields(
                        m_selectedLayer->verticalFields()));
        list.append(m_foregroundPickingLayer->setTopUnits(m_selectedLayer->topUnits()));
        list.append(m_foregroundPickingLayer->setLeftUnits(m_selectedLayer->leftUnits()));
        list.append(m_foregroundPickingLayer->setRightUnits(m_selectedLayer->rightUnits()));
        list.append(m_foregroundPickingLayer->setBottomUnits(m_selectedLayer->bottomUnits()));
        list.append(m_foregroundPickingLayer->setWidthUnits(m_selectedLayer->widthUnits()));
        list.append(m_foregroundPickingLayer->setHeightUnits(m_selectedLayer->heightUnits()));
        list.append(m_foregroundPickingLayer->setTop(m_selectedLayer->top()));
        list.append(m_foregroundPickingLayer->setLeft(m_selectedLayer->left()));
        list.append(m_foregroundPickingLayer->setRight(m_selectedLayer->right()));
        list.append(m_foregroundPickingLayer->setBottom(m_selectedLayer->bottom()));
        list.append(m_foregroundPickingLayer->setWidth(m_selectedLayer->width()));
        list.append(m_foregroundPickingLayer->setHeight(m_selectedLayer->height()));
        m_foregroundPickingLayer->notifyPropertyChanges(list);
    }
}

void Q3DSTranslation::updateSelectionWidgetProperties()
{
    if (m_selectedObject) {
        if (!m_selectionWidget.isCreated()) {
            createSelectionWidget();
        } else if (g_StudioApp.GetToolMode() != m_toolMode) {
            m_selectionWidget.destroy(m_presentation.data());
            createSelectionWidget();
        }
        m_selectionWidget.setEyeballEnabled(false);

        if (m_foregroundPickingCamera) {
            m_selectionWidget.applyProperties(m_selectedObject, m_foregroundPickingCamera,
                                              m_foregroundLayer, m_size);
        }
    }
}

void Q3DSTranslation::createSelectionWidget()
{
    m_toolMode = g_StudioApp.GetToolMode();
    if (m_toolMode == STUDIO_TOOLMODE_MOVE) {
        m_selectionWidget.create(m_presentation.data(), m_foregroundPickingLayer,
                                 SelectionWidgetType::Translation);
    } else if (m_toolMode == STUDIO_TOOLMODE_ROTATE) {
        m_selectionWidget.create(m_presentation.data(), m_foregroundPickingLayer,
                                 SelectionWidgetType::Rotation);
    } else if (m_toolMode == STUDIO_TOOLMODE_SCALE) {
        m_selectionWidget.create(m_presentation.data(), m_foregroundPickingLayer,
                                 SelectionWidgetType::Scale);
    }
}

void Q3DSTranslation::prepareDrag(const QPoint &mousePos, Q3DSGraphObjectTranslator *selected)
{
    if (!selected) {
        if (m_selectedObject)
            selected = Q3DSGraphObjectTranslator::translatorForObject(m_selectedObject);
        else
            return;
    }
    m_dragTranslator = selected;
    selected->enableAutoUpdates(false);
    Q3DSNode &node = static_cast<Q3DSNode &>(m_dragTranslator->graphObject());
    m_beginDragState.t = node.position();
    m_beginDragState.s = node.scale();
    m_beginDragState.r = node.rotation();
    m_currentDragState = m_beginDragState;
    m_dragCamera = cameraForNode(&node, true);
    m_dragStartMousePos = mousePos;

    // Find out the diff between node position and initial mouse click to avoid having the dragged
    // object initially jump a bit
    QPoint nodeScreenPos = localPositionToMousePoint(m_dragCamera, &node, m_beginDragState.t,
                                                     m_size);
    m_dragPosDiff = m_dragStartMousePos - nodeScreenPos;
}

void Q3DSTranslation::prepareWidgetDrag(const QPoint &mousePos, Q3DSGraphObject *obj)
{
    for (auto &visualAid : qAsConst(m_visualAids)) {
        if (visualAid.hasCollisionBox(obj)) {
            auto visualAidTranslator = Q3DSGraphObjectTranslator::translatorForObject(
                        visualAid.graphObject());
            m_doc.SelectDataModelObject(visualAidTranslator->instanceHandle());
            prepareDrag(mousePos, visualAidTranslator);
            return;
        }
    }
    prepareDrag(mousePos);

    m_pickedWidget = obj;
    m_selectionWidget.setColor(m_pickedWidget, Qt::yellow);
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
    endPickWidget();
}

void Q3DSTranslation::endPickWidget()
{
    if (m_pickedWidget) {
        m_selectionWidget.resetColor(m_pickedWidget);
        m_selectionWidget.resetScale(m_pickedWidget);
    }
    m_pickedWidget = nullptr;
}

void Q3DSTranslation::translateAlongCameraDirection(const QPoint &inOriginalCoords,
                                                    const QPoint &inMouseCoords,
                                                    CUpdateableDocumentEditor &inEditor)
{
    float theYDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    if (qFuzzyIsNull(theYDistance))
        return;
    float distanceMultiplier = theYDistance * 0.5f + 1.f;

    Q3DSCameraNode *cameraNode = m_dragCamera;
    Q3DSCameraAttached *cameraAttached = cameraNode->attached<Q3DSCameraAttached>();
    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;
    QMatrix4x4 parentMatrix = parentAttached->globalTransform;

    QVector3D cameraDirection = getDirection(cameraMatrix);
    QVector3D diff = cameraDirection * distanceMultiplier;
    QVector3D nodePos = m_beginDragState.t;
    flipZTranslation(nodePos);
    QVector3D globalNodePos = parentMatrix * nodePos;
    globalNodePos += diff;
    m_currentDragState.t = parentMatrix.inverted() * globalNodePos;
    flipZTranslation(m_currentDragState.t);

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
    if (m_pickedWidget) {
        translateAlongWidget(inOriginalCoords, inMouseCoords, inEditor);
        return;
    }

    QPoint mousePos = inLockToAxis ? getAxisLockedMousePos(inMouseCoords, m_dragStartMousePos)
                                   : inMouseCoords;
    mousePos -= m_dragPosDiff;

    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    m_currentDragState.t = mousePointToCameraPlaneIntersection(
                mousePos, m_dragCamera, node, m_beginDragState.t);

    Q3DSPropertyChangeList list;
    list.append(node->setPosition(m_currentDragState.t));
    node->notifyPropertyChanges(list);
    inEditor.EnsureEditor(QObject::tr("Set Position"), __FILE__, __LINE__)
        .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                  objectDefinitions().m_Node.m_Position,
                                  qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.t)));
    inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
}

void Q3DSTranslation::translateAlongWidget(const QPoint &inOriginalCoords,
                                           const QPoint &inMouseCoords,
                                           CUpdateableDocumentEditor &inEditor)
{
    // TODO: implement proper dragging that follows the mouse
    float xDistance = float(inMouseCoords.x() - inOriginalCoords.x());
    float yDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    float xMultiplier = xDistance * 0.011f;
    float yMultiplier = yDistance * 0.011f;

    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    Q3DSNodeAttached *widgetAttached = m_pickedWidget->attached<Q3DSNodeAttached>();
    QMatrix4x4 widgetMatrix = widgetAttached->globalTransform;
    adjustRotationLeftToRight(&widgetMatrix);

    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    QMatrix4x4 parentMatrix = parentAttached->globalTransform;
    adjustRotationLeftToRight(&parentMatrix);

    QVector4D direction1;
    QVector4D direction2;
    if (m_selectionWidget.isXAxis(m_pickedWidget)) {
        direction1 = QVector4D(1.f, 0.f, 0.f, 0.f);
        direction2 = QVector4D(0.f, 0.f, 0.f, 0.f);
    } else if (m_selectionWidget.isYAxis(m_pickedWidget)) {
        direction1 = QVector4D(0.f, 1.f, 0.f, 0.f);
        direction2 = QVector4D(0.f, 0.f, 0.f, 0.f);
    } else if (m_selectionWidget.isZAxis(m_pickedWidget)) {
        direction1 = QVector4D(0.f, 0.f, 1.f, 0.f);
        direction2 = QVector4D(0.f, 0.f, 0.f, 0.f);
    } else if (m_selectionWidget.isXYPlane(m_pickedWidget)) {
        direction1 = QVector4D(1.f, 0.f, 0.f, 0.f);
        direction2 = QVector4D(0.f, 1.f, 0.f, 0.f);
    } else if (m_selectionWidget.isYZPlane(m_pickedWidget)) {
        direction1 = QVector4D(0.f, 1.f, 0.f, 0.f);
        direction2 = QVector4D(0.f, 0.f, 1.f, 0.f);
    } else if (m_selectionWidget.isZXPlane(m_pickedWidget)) {
        direction1 = QVector4D(0.f, 0.f, 1.f, 0.f);
        direction2 = QVector4D(1.f, 0.f, 0.f, 0.f);
    }
    QVector3D diff = (widgetMatrix * direction1).toVector3D() * xMultiplier
            + (widgetMatrix * direction2).toVector3D() * yMultiplier;
    diff = (parentMatrix.inverted() * QVector4D(diff, 0.f)).toVector3D();
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

    if (qFuzzyIsNull(theScaleMultiplier))
        return;

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
    if (m_pickedWidget) {
        scaleAlongWidget(inOriginalCoords, inMouseCoords, inEditor);
        return;
    }

    // Scale scales uniformly and responds to mouse Y only.
    float theYDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    if (qFuzzyIsNull(theYDistance))
        return;

    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    float theScaleMultiplier = 1.0f + theYDistance * (1.0f / 40.0f);
    if (qFuzzyIsNull(theScaleMultiplier))
        return;
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

void Q3DSTranslation::scaleAlongWidget(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                                       CUpdateableDocumentEditor &inEditor)
{
    // TODO: implement proper scaling that follows the mouse
    float xDistance = float(inMouseCoords.x() - inOriginalCoords.x());
    float yDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    if (qFuzzyIsNull(yDistance))
        return;

    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();

    QVector4D direction1;
    QVector4D direction2;
    if (m_selectionWidget.isXAxis(m_pickedWidget)) {
        direction1 = QVector4D(1.f, 0.f, 0.f, 0.f);
        direction2 = QVector4D(0.f, 0.f, 0.f, 0.f);
    } else if (m_selectionWidget.isYAxis(m_pickedWidget)) {
        direction1 = QVector4D(0.f, 1.f, 0.f, 0.f);
        direction2 = QVector4D(0.f, 0.f, 0.f, 0.f);
    } else if (m_selectionWidget.isZAxis(m_pickedWidget)) {
        direction1 = QVector4D(0.f, 0.f, 1.f, 0.f);
        direction2 = QVector4D(0.f, 0.f, 0.f, 0.f);
    } else if (m_selectionWidget.isXYPlane(m_pickedWidget)) {
        direction1 = QVector4D(1.f, 0.f, 0.f, 0.f);
        direction2 = QVector4D(0.f, 1.f, 0.f, 0.f);
    } else if (m_selectionWidget.isYZPlane(m_pickedWidget)) {
        direction1 = QVector4D(0.f, 1.f, 0.f, 0.f);
        direction2 = QVector4D(0.f, 0.f, 1.f, 0.f);
    } else if (m_selectionWidget.isZXPlane(m_pickedWidget)) {
        direction1 = QVector4D(0.f, 0.f, 1.f, 0.f);
        direction2 = QVector4D(1.f, 0.f, 0.f, 0.f);
    }

    QVector3D scaleMultiplier(1.0f + xDistance * (1.0f / 40.0f) * direction1.x()
                              + yDistance * (1.0f / 40.0f) * direction2.x(),
                              1.0f + xDistance * (1.0f / 40.0f) * direction1.y()
                              + yDistance * (1.0f / 40.0f) * direction2.y(),
                              1.0f + xDistance * (1.0f / 40.0f) * direction1.z()
                              + yDistance * (1.0f / 40.0f) * direction2.z());

    if (!qFuzzyIsNull(scaleMultiplier.x()) && !qFuzzyIsNull(scaleMultiplier.y())
            && !qFuzzyIsNull(scaleMultiplier.z())) {
        m_selectionWidget.setScale(m_pickedWidget, scaleMultiplier);
        m_currentDragState.s = QVector3D(m_beginDragState.s.x() * scaleMultiplier.x(),
                                         m_beginDragState.s.y() * scaleMultiplier.y(),
                                         m_beginDragState.s.z() * scaleMultiplier.z());

        Q3DSPropertyChangeList list;
        list.append(node->setScale(m_currentDragState.s));
        node->notifyPropertyChanges(list);
        inEditor.EnsureEditor(QObject::tr("Set Scale"), __FILE__, __LINE__)
            .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                      objectDefinitions().m_Node.m_Scale,
                                      qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.s)));
        inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
    }
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
    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;
    QMatrix4x4 parentMatrix = parentAttached->globalTransform;

    adjustRotationLeftToRight(&cameraMatrix);
    adjustRotationLeftToRight(&parentMatrix);

    QVector3D cameraDirection = getDirection(cameraMatrix);
    QQuaternion origRotation = QQuaternion::fromEulerAngles(m_beginDragState.r);

    QVector3D position; // Dummy, not used
    QVector3D scale; // Dummy, not used
    QQuaternion parentRotation;
    decomposeQMatrix4x4(parentMatrix, position, parentRotation, scale);

    QQuaternion yRotation = QQuaternion::fromAxisAndAngle(cameraDirection, -.2f * theYDistance
                                                          * float(g_rotationScaleFactor));
    origRotation = parentRotation * origRotation;
    yRotation *= origRotation;
    yRotation = parentRotation.inverted() * yRotation;

    m_currentDragState.r = yRotation.toEulerAngles();
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
    if (m_pickedWidget) {
        rotateAlongWidget(inOriginalCoords, inMouseCoords, inEditor);
        return;
    }

    QPoint mousePos = inLockToAxis ? getAxisLockedMousePos(inMouseCoords, inOriginalCoords)
                                   : inMouseCoords;

    float theXDistance = float(mousePos.x() - inOriginalCoords.x());
    float theYDistance = float(mousePos.y() - inOriginalCoords.y());

    if (qFuzzyIsNull(theXDistance) && qFuzzyIsNull(theYDistance))
        return;

    Q3DSCameraNode *cameraNode = m_dragCamera;
    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();
    Q3DSNodeAttached *cameraAttached = cameraNode->attached<Q3DSNodeAttached>();
    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;
    QMatrix4x4 parentMatrix = parentAttached->globalTransform;

    adjustRotationLeftToRight(&cameraMatrix);
    adjustRotationLeftToRight(&parentMatrix);

    QVector3D xAxis = getXAxis(cameraMatrix);
    QVector3D yAxis = getYAxis(cameraMatrix);
    QVector3D rotAxis = theXDistance * yAxis + theYDistance * xAxis;
    float distance = rotAxis.length();
    rotAxis.normalize();
    QQuaternion rotation = QQuaternion::fromAxisAndAngle(rotAxis, -.2f * distance
                                                         * float(g_rotationScaleFactor));
    QQuaternion origRotation = QQuaternion::fromEulerAngles(m_beginDragState.r);

    QVector3D position; // Dummy, not used
    QVector3D scale; // Dummy, not used
    QQuaternion parentRotation;
    decomposeQMatrix4x4(parentMatrix, position, parentRotation, scale);

    origRotation = parentRotation * origRotation;
    rotation *= origRotation;
    rotation = parentRotation.inverted() * rotation;

    m_currentDragState.r = rotation.toEulerAngles();

    Q3DSPropertyChangeList list;
    list.append(node->setRotation(m_currentDragState.r));
    node->notifyPropertyChanges(list);
    inEditor.EnsureEditor(QObject::tr("Set Rotation"), __FILE__, __LINE__)
        .SetInstancePropertyValue(m_doc.GetSelectedInstance(),
                                  objectDefinitions().m_Node.m_Rotation,
                                  qt3dsdm::SValue(QVariant::fromValue(m_currentDragState.r)));
    inEditor.FireImmediateRefresh(m_doc.GetSelectedInstance());
}

void Q3DSTranslation::rotateAlongWidget(const QPoint &inOriginalCoords,
                                        const QPoint &inMouseCoords,
                                        CUpdateableDocumentEditor &inEditor)
{
    // TODO: implement proper rotation that follows the mouse
    QVector4D direction;
    if (m_selectionWidget.isXYCircle(m_pickedWidget)) {
        direction = QVector4D(1.f, 0.f, 0.f, 0.f);
    } else if (m_selectionWidget.isYZCircle(m_pickedWidget)) {
        direction = QVector4D(0.f, 1.f, 0.f, 0.f);
    } else if (m_selectionWidget.isZXCircle(m_pickedWidget)) {
        direction = QVector4D(0.f, 0.f, 1.f, 0.f);
    } else if (m_selectionWidget.isCameraCircle(m_pickedWidget)) {
        rotateAboutCameraDirectionVector(inOriginalCoords, inMouseCoords, inEditor);
        return;
    }

    float yDistance = float(inMouseCoords.y() - inOriginalCoords.y());
    Q3DSNode *node = m_dragTranslator->graphObject<Q3DSNode>();

    QQuaternion yRotation = QQuaternion::fromAxisAndAngle(direction.toVector3D(), .1f * yDistance
                                                          * float(g_rotationScaleFactor));
    QQuaternion origRotation = QQuaternion::fromEulerAngles(m_beginDragState.r);
    yRotation = origRotation * yRotation;
    m_currentDragState.r = yRotation.toEulerAngles();

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
