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

#include "PresentationFile.h"
#include <QtCore/qfile.h>
#include <QtXml/qdom.h>
#include "Dialogs.h"
#include "qdebug.h"
#include "QtCore/qfileinfo.h"
#include <QtCore/qdiriterator.h>
#include <qxmlstream.h>

// this class opens and manipulates a presentation file (.uip). new uip manipulation should be
// implemented here. Old uip functionality should be gradually moved here as well whenever feasible.

PresentationFile::PresentationFile(const QString &path)
{
    m_file.setFileName(path);
    bool success = m_file.open(QIODevice::ReadWrite);
    Q_ASSERT(success); // invalid path

    m_domDoc.setContent(&m_file);
}

PresentationFile::~PresentationFile()
{
    m_file.resize(0);
    m_file.write(m_domDoc.toByteArray(4));
    m_file.close();
}

// static
QSize PresentationFile::readSize(const QString &url)
{
    QFile file(url);
    file.open(QFile::Text | QFile::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << file.errorString();
        return QSize();
    }
    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("ProjectSettings")) {
            const auto attrs = reader.attributes();
            return QSize(attrs.value(QLatin1String("presentationWidth")).toInt(),
                         attrs.value(QLatin1String("presentationHeight")).toInt());
        }
    }

    return QSize();
}

// get all available child assets source paths (materials, images, effects, etc)
void PresentationFile::getSourcePaths(const QDir &srcDir, QList<QString> &outPaths,
                                      QString &outRootPath) const
{
    // search <Classes>
    QDomElement classesElem = m_domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Classes"));
    for (QDomElement p = classesElem.firstChild().toElement(); !p.isNull();
        p = p.nextSibling().toElement()) {
        QString sourcepath = p.attribute(QStringLiteral("sourcepath"));
        if (!sourcepath.isEmpty() && !outPaths.contains(sourcepath)) {
            outPaths.push_back(p.attribute(QStringLiteral("sourcepath")));

            QFileInfo fi(sourcepath);
            QByteArray ext = fi.suffix().toLatin1();

            // if material or effect, find and add their dependents
            if (CDialogs::IsMaterialFileExtension(ext.data()) ||
                CDialogs::IsEffectFileExtension(ext.data())) {
                QFile f(srcDir.path() + QStringLiteral("/") + sourcepath);
                if (f.open(QIODevice::ReadOnly)) {
                    QDomDocument domDoc;
                    domDoc.setContent(&f);

                    QDomNodeList propElems = domDoc.documentElement()
                            .firstChildElement(QStringLiteral("MetaData"))
                            .childNodes();

                    QFile file;
                    for (int i = 0; i < propElems.count(); ++i) {
                        QString path = propElems.at(i).toElement()
                                .attribute(QStringLiteral("default"));
                        if (!path.isEmpty()) {
                            if (path.indexOf(QLatin1String("./")) == 0) {
                                // find the imported uip file project root
                                if (outRootPath.isEmpty()) {
                                    QDir currDir = srcDir;
                                    do {
                                        QDirIterator di(currDir.path(), QDir::NoDotAndDotDot
                                                                           | QDir::Files);
                                        while (di.hasNext()) {
                                            QFileInfo fi = di.next();
                                            if (fi.suffix() == QLatin1String("uia")) {
                                                outRootPath = fi.path();
                                                break;
                                            }
                                        }
                                    } while (outRootPath.isEmpty() && currDir.cdUp());
                                }

                                // outRootPath can only be empty if no project file was found
                                // in which case all root paths (starting with ./) wont be imported
                                file.setFileName(outRootPath + QStringLiteral("/") + path);
                            } else {
                                file.setFileName(srcDir.path() + QStringLiteral("/") + path);
                            }

                            if (file.exists() && !outPaths.contains(path))
                                outPaths.push_back(path);
                        }
                    }

                    f.close();
                }
            }
        }
    }

    // search <Logic> -> <State> -> <Add>
    QDomNodeList addElems = m_domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Logic"))
            .elementsByTagName(QStringLiteral("Add"));

    for (int i = 0; i < addElems.count(); ++i) {
        QString sourcePath = addElems.at(i).toElement().attribute(QStringLiteral("sourcepath"));
        if (!sourcePath.isEmpty()) {
            QFileInfo fi(sourcePath);
            QByteArray ext = fi.suffix().toLatin1();
            // add image and mesh paths
            if (CDialogs::IsImageFileExtension(ext.data())
                    || CDialogs::isMeshFileExtension(ext.data())) {
                if (!outPaths.contains(sourcePath))
                    outPaths.push_back(sourcePath);
                continue;
            }
        }

        // add fonts paths
        QString font = addElems.at(i).toElement().attribute(QStringLiteral("font"));
        if (!font.isEmpty()) {
            // the .uip file only shows the font name, we search for the font file in the current
            // directory plus the 'fonts' directory at the same level or 1 level up.

            const QString TTF_EXT = QStringLiteral(".ttf"); // TODO: should we also consider .otf?

            // this is the most probable place so lets search it first
            QString fontPath = QStringLiteral("../fonts/") + font + TTF_EXT;
            if (QFile::exists(srcDir.path() + QStringLiteral("/") + fontPath)) {
                if (!outPaths.contains(fontPath))
                    outPaths.push_back(fontPath);
                continue;
            }

            fontPath = font + TTF_EXT;
            if (QFile::exists(srcDir.path() + QStringLiteral("/") + fontPath)) {
                if (!outPaths.contains(fontPath))
                    outPaths.push_back(fontPath);
                continue;
            }

            fontPath = QStringLiteral("fonts/") + font + TTF_EXT;
            if (QFile::exists(srcDir.path() + QStringLiteral("/") + fontPath)) {
                if (!outPaths.contains(fontPath))
                    outPaths.push_back(fontPath);
                continue;
            }
        }
    }
}
