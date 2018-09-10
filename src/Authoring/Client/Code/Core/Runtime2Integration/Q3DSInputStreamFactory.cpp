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

#include "Q3DSInputStreamFactory.h"
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qfileinfo.h>

namespace Q3DStudio {

class Q3DSInputStreamFactory : public IInputStreamFactory
{
public:
    Q3DSInputStreamFactory()
    {
    }

    void addSearchDirectory(const QString &inDirectory) override
    {
        if (m_list.contains(inDirectory))
            return;
        m_list.append(inDirectory);
    }

    IRefCountedInputStream getStreamForFile(const QString &inFilename, bool inQuiet) override
    {
        QString path = QStringLiteral("./");
        QFileInfo info(inFilename);
        IRefCountedInputStream stream;
        int idx = 0;
        QScopedPointer<QFile> file(new QFile(inFilename));
        if (info.isRelative()) {
            while (stream.isNull()) {
                file->setFileName(path + inFilename);
                if (file->exists()) {
                    if (file->open(QFile::ReadOnly)) {
                        stream.reset(file.take());
                        break;
                    }
                }
                path = m_list[idx++];
                if (!path.endsWith(QStringLiteral("/")))
                    path.append(QStringLiteral("/"));
            }
        } else {
            file->setFileName(inFilename);
            if (file->exists()) {
                if (file->open(QFile::ReadOnly))
                    stream.reset(file.take());
            }
        }
        return stream;
    }

    bool getPathForFile(const char *inFilename, QString &outFile, bool inQuiet) override
    {
        return false;
    }
private:
    QStringList m_list;
};

QSharedPointer<Q3DStudio::IInputStreamFactory> &IInputStreamFactory::Create()
{
    static QSharedPointer<Q3DStudio::IInputStreamFactory> s_factory;
    if (s_factory.isNull())
        s_factory.reset(new Q3DSInputStreamFactory());
    return s_factory;
}

}
