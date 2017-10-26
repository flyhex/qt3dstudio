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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Include
//==============================================================================
#include "CmdLocateReference.h"
#include "Doc.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMSlideCore.h"

//=============================================================================
/**
 * Constructor
 */
CCmdLocateReference::CCmdLocateReference(CDoc *inDoc, const Q3DStudio::CFilePath &inOldPath,
                                         const Q3DStudio::CFilePath &inNewPath)
    : qt3dsdm::CmdDataModel(*inDoc)
    , m_Doc(inDoc)
    , m_OldPath(inOldPath)
    , m_NewPath(inNewPath)
{
    // Convert the path to relative
    if (m_OldPath.IsAbsolute()) {
        m_OldPath = m_Doc->GetRelativePathToDoc(m_OldPath);
        ASSERT(!m_OldPath.IsAbsolute());
    }
    if (m_NewPath.IsAbsolute()) {
        m_NewPath = m_Doc->GetRelativePathToDoc(m_NewPath);
        ASSERT(!m_NewPath.IsAbsolute());
    }

    m_Bridge = m_Doc->GetStudioSystem()->GetClientDataModelBridge();
    m_PropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    m_SlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    m_SlideCore = m_Doc->GetStudioSystem()->GetSlideCore();
}

//=============================================================================
/**
 * Destructor
 */
CCmdLocateReference::~CCmdLocateReference()
{
}

//=============================================================================
/**
 * Do/Redo
 */
unsigned long CCmdLocateReference::Do()
{
    if (!ConsumerExists()) {
        qt3dsdm::SScopedDataModelConsumer __scopedConsumer(*this);
        LocateReference();
    } else {
        DataModelRedo();
    }
    return 0;
}

//=============================================================================
/**
 * Undo
 */
unsigned long CCmdLocateReference::Undo()
{
    if (ConsumerExists()) {
        DataModelUndo();
    }
    return 0;
}

//=============================================================================
/**
 * LocateReference
 */
void CCmdLocateReference::LocateReference()
{
    using namespace qt3dsdm;
    // Get all instances that are derived from ItemBase instance (they should have sourcepath
    // property)
    // Iterate through each instance derived from ItemBase and get the sourcepath property value.
    TInstanceHandleList theInstances = m_Bridge->GetItemBaseInstances();
    for (TInstanceHandleList::const_iterator theIter = theInstances.begin();
         theIter != theInstances.end(); ++theIter) {
        // Check if the instance is in master slide and if the property is unlinked.
        // This will determine how we should update the value.
        Qt3DSDMSlideHandle theSlide = m_SlideSystem->GetAssociatedSlide(*theIter);
        if (theSlide.Valid() && m_SlideSystem->IsMasterSlide(theSlide)
            && !m_SlideSystem->IsPropertyLinked(*theIter, m_Bridge->GetSourcePathProperty())) {
            // If the instance is in master slide and the property is unlinked, we need to update
            // the value from each slides
            size_t theSlideCount =
                m_SlideSystem->GetSlideCount(m_SlideSystem->GetAssociatedSlide(*theIter));
            for (size_t theSlideIndex = 0; theSlideIndex < theSlideCount; ++theSlideIndex) {
                Qt3DSDMSlideHandle theSpecificSlide =
                    m_SlideSystem->GetSlideByIndex(theSlide, theSlideIndex);
                UpdateSourcePath(*theIter, theSpecificSlide);
            }
        } else {
            UpdateSourcePath(*theIter);
        }
    }
}

//=============================================================================
/**
 * Update sourcepath property from a specific instance (and maybe from specific slide) if it meets
 * the criteria
 * @param inInstance		the instance to update
 * @param inSpecificSlide	the slide to update, if valid
 */
void CCmdLocateReference::UpdateSourcePath(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                           qt3dsdm::Qt3DSDMSlideHandle inSpecificSlide)
{
    // Get the sourcepath value
    Q3DStudio::CFilePath thePath;
    Q3DStudio::CString theIdentifier;
    GetSourcePath(inInstance, inSpecificSlide, thePath, theIdentifier);

    // compare path. subclass should implement ComparePath function
    if (ComparePath(thePath)) {
        // Set to New Path. subclass should implement GetNewPath function
        SetSourcePath(inInstance, inSpecificSlide, GetNewPath(thePath), theIdentifier);
    }
}

//=============================================================================
/**
 * Get sourcepath property from a specific instance
 * @param inInstance		the instance to query
 * @param inSpecificSlide	the slide to query, if valid
 * @param outPath			the sourcepath without the identifier
 * @param outIdentifier		the identifier
 */
void CCmdLocateReference::GetSourcePath(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                        qt3dsdm::Qt3DSDMSlideHandle inSpecificSlide,
                                        Q3DStudio::CFilePath &outPath,
                                        Q3DStudio::CString &outIdentifier)
{
    // Get the sourcepath property value
    qt3dsdm::SValue theValue;
    bool theGetValue = false;

    if (inSpecificSlide.Valid())
        theGetValue = m_SlideCore->GetSpecificInstancePropertyValue(
            inSpecificSlide, inInstance, m_Bridge->GetSourcePathProperty(), theValue);
    else {
        qt3dsdm::SValue temp;
        theGetValue = m_PropertySystem->GetInstancePropertyValue(
            inInstance, m_Bridge->GetSourcePathProperty(), temp);
        if (theGetValue)
            theValue = temp.toOldSkool();
    }

    if (theGetValue) {
        Q3DStudio::CFilePath theSourcePath(qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue)->GetData());

        // Split the source path and the identifier
        outIdentifier = theSourcePath.GetIdentifier();
        outPath = theSourcePath.GetPathWithoutIdentifier();
    } else {
        outIdentifier.Clear();
        outPath.Clear();
    }
}

//=============================================================================
/**
 * Set sourcepath property for a specific instance
 * @param inInstance		the instance to set value to
 * @param inSpecificSlide	the slide to set value to, if valid
 * @param inPath			the sourcepath without the identifier
 * @param inIdentifier		the identifier
 */
void CCmdLocateReference::SetSourcePath(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                        qt3dsdm::Qt3DSDMSlideHandle inSpecificSlide,
                                        const Q3DStudio::CFilePath &inPath,
                                        const Q3DStudio::CString &inIdentifier)
{
    Q3DStudio::CFilePath theSourcePath(inPath);
    theSourcePath.SetIdentifier(inIdentifier);
    qt3dsdm::SValue theValue(qt3dsdm::TDataStrPtr(new qt3dsdm::CDataStr(theSourcePath)));

    if (inSpecificSlide.Valid())
        m_SlideCore->ForceSetInstancePropertyValue(inSpecificSlide, inInstance,
                                                   m_Bridge->GetSourcePathProperty(), theValue);
    else
        m_PropertySystem->SetInstancePropertyValue(inInstance, m_Bridge->GetSourcePathProperty(),
                                                   theValue);
}

//=============================================================================
/**
 * Constructor
 * @param inDoc			CDoc
 * @param inOldPath		the old file path
 * @param inNewPath		the new file path
 */
CCmdLocateFile::CCmdLocateFile(CDoc *inDoc, const Q3DStudio::CFilePath &inOldPath,
                               const Q3DStudio::CFilePath &inNewPath)
    : CCmdLocateReference(inDoc, inOldPath, inNewPath)
{
}

//=============================================================================
/**
 * Destructor
 */
CCmdLocateFile::~CCmdLocateFile()
{
}

//=============================================================================
/**
 * ComparePath
 */
bool CCmdLocateFile::ComparePath(const Q3DStudio::CFilePath &inPath)
{
    // Do a caseless comparison between inPath and the old path
    return inPath.Compare(m_OldPath, false);
}

//=============================================================================
/**
 * GetNewPath
 */
Q3DStudio::CFilePath CCmdLocateFile::GetNewPath(const Q3DStudio::CFilePath &inPath)
{
    // Simply replace inPath with the new path
    Q_UNUSED(inPath);
    return m_NewPath;
}

//=============================================================================
/**
 * ToString
 */
QString CCmdLocateFile::ToString()
{
    return QObject::tr("Locate File");
}

//=============================================================================
/**
 * Constructor
 * @param inDoc			CDoc
 * @param inOldPath		the old folder path
 * @param inNewPath		the new folder path
 */
CCmdLocateFolder::CCmdLocateFolder(CDoc *inDoc, const Q3DStudio::CFilePath &inOldPath,
                                   const Q3DStudio::CFilePath &inNewPath)
    : CCmdLocateReference(inDoc, inOldPath, inNewPath)
{
}

//=============================================================================
/**
 * Destructor
 */
CCmdLocateFolder::~CCmdLocateFolder()
{
}

//=============================================================================
/**
 * ComparePath
 */
bool CCmdLocateFolder::ComparePath(const Q3DStudio::CFilePath &inPath)
{
    // check if inPath is in the m_OldPath folder
    return inPath.IsInSubDirectory(m_OldPath);
}

//=============================================================================
/**
 * GetNewPath
 */
Q3DStudio::CFilePath CCmdLocateFolder::GetNewPath(const Q3DStudio::CFilePath &inPath)
{
    // Change inPath from old folder to new folder
    Q3DStudio::CFilePath theNewPath(m_NewPath + inPath.Extract(m_OldPath.Length()));
    return theNewPath;
}

//=============================================================================
/**
 * ToString
 */
QString CCmdLocateFolder::ToString()
{
    return QObject::tr("Locate Folder");
}
