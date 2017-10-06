/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#pragma once
#ifndef UICIMPORTPERFORMIMPORTH
#define UICIMPORTPERFORMIMPORTH
#include "UICDMDataTypes.h"
#include "UICDMHandles.h"
#include "foundation/Qt3DS.h"
#include "UICString.h"
#include "UICDMHandles.h"
#include "UICFileTools.h"
#include "UICDMAnimation.h"
#include "UICImportErrorCodes.h"
#include "UICDMComposerTypeDefinitions.h"

namespace Q3DStudio {
class CFilePath;
};

namespace UICIMP {
using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace std;
using namespace UICDM;

struct ParentChildLink;
struct PropertyValue;
class Import;

struct ImportErrorData
{
    ImportErrorCodes::Enum m_Error;
    const wchar_t *m_ExtraInfo;
    explicit ImportErrorData(ImportErrorCodes::Enum err, TCharPtr extraInfo)
        : m_Error(err)
        , m_ExtraInfo(extraInfo)
    {
    }
    ImportErrorData()
        : m_Error(ImportErrorCodes::NoError)
        , m_ExtraInfo(L"")
    {
    }

    void PrintErrorString(wchar_t *buffer, QT3DSU32 bufLen)
    {
        if (m_Error != ImportErrorCodes::NoError)
            swprintf(buffer, bufLen, ImportErrorCodes::GetEnglishFormatString(m_Error),
                     m_ExtraInfo);
        else
            swprintf(buffer, bufLen, L"No error");
    }
};

typedef SValue TImportModelValue;

struct PropertyValue
{
    ComposerPropertyNames::Enum m_Name;
    TImportModelValue m_Value;

    PropertyValue(ComposerPropertyNames::Enum nm, const TImportModelValue &val)
        : m_Name(nm)
        , m_Value(val)
    {
    }
    PropertyValue()
        : m_Name(ComposerPropertyNames::Unknown)
    {
    }
};

typedef const wchar_t *TImportId;

/**
 *	Interface to allow abstract UIComposer details (doc, slides, etc)
 *	from the operation of applying a import result to composer.
 *
 *	Implementations must remember that a document may be imported several times.
 *	Each time the import id will be incremented.
 *
 *	If they can't guarantee they will give back a unique handle every time the
 *	document is imported, including in the case where they create a new project
 *	and import a previously imported document into the new project, then they
 *	need to use the combination of import id and handle in order to identify
 *	precisely which object the import library is attempting to update.
 */
class IComposerEditor
{
protected:
    virtual ~IComposerEditor() {}
public:
    virtual void Release() = 0;
    virtual void BeginImport(Import &importObj) = 0;
    virtual void RemoveChild(TImportId inParentId, TImportId inChildId) = 0;
    virtual void RemoveInstance(TImportId parent) = 0;
    /**
     *	Note that instance properties that point to files (sourcepath generally) point to files
     *	relative to the import file.  You need to do combineBaseAndRelative with those files
     *	and the a new getRelativeFromBase with the final file in order to transfer data
     *	successfully.  The time to do this sort of thing is upon create or update instance.
     */
    virtual void CreateRootInstance(TImportId inImportId, ComposerObjectTypes::Enum type) = 0;
    // inParent may be null (or an invalid handle) if the instance doesn't have a parent (images)
    virtual void CreateInstance(TImportId inImportId, ComposerObjectTypes::Enum type,
                                TImportId inParent) = 0;

    // We guarantee that all instances will be created before their properties are updated thus you
    // can resolve references during this updateInstanceProperties call if necessary.
    virtual void UpdateInstanceProperties(TImportId hdl, const PropertyValue *propertBuffer,
                                          QT3DSU32 propertyBufferSize) = 0;
    // This is called even for new instances where we told you the parent because
    // they may be out of order so if the child already has this parent relationship you need
    // to check the order and ensure that is also (somewhat) correct.
    virtual void AddChild(TImportId parent, TImportId child, TImportId nextSibling) = 0;
    virtual void RemoveAnimation(TImportId hdl, const wchar_t *propName, long propSubIndex) = 0;
    virtual void UpdateAnimation(TImportId hdl, const wchar_t *propName, long propSubIndex,
                                 EAnimationType animType, const float *animData,
                                 QT3DSU32 numFloats) = 0;
    virtual void AddAnimation(TImportId hdl, const wchar_t *propName, long propSubIndex,
                              EAnimationType animType, const float *animData, QT3DSU32 numFloats) = 0;

    virtual void EndImport() = 0;
};

// Translators perform src file to import translation,
// so from collada to import format translation.
class ITranslator
{
protected:
    virtual ~ITranslator() {}
public:
    virtual void Release() = 0;

    virtual const QString &GetSourceFile() = 0;
    // Returning false causes the rest of the import or refresh process
    // to fail.
    virtual bool PerformTranslation(Import &import) = 0;
};

// The result of an import or a refresh.  The file path contains a string that
// will have a import file version number at the end.
struct SImportResult
{
    ImportErrorCodes::Enum m_Error;
    // The file path will probably contain a version number at the end of the
    // string, so you need to be aware of this.
    Q3DStudio::CFilePath m_FilePath;

    SImportResult(ImportErrorCodes::Enum inError)
        : m_Error(inError)
    {
    }
    SImportResult(const Q3DStudio::CFilePath &inFilePath, QT3DSU32 inFileVersion);
};

// The file path

class CPerformImport
{
public:
    // Note that the translator has the source file in all the functions below,
    // so we don't pass in "colladaFile" anywhere but we get the source file
    // from the translator.

    /**
      *	Exception safe!  translator and importer are always released!!
      *	2.  Perform translation to import format
      *	3.  Diff original with new translation
      *	4.  Transfer results to each composer interface
      *	5.  Save import results out original file under new version
      *	6.	Release translator and importer
      *
      *	This interface is meant to refresh each and every composer interface
      *	that was imported.
      */
    static SImportResult RefreshToComposer(ITranslator &translator, IComposerEditor &inComposers,
                                           Import &inOriginalFile,
                                           const Q3DStudio::CFilePath &ioImportFile);

    /**
      *	Exception safe!  translator and importer are always released!!
      *	1.  Load original file
      *	2.  Perform translation to import format
      *	3.  Diff original with new translation
      *	4.  Transfer results to each composer interface
      *	5.  Save import results out original file under new version
      *	6.	Release translator and importer
      *
      *	This interface is meant to refresh each and every composer interface
      *	that was imported.
      */
    static SImportResult RefreshToComposer(ITranslator &translator, IComposerEditor &inComposers,
                                           const Q3DStudio::CFilePath &ioImportFile);
    /**
      *	Exception safe!  translator and importer are always released!!
      *	1.  Create blank importer
      *	2.  Perform translation to import format
      *	3.  Transfer result to composer and update import with uicdm handles
      *	4.  Save import results out into destination
      *	5.	Release translator and importer
      *
      *	If the import file exists, a new import version will be written to the file.
      */
    static SImportResult ImportToComposer(ITranslator &translator, IComposerEditor &composer,
                                          const Q3DStudio::CFilePath &inImportFile);

    /**
     *	Load the dest file, and import the objects into composer.
     *	Do steps 3+ of ImportToComposer (sans save step).
     *
     *	The import file must exist.
     */
    static SImportResult ImportToComposerFromImportFile(IComposerEditor &composer,
                                                        const Q3DStudio::CFilePath &inImportFile);

    /**
     *	Translate an outside file type to an import file but don't import that file.  If thei mport
     *file exists a new
     *	version is written to the import file.
     */
    static SImportResult TranslateToImportFile(ITranslator &translator,
                                               const Q3DStudio::CFilePath &inImportFile);

    static void DoImportToComposer(Import &import, IComposerEditor &composer);
};
}
#endif
