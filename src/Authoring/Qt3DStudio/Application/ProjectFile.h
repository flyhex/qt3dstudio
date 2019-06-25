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

#ifndef PROJECTFILE_H
#define PROJECTFILE_H

#include "Qt3DSFileTools.h"
#include <QtXml/qdom.h>

namespace Q3DStudio {
class CFilePath;
class CString;
}
struct SubPresentationRecord;
class CDataInputDialogItem;

class ProjectFile : public QObject
{
    Q_OBJECT

public:
    ProjectFile();

    struct VariantGroup {
        QString m_color;
        QStringList m_tags;
    };

    struct DataInputOutputBinding {
        QString dioName;
        QString propertyName;
        QString propertyDefinitionFile; // E.g. for CustomMaterials, this is the .shader file
    };

    void create(const QString &uiaPath);
    void ensureProjectFile();
    void initProjectFile(const QString &presPath);
    static void parseDataInputElem(const QDomElement &elem,
                                   QMap<QString, CDataInputDialogItem *> &dataInputs);
    static void loadDataInputs(const QString &projFile,
                               QMap<QString, CDataInputDialogItem *> &dataInputs);
    void loadSubpresentationsAndDatainputs(
            QVector<SubPresentationRecord> &subpresentations,
            QMap<QString, CDataInputDialogItem *> &datainputs);
    void writePresentationId(const QString &id, const QString &src = {});
    void updateDocPresentationId();
    void addPresentationNode(const QString &uip, const QString &pId = {});
    void addPresentationNodes(const QHash<QString, QString> &nodeList);
    bool isUniquePresentationId(const QString &id, const QString &src = {}) const;
    QString getUniquePresentationName(const QString &presSrc) const;
    QString getProjectPath() const;
    QString getProjectFilePath() const;
    QString getProjectName() const;
    QString getPresentationId(const QString &src) const;
    QString getAbsoluteFilePathTo(const QString &relFilePath) const;
    QString getRelativeFilePathTo(const QString &absFilePath) const;
    QString createPreview();

    QMultiHash<QString, DataInputOutputBinding> getDiBindingtypesFromSubpresentations() const;

    static QString getInitialPresentationSrc(const QString &uiaPath);
    static void getPresentations(const QString &inUiaPath,
                                 QVector<SubPresentationRecord> &outSubpresentations,
                                 const QString &excludePresentationSrc = {});
    QString initialPresentation() const { return m_initialPresentation; }
    void setInitialPresentation(const QString &initialId);
    bool renamePresentationFile(const QString &oldName, const QString &newName);
    void deletePresentationFile(const QString &filePath);
    void renameMaterial(const QString &oldName, const QString &newName);
    bool duplicatePresentation(const QString &oldPres, const QString &newPres);
    void loadVariants(const QString &filePath = {});
    void addVariantTag(const QString &group, const QString &newTag);
    void renameVariantTag(const QString &group, const QString &oldTag, const QString &newTag);
    void deleteVariantTag(const QString &group, const QString &tag);
    void addVariantGroup(const QString &newGroup);
    void renameVariantGroup(const QString &oldGroup, const QString &newGroup);
    void deleteVariantGroup(const QString &group);
    void changeVariantGroupColor(const QString &group, const QString &newColor);
    bool isVariantGroupUnique(const QString &group) const;
    bool isVariantTagUnique(const QString &group, const QString &tag) const;

    QHash<QString, VariantGroup> variantsDef() const { return m_variantsDef; }
    QStringList variantsDefKeys() const { return m_variantsDefKeys; }

Q_SIGNALS:
    void presentationIdChanged(const QString &path, const QString &id);
    void assetNameChanged();

private:
    enum class VariantsUpdateMode { Rename, Delete };

    QString ensureUniquePresentationId(const QString &id) const;
    bool tagExistsInUip(const QString &src, const QString &group, const QString &tag = {}) const;
    void updateVariantsInUip(const QString &src, const VariantsUpdateMode &updateType,
                             const QString &group, const QString &tag = {},
                             const QString &newName = {});

    QFileInfo m_fileInfo; // uia file info
    QString m_initialPresentation;
    QHash<QString, VariantGroup> m_variantsDef; // definition of variants
    QStringList m_variantsDefKeys; // maintains insertion order
    int m_variantColorsIter = -1; // tracks the next default color to assign to newly created group
};

#endif // PROJECTFILE_H
