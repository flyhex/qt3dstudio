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
void ProjectFile::ensureProjectFile(const QString &uipPath)
{
    QString uiaPath = PresentationFile::findProjectFile(uipPath);

    if (!uiaPath.isEmpty()) // uia found
        m_fileInfo.setFile(uiaPath);
    else
        throw ProjectFileNotFoundException();
}

/**
 * Add a presentation or presentation-qml node to the project file
 *
 * @param pPath the absolute path to the presentation file, it will be saved as relative
 * @param pId presentation Id
 */
void ProjectFile::addPresentationNode(const QString &pPath, const QString &pId)
{
    // open the uia file
    QFile file(getProjectFilePath());
    file.open(QIODevice::ReadWrite);
    QDomDocument doc;
    doc.setContent(&file);

    QDomElement rootElem = doc.documentElement();
    QDomElement assetsElem = rootElem.firstChildElement(QStringLiteral("assets"));

    // create the <assets> node if it doesn't exist
    if (assetsElem.isNull()) {
        assetsElem = doc.createElement(QStringLiteral("assets"));
        assetsElem.setAttribute(QStringLiteral("initial"), QFileInfo(pPath).completeBaseName());
        rootElem.insertBefore(assetsElem, {});
    }

    QString relativePresentationPath = QDir(getProjectPath()).relativeFilePath(pPath);

    // make sure the node doesn't already exist
    bool nodeExists = false;
    for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
        p = p.nextSibling().toElement()) {
        if ((p.nodeName() == QLatin1String("presentation")
             || p.nodeName() == QLatin1String("presentation-qml"))
                && p.attribute(QStringLiteral("src")) == relativePresentationPath) {
            nodeExists = true;
            break;
        }
    }

    if (!nodeExists) {
        QString presentationId = pId.isEmpty()
                ? ensureUniquePresentationId(QFileInfo(pPath).completeBaseName()) : pId;

        // add the presentation node
        QDomElement pElem = pPath.endsWith(QLatin1String(".qml"))
                              ? doc.createElement(QStringLiteral("presentation-qml"))
                              : doc.createElement(QStringLiteral("presentation"));
        pElem.setAttribute(QStringLiteral("id"), presentationId);
        pElem.setAttribute(QStringLiteral("src"), relativePresentationPath);
        assetsElem.appendChild(pElem);

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

    QFile file(getProjectFilePath());
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
            QString path = getProjectPath() + QStringLiteral("/")
                           + pElem.attribute(QStringLiteral("src"));
            PresentationFile::updatePresentationId(path, oldId, id);
        }
    }
}

// set the doc PresentationId from the project file, this is called after a document is loaded
void ProjectFile::updateDocPresentationId()
{
    QFile file(getProjectFilePath());
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    QDomElement rootElem = doc.documentElement();
    QDomElement assetsElem = rootElem.firstChildElement(QStringLiteral("assets"));

    if (!assetsElem.isNull()) {
        QString relativeDocPath = QDir(getProjectPath()).relativeFilePath(
                    g_StudioApp.GetCore()->GetDoc()->GetDocumentPath().GetPath().toQString());

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
    QFile file(getProjectFilePath());
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
void ProjectFile::create(const QString &projectName,
                         const Q3DStudio::CFilePath &projectPath)
{
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

    QString uiaPath = projectPath.toQString() + QStringLiteral("/") + projectName
                      + QStringLiteral(".uia");

    QFile file(uiaPath);
    file.open(QIODevice::WriteOnly);
    file.resize(0);
    file.write(doc.toByteArray(4));
    file.close();

    m_fileInfo.setFile(uiaPath);
}

void ProjectFile::loadSubpresentationsAndDatainputs(
                                                QVector<SubPresentationRecord> &subpresentations,
                                                QMap<QString, CDataInputDialogItem *> &datainputs)
{
    subpresentations.clear();
    datainputs.clear();

    QFile file(getProjectFilePath());
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
    QFile file(getProjectFilePath());
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    QDomElement assetsElem = doc.documentElement().firstChildElement(QStringLiteral("assets"));
    if (!assetsElem.isNull()) {
        QString relativePath = !src.isEmpty() ? src
                                              : g_StudioApp.GetCore()->GetDoc()->getRelativePath();
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
    QFile file(getProjectFilePath());
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

// Get the path to the project root
QString ProjectFile::getProjectPath() const
{
    return m_fileInfo.path();
}

// Get the path to the project's .uia file
QString ProjectFile::getProjectFilePath() const
{
    return m_fileInfo.filePath();
}

QString ProjectFile::getProjectName() const
{
    return m_fileInfo.completeBaseName();
}

/**
 * Get presentations out of a uia file
 *
 * @param inUiaPath uia file path
 * @param outSubpresentations list of collected presentations
 * @param excludePresentationSrc execluded presentation, (commonly the current presentation)
 */
// static
void ProjectFile::getPresentations(const QString &inUiaPath,
                                   QVector<SubPresentationRecord> &outSubpresentations,
                                   const QString &excludePresentationSrc)
{
    QFile file(inUiaPath);
    file.open(QFile::Text | QFile::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << file.errorString();
        return;
    }

    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    while (!reader.atEnd()) {
        if (reader.readNextStartElement()
            && (reader.name() == QLatin1String("presentation")
                || reader.name() == QLatin1String("presentation-qml"))) {
            const auto attrs = reader.attributes();
            QString argsOrSrc = attrs.value(QLatin1String("src")).toString();
            if (excludePresentationSrc == argsOrSrc)
                continue;
            if (argsOrSrc.isNull())
                argsOrSrc = attrs.value(QLatin1String("args")).toString();

            outSubpresentations.push_back(
                        SubPresentationRecord(reader.name().toString(),
                                              attrs.value(QLatin1String("id")).toString(),
                                              argsOrSrc));
        } else if (reader.name() == QLatin1String("assets") && !reader.isStartElement()) {
            break; // reached end of <assets>
        }
    }
}

QString ProjectFile::getResolvedPathTo(const QString &path) const
{
    auto projectPath = QDir(getProjectPath()).absoluteFilePath(path);
    return QDir::cleanPath(projectPath);
}
