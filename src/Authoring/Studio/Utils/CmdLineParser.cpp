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

//==============================================================================
//	Prefix
//==============================================================================

#include "stdafx.h"

//=============================================================================
//	Includes
//==============================================================================

#include "CmdLineParser.h"
#include "StudioObjectTypes.h"
#include "Qt3DSFile.h"

// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

//=============================================================================
/**
 * Create a new Command Line Parser object.
 */
CCmdLineParser::CCmdLineParser()
    : m_Silent(false)
    , m_RunUnitTests(false)
{
}

CCmdLineParser::~CCmdLineParser()
{
}

//=============================================================================
/**
 * If the execution mode is to open a file or execute XML tests strings then
 * this will return the filename used for them.
 * @return filename for file open or XML tests
 */
Q3DStudio::CString CCmdLineParser::GetFilename() const
{
    return m_Filename;
}

//=============================================================================
/**
 * Call to parse the specified parameter from the command line format into
 * a format useful by the app.
 * @param inParameter the name of the parameter to be parsed.
 * @param inIsFlag true if the param is a flag, false if it is an argument.
 */
void CCmdLineParser::ParseParam(const Q3DStudio::CString &inParameter, bool inIsFlag)
{
    if (inIsFlag) {
        if (inParameter == "t" || inParameter == "test") {
            m_ExecutionQueue.push_back(TEST_CMD_LINE);
            m_RunUnitTests = true;
        } else if (inParameter == "silent") {
            m_Silent = true;
        }
    } else {
        m_Params.push_back(inParameter);
    }
}

//=============================================================================
/**
 * Call this to parse all the command line arguments into useful info.
 * This is most often called with the args from main( ).
 * @param inArgc the number of argments being given.
 * @param inArgv the arguments.
 */
void CCmdLineParser::ParseArguments(int inArgc, wchar_t **inArgv)
{
    m_Filename.Clear();
    m_Params.clear();

    for (int i = 0; i < inArgc; ++i) {
        bool isFlag = false;
        Q3DStudio::CString theArg(inArgv[i]);
        if (theArg[(long)0] == '-') {
            isFlag = true;
            // take off the dash
            theArg = theArg.Extract(1);
        }
        ParseParam(theArg, isFlag);
    }

    // m_Params[1] is m_Filename
    // m_Params[0] is the path to the executable
    if (m_Params.size() > 1)
        ConvertToLongFilename(m_Params[1], m_Filename);

    // Post-processing
    // If there were no switches modifying m_ExecutionMode, default execution mode
    // according to the file extension found on m_Filename.
    if (0 == m_ExecutionQueue.size()) {
        Qt3DSFile theFile(m_Filename);
        QString theExtension = "." + theFile.GetExtension().toQString();
        if (!QString::compare(theExtension, QStringLiteral(".uip"), Qt::CaseInsensitive))
            m_ExecutionQueue.push_back(OPEN_FILE);
    }
}

//=============================================================================
/**
 * Get the next main mode of execution that the application should be in.
 * @return EExecutionMode execution mode; END_OF_CMDS if end of queue.
 */
CCmdLineParser::EExecutionMode CCmdLineParser::PopExecutionMode()
{
    EExecutionMode theMode = END_OF_CMDS;
    if (m_ExecutionQueue.size() > 0) {
        theMode = m_ExecutionQueue.front();
        m_ExecutionQueue.pop_front();
    }
    return theMode;
}

//=============================================================================
/**
 * Return true if the silent flag is specified.
 * @return true if running in muted mode. ie. no dialogs.
 */
bool CCmdLineParser::IsSilent() const
{
    return m_Silent;
}

//=============================================================================
/**
 * Return true if running unit tests.
 * @return true if running unit tests.
 */
bool CCmdLineParser::IsRunUnitTests() const
{
    return m_RunUnitTests;
}

//=============================================================================
/**
 * Converts the specified path to its long form. If no long path is found,
 * this function simply returns the specified name.
 * @param inSource			Source filename
 * @param outLongFilename	Output filename
 */
void CCmdLineParser::ConvertToLongFilename(const Q3DStudio::CString &inSource,
                                           Q3DStudio::CString &outLongFilename) const
{
#ifdef _WIN32
    TCHAR thePathBuffer[_MAX_PATH * 2 + 1] = {
        0
    }; // make sure this char buffer is big enough for 2-byte chars
    long theRet = GetLongPathName(inSource, thePathBuffer, _MAX_PATH * 2);
    if (theRet != 0 && theRet <= _MAX_PATH * 2)
        outLongFilename = Q3DStudio::CString(thePathBuffer);
    else
        outLongFilename = inSource; // No conversion
#else
    outLongFilename = inSource;
#endif
}
