/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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

#ifndef INCLUDED_CMD_LINE_PARSER_H
#define INCLUDED_CMD_LINE_PARSER_H 1

#include <deque>

#include <UICString.h>

class CCmdLineParser
{
public:
    enum EExecutionMode { NORMAL, OPEN_FILE, TEST_CMD_LINE, END_OF_CMDS };

    CCmdLineParser();
    virtual ~CCmdLineParser();

    void ParseArguments(int inArgc, wchar_t **);

    EExecutionMode PopExecutionMode();
    bool IsSilent() const;
    bool IsRunUnitTests() const;

    Q3DStudio::CString GetFilename() const;

protected:
    virtual void ParseParam(const Q3DStudio::CString &inParameter, bool inIsFlag);
    void ConvertToLongFilename(const Q3DStudio::CString &inSource,
                               Q3DStudio::CString &outLongFilename) const;

    Q3DStudio::CString m_Filename; ///< input filename for file open
    bool m_Silent; ///< true if non-dialog alternative output
    std::vector<Q3DStudio::CString> m_Params; ///< filenames or optional parameters
    std::deque<EExecutionMode> m_ExecutionQueue; ///< queue holding execution mode commands
    bool m_RunUnitTests; ///< true if running unit tests
};
#endif // INCLUDED_CMD_LINE_PARSER_H
