/****************************************************************************
 * *
 ** Copyright (C) 1999-2004 NVIDIA Corporation.
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
//  Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//  Includes
//==============================================================================
#include "StudioClipboard.h"

#include "Literals.h"
#include "StudioObjectTypes.h"
#include "Qt3DSString.h"

#include <QClipboard>
#include <QDataStream>
#include <QFile>
#include <QGuiApplication>
#include <QMimeData>

Qt3DSFile CStudioClipboard::s_InternalClipObject("");
qint64 CStudioClipboard::s_InternalContextData = 0;
EStudioObjectType CStudioClipboard::s_AssetType(OBJTYPE_UNKNOWN);
qint64 CStudioClipboard::s_CopyProcessID = 0;

//==============================================================================
/**
 *  Destructor.
 */
CStudioClipboard::~CStudioClipboard()
{
}

//==============================================================================
/**
 *  Retrieve a previously stored object from the respective clipboard.
 *  @param  inIsInternal  true if we are retrieving for an internal operation like
 *CmdMakeComponent or CmdDuplicateObject
 *  @param  outContextData  arbitrary data saved earlier
 *  @return  the object that was retrieved from the respective clipboard.
 */
Qt3DSFile CStudioClipboard::GetObjectFromClipboard(bool inIsInternal, qint64 &outContextData)
{
    if (inIsInternal)
        return GetObjectFromInternalClipboard(outContextData);
    else {
        return GetObjectFromGlobalClipboard(outContextData, "application/x-qt3dstudio-clipboard"_L1);
    }
}

//==============================================================================
/**
 *  Store the given object to the respective clipboard.
 *  @param  inTempFile  The object to store to clipboard.
 *  @param  inContextData  any arbitrary data to be retrieved later
 *  @param  inIsInternal  true if we are storing for an internal operation like
 *CmdMakeComponent or CmdDuplicateObject
 *  @param  inAssetType    asset type of object
 */
void CStudioClipboard::CopyObjectToClipboard(Qt3DSFile &inFile, qint64 inContextData,
                                             bool inIsInternal, EStudioObjectType inObjectType)
{
    s_AssetType = inObjectType;

    if (inIsInternal) {
        CopyObjectToInternalClipboard(inFile, inContextData);
    } else {
        CopyObjectToGlobalClipboard(inFile, inContextData, "application/x-qt3dstudio-clipboard"_L1);
    }
}

//==============================================================================
/**
 *  Returns true if there is an object available on the internal or global clipboard.
 *  Make sure the destination asset is capable of attaching the "Pastee".
 *  @param  inDestType    Asset type of destination asset
 *  @param  inIsInternal  true if we are checking this from an internal operation like
 *CmdMakeComponent or CmdDuplicateObject
 *  @return  true if there is an object available on the internal or global clipboard, and
 *the object is paste-able onto the destination object
 */
bool CStudioClipboard::CanPasteObject(EStudioObjectType inDestType, bool inIsInternal /* = false */)
{
    if (inIsInternal) {
        if (CStudioObjectTypes::AcceptableParent(s_AssetType, inDestType))
            return s_InternalClipObject.Exists();
    } else {
        QClipboard *clipboard = QGuiApplication::clipboard();
        auto mimeData = clipboard->mimeData();
        if (mimeData->hasFormat("application/x-qt3dstudio-clipboard"_L1)) {
            GetObjectFromGlobalClipboard("application/x-qt3dstudio-clipboard"_L1);
            if (CStudioObjectTypes::AcceptableParent(s_AssetType, inDestType))
                return true;
        }
    }

    return false;
}

//==============================================================================
/**
 *  Retrieve the object from the internal Clipboard.
 *  @param  outContextData  arbitrary data saved earlier
 *  @return  Qt3DSFile object that was retrieved from the internal Clipboard.
 */
Qt3DSFile CStudioClipboard::GetObjectFromInternalClipboard(qint64 &outContextData)
{
    // Don't have to delete the InternalClipObject as the CmdPasteObject would delete the file later
    outContextData = s_InternalContextData;
    return s_InternalClipObject;
}

//==============================================================================
/**
 *  Copy the input file to the internal clipboard.
 *  @param  inFile  The object to copy to the internal clipboard.
 *  @param  inContextData  any arbitrary data to be retrieved later
 */
void CStudioClipboard::CopyObjectToInternalClipboard(Qt3DSFile &inFile, qint64 inContextData)
{
    s_InternalClipObject = inFile;
    s_InternalContextData = inContextData;
}

//==============================================================================
/**
 *  Retrieve the object from the global Clipboard.
 *  @param  outContextData  arbitrary data saved earlier
 *  @return  Qt3DSFile object that was retrieved from the global Clipboard.
 */
Qt3DSFile CStudioClipboard::GetObjectFromGlobalClipboard(qint64 &outContextData,
                                                        const QString &inMimeType)
{
    Qt3DSFile theTempAPFile = Qt3DSFile::GetTemporaryFile();
    GetObjectFromGlobalClipboard(inMimeType, &theTempAPFile, &outContextData);
    return theTempAPFile;
}

//==============================================================================
/**
 *  Copy the input file to the global clipboard.
 *  @param  inFile  The object to copy to the global clipboard.
 *  @param  inContextData  any arbitrary data to be retrieved later
 */
void CStudioClipboard::CopyObjectToGlobalClipboard(Qt3DSFile &inFile, qint64 inContextData,
                                                   const QString &inMimeType)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_8);
    QFile file(inFile.GetAbsolutePath().toQString());
    if (!file.open(QIODevice::ReadOnly))
        return;

    stream << file.readAll();
    // Append this after the copied data, so that pasting from prior versions of studio would still
    // work.
    stream << inContextData;
    stream << (qint32)s_AssetType;
    stream << s_CopyProcessID;

    QClipboard *clipboard = QGuiApplication::clipboard();
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(inMimeType, data);
    clipboard->setMimeData(mimeData);
}

//==============================================================================
/**
 *  Returns the text that is being stored in the global clipboard.
 *  @return  The text that was retrieved from the global clipboard.
 */
QString CStudioClipboard::GetTextFromClipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    return clipboard->text();
}

//==============================================================================
/**
 *  Copies input text to global clipboard.
 *  @param  inText The text to copy to the global clipboard.
 */
void CStudioClipboard::CopyTextToClipboard(const QString &inText)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(inText);
}

//==============================================================================
/**
 *  Returns true if there is text available on the global clipboard for a paste operation.
 *  @return  true if there is text available on the global clipboard for a paste operation.
 */
bool CStudioClipboard::CanPasteText()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    auto mimeData = clipboard->mimeData();
    return mimeData->hasText();
}

//==============================================================================
/**
 *  Clears the global clipboard.
 */
void CStudioClipboard::ClearClipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->clear();
}

//==============================================================================
/**
 *  Retrieve a previously stored action from the global clipboard.
 *  @return  the object that was retrieved from the respective clipboard.
 */
Qt3DSFile CStudioClipboard::GetActionFromClipboard()
{
    qint64 theContextData = 0;
    return GetObjectFromGlobalClipboard(theContextData, "application/x-qt3dstudio-action"_L1);
}

//==============================================================================
/**
 *  Copy an Action to the Clipboard.
 *  @param  inFile  The object to store to clipboard.
 */
void CStudioClipboard::CopyActionToClipboard(Qt3DSFile &inFile)
{
    s_CopyProcessID = qApp->applicationPid();
    CopyObjectToGlobalClipboard(inFile, 0, "application/x-qt3dstudio-action"_L1);
}

//==============================================================================
/**
 *  Check if there are available actions on the clipboard for paste operations.
 *  @return true if Actions are available to be pasted.
 */
bool CStudioClipboard::CanPasteAction()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    auto mimeData = clipboard->mimeData();
    return mimeData->hasFormat("application/x-qt3dstudio-action"_L1);
}

//==============================================================================
/**
 *  Retrieve the object from the global Clipboard.
 *  @param  inObj      the OleDataObject to extract the data from
 *  @param  inClipFormat  the clipboard format that was registered for
 *  @param  outContextData  arbitrary data saved earlier
 *  @param  outFile      file to write clipboard data to, if any.
 */
void CStudioClipboard::GetObjectFromGlobalClipboard(const QString &inMimeType,
                                                    Qt3DSFile *outFile /*= NULL */,
                                                    qint64 *outContextData /*= NULL */)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    auto mimeData = clipboard->mimeData();
    if (mimeData->hasFormat(inMimeType)) {
        QByteArray clipboardData = mimeData->data(inMimeType);
        QDataStream stream(&clipboardData, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_5_8);
        QByteArray data;
        stream >> data;
        if (outFile) {
            QFile file(outFile->GetAbsolutePath().toQString());
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
            }
        }
        if (outContextData) {
            stream >> *outContextData;
        } else {
            qint64 temp;
            stream >> temp;
        }
        qint32 assetType;
        stream >> assetType;
        s_AssetType = (EStudioObjectType)assetType;
        stream >> s_CopyProcessID;
    }
}

//==============================================================================
/**
 *  Determines if the object of the current paste operation was copied from
 *  another Studio instance. This function should be called only for global
 *  pastes.
 *  @return true if paste target is from another instance of Studio.
 */
bool CStudioClipboard::IsPastingAcrossInstances()
{
    return qApp->applicationPid() != s_CopyProcessID;
}
