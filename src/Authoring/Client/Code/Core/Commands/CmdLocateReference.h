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
#ifndef INCLUDED_CMD_LOCATE_REFERENCE_H
#define INCLUDED_CMD_LOCATE_REFERENCE_H 1

#pragma once

//==============================================================================
//	Include
//==============================================================================
#include "Cmd.h"
#include "CmdDataModel.h"
#include "UICFileTools.h"

//==============================================================================
//	Forwards
//==============================================================================
class CDoc;
class CClientDataModelBridge;
namespace qt3dsdm {
class IPropertySystem;
class ISlideSystem;
class ISlideCore;
}

// Left clicking on the Broken Link Icon opens a pop-up menu
//
//    The menu has a single item
//        If the Broken Link is a Folder: "Locate Folder"
//        If the Broken Link is a File: "Locate File"
//        Selecting this will open a file browser dialog allowing the use to choose any folder (for
//        broken folder) or file (for broken file) in the Project path
//        This operation will replace the old (broken) path substring with the new path substring in
//        all references in the scene
//            For broken folders this will effectively fix all files that are broken in the path
//            (commonly occurs when a folder is moved)
//            For broken files this will only fix the specific file that was located
//            The project palette will update to reflect the changes

class CCmdLocateReference : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    Q3DStudio::CFilePath m_OldPath;
    Q3DStudio::CFilePath m_NewPath;
    CClientDataModelBridge *m_Bridge;
    qt3dsdm::IPropertySystem *m_PropertySystem;
    qt3dsdm::ISlideSystem *m_SlideSystem;
    qt3dsdm::ISlideCore *m_SlideCore;

public:
    CCmdLocateReference(CDoc *inDoc, const Q3DStudio::CFilePath &inOldPath,
                        const Q3DStudio::CFilePath &inNewPath);
    ~CCmdLocateReference();

    unsigned long Do() override;
    unsigned long Undo() override;
    QString ToString() override = 0;

protected:
    void LocateReference();
    void UpdateSourcePath(qt3dsdm::CUICDMInstanceHandle inInstance,
                          qt3dsdm::CUICDMSlideHandle inSpecificSlide = 0);
    void GetSourcePath(qt3dsdm::CUICDMInstanceHandle inInstance,
                       qt3dsdm::CUICDMSlideHandle inSpecificSlide, Q3DStudio::CFilePath &outPath,
                       Q3DStudio::CString &outIdentifier);
    void SetSourcePath(qt3dsdm::CUICDMInstanceHandle inInstance,
                       qt3dsdm::CUICDMSlideHandle inSpecificSlide, const Q3DStudio::CFilePath &inPath,
                       const Q3DStudio::CString &inIdentifier);

    // subclasses should implement the following methods
    virtual bool ComparePath(const Q3DStudio::CFilePath &inPath) = 0;
    virtual Q3DStudio::CFilePath GetNewPath(const Q3DStudio::CFilePath &inPath) = 0;
};

class CCmdLocateFile : public CCmdLocateReference
{
public:
    CCmdLocateFile(CDoc *inDoc, const Q3DStudio::CFilePath &inOldPath,
                   const Q3DStudio::CFilePath &inNewPath);
    ~CCmdLocateFile();

    bool ComparePath(const Q3DStudio::CFilePath &inPath) override;
    Q3DStudio::CFilePath GetNewPath(const Q3DStudio::CFilePath &inPath) override;

    QString ToString() override;
};

class CCmdLocateFolder : public CCmdLocateReference
{
public:
    CCmdLocateFolder(CDoc *inDoc, const Q3DStudio::CFilePath &inOldPath,
                     const Q3DStudio::CFilePath &inNewPath);
    ~CCmdLocateFolder();

    bool ComparePath(const Q3DStudio::CFilePath &inPath) override;
    Q3DStudio::CFilePath GetNewPath(const Q3DStudio::CFilePath &inPath) override;

    QString ToString() override;
};

#endif
