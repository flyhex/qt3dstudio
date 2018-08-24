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

#ifndef Q3DS_INPUTSTREAMFACTORY_H
#define Q3DS_INPUTSTREAMFACTORY_H

#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include <QtCore/qfile.h>
#include <QtCore/qshareddata.h>

namespace Q3DStudio {

/* This class replaces IInputStreamFactory

    class IRefCountedInputStream : public qt3ds::foundation::ISeekableIOStream,
                                   public qt3ds::foundation::NVRefCounted
    {
    protected:
        virtual ~IRefCountedInputStream() {}
    };

    // This class is threadsafe.
    class IInputStreamFactory : public qt3ds::foundation::NVRefCounted
    {
    protected:
        virtual ~IInputStreamFactory() {}
    public:
        // These directories must have a '/' on them
        virtual void AddSearchDirectory(const char8_t *inDirectory) = 0;
        virtual IRefCountedInputStream *GetStreamForFile(const char8_t *inFilename,
                                                         bool inQuiet = false) = 0;
        // Return a path for this file.  Returns true if GetStreamForFile would return a valid
        // stream.
        // else returns false
        virtual bool GetPathForFile(const char8_t *inFilename, eastl::string &outFile,
                                    bool inQuiet = false) = 0;

        // Create an input stream factory using this foundation and an platform-optional app
        // directory
        // on android the app directory has no effect; use use the assets bundled with the APK file.
        QT3DS_STATE_API static IInputStreamFactory &Create(qt3ds::NVFoundationBase &inFoundation);
    };
*/

typedef QSharedPointer<QFile> IRefCountedInputStream;

// TODO: This class is threadsafe.
class IInputStreamFactory
{
public:
    virtual ~IInputStreamFactory() {}
public:
    // These directories must have a '/' on them
    virtual void addSearchDirectory(const char *inDirectory) = 0;
    virtual IRefCountedInputStream *getStreamForFile(const QString inFilename,
                                                     bool inQuiet = false) = 0;
    // Return a path for this file.  Returns true if getStreamForFile would
    // return a valid stream, else returns false.
    virtual bool getPathForFile(const char *inFilename, QString &outFile,
                                bool inQuiet = false) = 0;

    // Create an input stream factory
    static QSharedPointer<Q3DStudio::IInputStreamFactory> &Create();
};

}
#endif
