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

#include "ProjectFile.h"
#include "Qt3DSFileTools.h"
#include "Exceptions.h"
#include "DataInputDlg.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "PresentationFile.h"
#include <QtCore/qdiriterator.h>
#include <QtXml/qdom.h>

ProjectFile::ProjectFile()
{

}

// find the 1st .uia file in the current or parent directories and assume this is the project file,
// as a project should have only 1 .uia file
void ProjectFile::ensureProjectFile(const QDir &uipDirectory)
{
    QDir currentDir = uipDirectory;
    bool uiaFound = false;
    do {
        QDirIterator di(currentDir.path(), QDir::NoDotAndDotDot | QDir::Files);
        while (di.hasNext()) {
            Q3DStudio::CFilePath file = di.next();

            if (file.GetExtension() == "uia") {
                // found the project file, update project name and directory
                setProjectNameAndPath(file.GetFileStem(), file.GetDirectory());
                uiaFound = true;
                break;
            }
        }
    } while (!uiaFound && currentDir.cdUp());

    if (!uiaFound)
        throw ProjectFileNotFoundException();
}

/**
 * Add a presentation node to the project file
 *
 * @param uip the absolute path of the presentation file, it will be saved as relative
 */
void ProjectFile::addPresentationNode(const Q3DStudio::CFilePath &uip)
{
    // open the uia file
    QString path = m_projectPath.toQString() + QStringLiteral("/") + m_projectName.toQString()
            + QStringLiteral(".uia");
    QFile file(path);
    file.open(QIODevice::ReadWrite);
    QDomDocument doc;
    doc.setContent(&file);

    QDomElement rootElem = doc.documentElement();
    QDomElement assetsElem = rootElem.firstChildElement(QStringLiteral("assets"));

    // create the <assets> node if it doesn't exist
    if (assetsElem.isNull()) {
        assetsElem = doc.createElement(QStringLiteral("assets"));
        assetsElem.setAttribute(QStringLiteral("initial"), uip.GetFileStem().toQString());
        rootElem.insertBefore(assetsElem, {});
    }

    QString relativeUipPath = uip.absoluteFilePath()
            .remove(0, m_projectPath.toQString().length() + 1);

    // make sure the node doesn't already exist
    bool nodeExists = false;
    for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
        p = p.nextSibling().toElement()) {
        if ((p.nodeName() == QLatin1String("presentation")
             || p.nodeName() == QLatin1String("presentation-qml"))
                && p.attribute(QStringLiteral("src")) == relativeUipPath) {
            nodeExists = true;
            break;
        }
    }

    if (!nodeExists) {
        QString defaultPresentationId = ensureUniquePresentationId(uip.GetFileStem().toQString());

        // add the presentation tag
        QDomElement uipElem = doc.createElement(QStringLiteral("presentation"));
        uipElem.setAttribute(QStringLiteral("id"), defaultPresentationId);
        uipElem.setAttribute(QStringLiteral("src"), relativeUipPath);
        assetsElem.appendChild(uipElem);

        file.resize(0);
        file.write(doc.toByteArray(4));
    }

    file.close();
}

// get the path (relative) to the first presentation in a uia file
QString ProjectFile::getFirstPresentationPath(const QString &uiaPath) const
{
    QFile file(uiaPath);
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    QDomElement assetsElem = doc.documentElement().firstChildElement(QStringLiteral("assets"));
    if (!assetsElem.isNull()) {
        QDomElement firstPresentationElem =
                assetsElem.firstChildElement(QStringLiteral("presentation"));

        if (!firstPresentationElem.isNull())
            return firstPresentationElem.attribute(QStringLiteral("src"));
    }

    return {};
}

/**
 * Write a presentation id to the project file.
 *
 * This also update the Doc presentation Id if the src param is empty
 *
 * @param id presentation Id
 * @param src source node, if empty the current document node is used
 */
void ProjectFile::writePresentationId(const QString &id, const QString &src)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();

    if (src.isEmpty() || src == doc->getRelativePath())
        doc->setPresentationId(id);

    QString path = m_projectPath.toQString() + QStringLiteral("/") + m_projectName.toQString()
            + QStringLiteral(".uia");
    QFile file(path);
    file.open(QIODevice::ReadWrite);
    QDomDocument domDoc;
    domDoc.setContent(&file);

    QDomElement rootElem = domDoc.documentElement();
    QDomNodeList pNodes = rootElem.elementsByTagName(QStringLiteral("presentation"));
    QString oldId;
    if (!pNodes.isEmpty()) {
        QString relativePath = !src.isEmpty() ? src : doc->getRelativePath();
        for (int i = 0; i < pNodes.length(); ++i) {
            QDomElement pElem = pNodes.at(i).toElement();
            if (pElem.attribute(QStringLiteral("src")) == relativePath) {
                oldId = pElem.attribute(QStringLiteral("id"));
                pElem.setAttribute(QStringLiteral("id"), !id.isEmpty() ? id
                                                                       : doc->getPresentationId());
                break;
            }
        }
    }

    // overwrite the uia file
    file.resize(0);
    file.write(domDoc.toByteArray(4));
    file.close();

    // update changed presentation Id in all .uip files if in-use
    if (!oldId.isEmpty()) {
        for (int i = 0; i < pNodes.length(); ++i) {
            QDomElement pElem = pNodes.at(i).toElement();
            QString path = m_projectPath.toQString() + QStringLiteral("/")
                           + pElem.attribute(QStringLiteral("src"));
            PresentationFile::updatePresentationId(path, oldId, id);
        }
    }
}

// set the doc PresentationId from the project file, this is called after a document is loaded
void ProjectFile::updateDocPresentationId()
{
    QString path = m_projectPath.toQString() + QStringLiteral("/") + m_projectName.toQString()
            + QStringLiteral(".uia");
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    QDomElement rootElem = doc.documentElement();
    QDomElement assetsElem = rootElem.firstChildElement(QStringLiteral("assets"));

    if (!assetsElem.isNull()) {
        QString relativeDocPath = g_StudioApp.GetCore()->GetDoc()->GetDocumentPath().GetPath()
                .toQString().remove(0, m_projectPath.toQString().length() + 1)
                .replace(QStringLiteral("\\"), QStringLiteral("/"));

        for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
            p = p.nextSibling().toElement()) {
            if ((p.nodeName() == QLatin1String("presentation")
                 || p.nodeName() == QLatin1String("presentation-qml"))
                    && p.attribute(QStringLiteral("src")) == relativeDocPath) {
                // current presentation node
                g_StudioApp.GetCore()->GetDoc()->setPresentationId(
                            p.attribute(QStringLiteral("id")));
                return;
            }
        }
    }
}

// get a presentationId from the project file, that match a given src attribute
QString ProjectFile::getPresentationId(const QString &src) const
{
    QString path = m_projectPath.toQString() + QStringLiteral("/") + m_projectName.toQString()
            + QStringLiteral(".uia");

    QFile file(path);
    file.open(QFile::Text | QFile::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << file.errorString();
        return {};
    }
    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    while (!reader.atEnd()) {
        if (reader.readNextStartElement() && reader.name() == QLatin1String("presentation")) {
            const auto attrs = reader.attributes();
            if (attrs.value(QLatin1String("src")) == src)
                return attrs.value(QLatin1String("id")).toString();
        }
    }

    return {};
}

// create the project .uia file
void ProjectFile::create(const Q3DStudio::CString &projectName,
                         const Q3DStudio::CFilePath &projectPath)
{
    setProjectNameAndPath(projectName, projectPath);

    QDomDocument doc;
    doc.setContent(QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                                  "<application xmlns=\"http://qt.io/qt3dstudio/uia\">"
                                    "<statemachine ref=\"#logic\">"
                                      "<visual-states>"
                                        "<state ref=\"Initial\">"
                                          "<enter>"
                                            "<goto-slide element=\"main:Scene\" rel=\"next\"/>"
                                          "</enter>"
                                        "</state>"
                                      "</visual-states>"
                                    "</statemachine>"
                                  "</application>"));

    QString path = m_projectPath.toQString() + QStringLiteral("/") + m_projectName.toQString()
            + QStringLiteral(".uia");
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.resize(0);
    file.write(doc.toByteArray(4));
    file.close();
}

void ProjectFile::loadSubpresentationsAndDatainputs(
                                                QVector<SubPresentationRecord> &subpresentations,
                                                QMap<QString, CDataInputDialogItem *> &datainputs)
{
    subpresentations.clear();
    datainputs.clear();

    QString path = m_projectPath.toQString() + QStringLiteral("/") + m_projectName.toQString()
                   + QStringLiteral(".uia");
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    QDomElement assetsElem = doc.documentElement().firstChildElement(QStringLiteral("assets"));
    if (!assetsElem.isNull()) {
        for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
            p = p.nextSibling().toElement()) {
            if ((p.nodeName() == QLatin1String("presentation")
                 || p.nodeName() == QLatin1String("presentation-qml"))
                    && p.attribute(QStringLiteral("id"))
                       != g_StudioApp.GetCore()->GetDoc()->getPresentationId()) {
                QString argsOrSrc = p.attribute(QStringLiteral("src"));
                if (argsOrSrc.isNull())
                    argsOrSrc = p.attribute(QStringLiteral("args"));

                subpresentations.push_back(
                            SubPresentationRecord(p.nodeName(), p.attribute("id"), argsOrSrc));
            } else if (p.nodeName() == QLatin1String("dataInput")) {
                CDataInputDialogItem *item = new CDataInputDialogItem();
                item->name = p.attribute(QStringLiteral("name"));
                QString type = p.attribute(QStringLiteral("type"));
                if (type == QLatin1String("Ranged Number")) {
                    item->type = EDataType::DataTypeRangedNumber;
                    item->minValue = p.attribute(QStringLiteral("min")).toFloat();
                    item->maxValue = p.attribute(QStringLiteral("max")).toFloat();
                } else if (type == QLatin1String("String")) {
                    item->type = EDataType::DataTypeString;
                } else if (type == QLatin1String("Float")) {
                    item->type = EDataType::DataTypeFloat;
                } else if (type == QLatin1String("Boolean")) {
                    item->type = EDataType::DataTypeBoolean;
                } else if (type == QLatin1String("Vector3")) {
                    item->type = EDataType::DataTypeVector3;
                } else if (type == QLatin1String("Vector2")) {
                    item->type = EDataType::DataTypeVector2;
                } else if (type == QLatin1String("Variant")) {
                    item->type = EDataType::DataTypeVariant;
                }
#ifdef DATAINPUT_EVALUATOR_ENABLED
                else if (type == QLatin1String("Evaluator")) {
                    item->type = EDataType::DataTypeEvaluator;
                    item->valueString = p.attribute(QStringLiteral("evaluator"));
                }
#endif
                datainputs.insert(item->name, item);
            }
        }
    }
}

/**
 * Write a presentation id to the project file.
 *
 * Check that a given presentation is unique
 *
 * @param id presentation Id
 * @param src source node to exclude from the check, if empty the current document node is used
 */
bool ProjectFile::isUniquePresentationId(const QString &id, const QString &src) const
{
    QString path = m_projectPath.toQString() + QStringLiteral("/") + m_projectName.toQString()
            + QStringLiteral(".uia");
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    QDomElement assetsElem = doc.documentElement().firstChildElement(QStringLiteral("assets"));
    if (!assetsElem.isNull()) {
        QString relativePath = !src.isEmpty() ? src
                               : g_StudioApp.GetCore()->GetDoc()->GetDocumentPath()
                                 .GetPath().toQString()
                                 .remove(0, m_projectPath.toQString().length() + 1)
                                 .replace(QStringLiteral("\\"), QStringLiteral("/"));
        for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
            p = p.nextSibling().toElement()) {
            if ((p.nodeName() == QLatin1String("presentation")
                 || p.nodeName() == QLatin1String("presentation-qml"))
                    && p.attribute(QStringLiteral("id")) == id
                    && p.attribute(QStringLiteral("src")) != relativePath) {
                return false;
            }
        }
    }

    return true;
}

QString ProjectFile::ensureUniquePresentationId(const QString &id) const
{
    QString path = m_projectPath.toQString() + QStringLiteral("/") + m_projectName.toQString()
            + QStringLiteral(".uia");
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();
    QString newId = id;
    QDomElement assetsElem = doc.documentElement().firstChildElement(QStringLiteral("assets"));
    if (!assetsElem.isNull()) {
        bool unique;
        int n = 1;
        do {
            unique = true;
            for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
                p = p.nextSibling().toElement()) {
                if ((p.nodeName() == QLatin1String("presentation")
                     || p.nodeName() == QLatin1String("presentation-qml"))
                        && p.attribute(QStringLiteral("id")) == newId) {
                    newId = id + QString::number(n++);
                    unique = false;
                    break;
                }
            }
        } while (!unique);
    }

    return newId;
}

void ProjectFile::setProjectNameAndPath(const Q3DStudio::CString &projectName,
                                        const Q3DStudio::CFilePath &projectPath)
{
    m_projectName = projectName;
    m_projectPath = projectPath;
}

Q3DStudio::CFilePath ProjectFile::getProjectPath() const
{
    return m_projectPath;
}

Q3DStudio::CString ProjectFile::getProjectName() const
{
    return m_projectName;
}
