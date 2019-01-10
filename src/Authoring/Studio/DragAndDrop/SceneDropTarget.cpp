/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "Qt3DSCommonPrecompile.h"
#include "SceneDropTarget.h"
#include "DropTarget.h"
#include "DropSource.h"
#include "StudioApp.h"
#include "Doc.h"
#include "IDocumentEditor.h"
#include "HotKeys.h"
#include "IDropTargetHelper.h"
#include "Core.h"
#include "GraphUtils.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMSlides.h"
#include "PresentationFile.h"
#include "QtWidgets/qmessagebox.h"
#include "QtWidgets/qpushbutton.h"
#include "IDocSceneGraph.h"

// Sceneview stuff
//===============================================================================
/**
 * 	Constructor.
 */
CSceneViewDropTarget::CSceneViewDropTarget()
{
    m_ObjectType = OBJTYPE_LAYER;
}

//===============================================================================
/**
 * 	 This will get the objec ttype from the Asset.
 *	 Note: The asset can change all of the time, so i always ask the asset for its type.
 *	@return the Studio object type.
 */
long CSceneViewDropTarget::GetObjectType()
{
    qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
    if (theInstance.Valid()) {
        CClientDataModelBridge *theBridge =
            g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
        m_ObjectType = theBridge->GetObjectType(theInstance);
        return m_ObjectType;
    }
    return OBJTYPE_UNKNOWN;
}

//===============================================================================
/**
 * 	This get called on every DragWithin.
 *	Note: the source will validate the target instead of the otherway around.
 *	This is because the DropSource knows how to get information from itself without
 *	creating an asset. This is mainly for DropSources that have a lazy creation idiom.
 *  like files.
 *	Dropping into "locked" layers are not allowed.
 *	@param inSource	the DropSource in question.
 *	@return true if the DropSource likes the DropTarget.
 */
bool CSceneViewDropTarget::Accept(CDropSource &inSource)
{
    // We have to set this so we can adjust the Target to accept this source.
    m_DropSourceObjectType = inSource.GetObjectType();
    m_DropSourceFileType = inSource.getFileType();

    // always allow DnD presentations and qml streams to the scene
    if (m_DropSourceObjectType & (OBJTYPE_PRESENTATION | OBJTYPE_QML_STREAM)) {
        inSource.SetHasValidTarget(true);
        return true;
    }

    if (m_DropSourceObjectType == OBJTYPE_MATERIALDATA) {
        CDoc *doc = g_StudioApp.GetCore()->GetDoc();
        m_objectRequestPending = doc->GetSceneGraph()->requestObjectAt(inSource.GetCurrentPoint());
    }

    bool theAcceptable = false;
    // We don't want to generate an asset right now so let the DropSource ask us if it can drop.
    theAcceptable = inSource.ValidateTarget(this);

    // The DropSource already generated the asset for this in the above.
    CClientDataModelBridge *theBridge
            = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
    if (theAcceptable && m_Instance.Valid())
        theAcceptable = !theBridge->IsLockedAtAll(m_Instance);

    return theAcceptable;
}

//===============================================================================
/**
 *	This is where is actually happens.
 *	Note: At this point either everything should be verified, and setup in the dropsource.
 *	Then the only thing left to do is to get the Assets and move/copy or connect them.
 *	Or the dropsource needs the target to perform the actual drop.
 *	Note that if the Control key is depressed, the start time follows the current view time(
 *i.e. playhead position )
 *  And if the Alt key (KEY_MENU) is depressed, the object is dropped at the mouse location.
 *
 *	@param inSource the Object in question.
 *	@return true if the drop was successful .
 */
bool CSceneViewDropTarget::Drop(CDropSource &inSource)
{
    // The Parent is a tree control item, so iwe know it can be converted to an Asset.
    // We have to set this so we can adjust the Target to accept this source.
    m_DropSourceObjectType = inSource.GetObjectType();
    m_DropSourceFileType = inSource.getFileType();

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::Qt3DSDMInstanceHandle instance = GetInstance();
    if (m_DropSourceObjectType & (OBJTYPE_PRESENTATION | OBJTYPE_QML_STREAM)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(QObject::tr("Set Sub-presentation"));
        msgBox.setText(QObject::tr("Set as sub-presentation to"));
        QPushButton *layerButton = msgBox.addButton(QObject::tr("Layer"), QMessageBox::YesRole);
        QPushButton *textureButton = msgBox.addButton(QObject::tr("Texture"), QMessageBox::NoRole);
        msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();
        if (msgBox.clickedButton() == layerButton) {
            instance = doc->GetActiveLayer();
            // The GenerateAssetCommand below will take care of setting the subpresentation
        } else if (msgBox.clickedButton() == textureButton) { // texture
            instance = doc->GetActiveRootInstance();
            // The GenerateAssetCommand below will take care of setting the subpresentation
        } else {
            return true; // cancel
        }
    }

    if (m_DropSourceObjectType == OBJTYPE_MATERIALDATA)
        m_objectRequestPending = doc->GetSceneGraph()->requestObjectAt(inSource.GetCurrentPoint());

    if (instance.Valid()) {
        qt3dsdm::ISlideSystem *theSlideSystem = doc->GetStudioSystem()->GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle theSlide = doc->GetActiveSlide();
        if (!theSlideSystem->IsMasterSlide(theSlide)
            && (inSource.GetCurrentFlags() & CHotKeys::MODIFIER_ALT)) {
            if (CanAddToMaster()) {
                qt3dsdm::Qt3DSDMSlideHandle theMasterSlideHandle =
                    theSlideSystem->GetMasterSlide(theSlide);
                if (theMasterSlideHandle.Valid())
                    theSlide = theMasterSlideHandle;
            }
        }

        CCmd *command = inSource.GenerateAssetCommand(instance, EDROPDESTINATION_ON, theSlide);
        if (command)
            doc->GetCore()->ExecuteCommand(command);
    }

    return true;
}

//===============================================================================
/**
 * 	 @return the Asset that we would like the DropSource to drop on to.
 */
qt3dsdm::Qt3DSDMInstanceHandle CSceneViewDropTarget::GetInstance()
{
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::Qt3DSDMInstanceHandle theRootObject = theDoc->GetActiveRootInstance();
    EStudioObjectType theRootObjType =
        theDoc->GetStudioSystem()->GetClientDataModelBridge()->GetObjectType(theRootObject);

    // Check if the inObjectType can just go ahead and drop onto the Root object.
    if (CStudioObjectTypes::AcceptableParent((EStudioObjectType)m_DropSourceObjectType,
                                             theRootObjType)
        || m_DropSourceFileType == Q3DStudio::DocumentEditorFileType::Image) {
        m_Instance = theRootObject;
    } else if (theRootObject == theDoc->GetSceneInstance()
               && CStudioObjectTypes::AcceptableParent((EStudioObjectType)m_DropSourceObjectType,
                                                       OBJTYPE_LAYER)) {
        m_Instance = theDoc->GetActiveLayer();
    }

    return m_Instance;
}

//===============================================================================
/**
 *		Check to see if the Asset is a relative of our asset.
 * 		@return true if the inAsset is a parent grandparent...etc. of this asset.
 */
bool CSceneViewDropTarget::IsRelative(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    bool theReturn = false;

    qt3dsdm::Qt3DSDMInstanceHandle theThisInstance = GetInstance();
    // This will check to see if the inAsset is already a parent, grandparent....etc.
    if (theThisInstance.Valid())
        theReturn = IsAscendant(theThisInstance, inInstance,
                                g_StudioApp.GetCore()->GetDoc()->GetAssetGraph());
    return theReturn;
}

//===============================================================================
/**
 * 	 Check to see if the inAsset is our asset.
 *	 @param inAsset The Asset to check.
 *	 @return true if we are the same.
 */
bool CSceneViewDropTarget::IsSelf(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    qt3dsdm::Qt3DSDMInstanceHandle theThisInstance = GetInstance();
    return (theThisInstance == inInstance);
}
