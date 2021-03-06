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

#ifndef PRESENTATIONFILE_H
#define PRESENTATIONFILE_H

#include "ProjectFile.h"

#include <QtCore/qfile.h>
#include <QtXml/qdom.h>
#include <QtCore/qmap.h>

QT_FORWARD_DECLARE_CLASS(QDir)
QT_FORWARD_DECLARE_CLASS(QFileInfo)
class CDataInputDialogItem;

class PresentationFile
{
public:
    static void getSourcePaths(const QFileInfo &uipSrc, const QFileInfo &uipTarget,
                               QHash<QString, QString> &outPathMap, QString &outRootPath,
                               QHash<QString, QString> &outPresentationNodes,
                               QSet<QString> &outDataInputs, QSet<QString> &outDataOutputs);
    static void updatePresentationId(const QString &url, const QString &oldId,
                                     const QString &newId);
    static void renameMaterial(const QString &uipPath, const QString &oldName,
                               const QString &newName);
    static QSize readSize(const QString &uipPath);
    static QString findProjectFile(const QString &uipPath);
    static bool getDataInputBindings(
            const SubPresentationRecord &subpresentation,
            QMultiHash<QString, ProjectFile::DataInputOutputBinding> &outmap);
    static bool getDataOutputBindings(
            const SubPresentationRecord &subpresentation,
            QMultiHash<QString, ProjectFile::DataInputOutputBinding> &outmap);

private:
    static bool getDataInputOutputBindings(
            const SubPresentationRecord &subpresentation,
            QMultiHash<QString, ProjectFile::DataInputOutputBinding> &outmap,
            const QString &dioProperty);
    PresentationFile();
};

#endif // PRESENTATIONFILE_H
