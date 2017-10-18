/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

#ifndef __CommonConstants_H__
#define __CommonConstants_H__

// The current version of the player
const float CURRENT_PLAYER_VERSION = 1.0f;

// Current User Registry Keys
const char QT3DS_CLIENT_PREFERENCES_VALUE[] = "Preferences";
const char QT3DS_CLIENT_REGISTRY_KEY_BASE[] = "Software\\Qt3DStudio\\Client";
const char QT3DS_CLIENT_REGISTRY_KEY_ROOT[] = "Software\\Qt3DStudio\\Client\\1.0";
const char QT3DS_CLIENT_REGISTRY_KEY[] = "Software\\Qt3DStudio\\Client\\1.0\\Preferences";
const char QT3DS_CLIENT_MESSAGES_KEY[] = "Software\\Qt3DStudio\\Client\\1.0\\Preferences\\Messages";

// Local Machine Registry Keys
const char QT3DS_CLIENT_PLUGINS_VALUE[] = "Plugins";
const char QT3DS_CLIENT_EXTENSIONS_VALUE[] = "Extensions";
const char QT3DS_CLIENT_LATESTVERSION_VALUE[] = "LatestVersion";
const char QT3DS_CLIENT_CURRENTVERSION_VALUE[] = "CurrentVersion";

const char QT3DS_CLIENT_PLUGINS_KEY[] = "Software\\Qt3DStudio\\Client\\1.0\\Plugins";
const char QT3DS_CLIENT_EXTENSIONS_KEY[] = "Software\\Qt3DStudio\\Client\\1.0\\Extensions";
const char QT3DS_CLIENT_LATESTVERSION_KEY[] = "Software\\Qt3DStudio\\Client\\1.0\\LatestVersion";
const char QT3DS_CLIENT_CURRENTVERSION_KEY[] = "Software\\Qt3DStudio\\Client\\1.0\\CurrentVersion";

// Preference Registry Keys
const char QT3DS_CLIENT_INITIALIZED_VALUE[] = "Initialized";
const char QT3DS_CLIENT_AUTOMATICUPDATE_VALUE[] = "AutomaticallyUpdate";
const char QT3DS_CLIENT_FORCENOINSTALL_VALUE[] = "ForceNoInstall";
const char QT3DS_CLIENT_FORCEINSTALL_VALUE[] = "ForceInstall";
const char QT3DS_CLIENT_NOREGISTER_VALUE[] = "NoRegister";
const char QT3DS_CLIENT_LOGMODE_VALUE[] = "LogMode"; // REG_DWORD, 0, 1, 2, 3
const char QT3DS_CLIENT_LOGLEVEL_VALUE[] = "LogLevel"; // REG_DWORD, 0, 1, 2, 3
const char QT3DS_CLIENT_LOGTHRESHOLD_VALUE[] = "LogThreshold"; // REG_DWORD, # of entries
const char QT3DS_CLIENT_CONTEXTMENU_VALUE[] = "ContextMenu";

const char QT3DS_CLIENT_TRACKERRORTYPES_VALUE[] = "TrackErrorTypes";
const char QT3DS_CLIENT_REPORTINGLEVEL_VALUE[] = "ReportingLevel";
const char QT3DS_CLIENT_ERRORACTION_VALUE[] = "ErrorAction";
const char QT3DS_CLIENT_FILENAMELINENO_VALUE[] = "FileNameLineNo";

const char QT3DS_CLIENT_ENABLESCRIPTDEBUGGER_VALUE[] = "EnableScriptDebugger";
const char QT3DS_CLIENT_DEBUGMODE_VALUE[] = "DebugMode";
const char QT3DS_CLIENT_MINTIMEPERFRAME_VALUE[] = "MinTimePerFrame";
const char QT3DS_CLIENT_UPDATEURL_VALUE[] = "DebugUpdateUrl";
const char QT3DS_CLIENT_LASTUPDATECHECK_VALUE[] = "LastUpdateCheck";
const char QT3DS_CLIENT_OVERRIDELASTUPDATE_VALUE[] = "LastUpdateCheckOverride";
const char QT3DS_CLIENT_UPDATEINTERVAL_VALUE[] = "UpdateInterval";
const char QT3DS_CLIENT_DEPRECATEDPRESENTATION_VALUE[] = "DeprecatedPresentationWarning";
const char QT3DS_CLIENT_OPENGLDRIVER_VALUE[] = "OpenGLDriverWarning";
const char QT3DS_CLIENT_GRAPHICSRENDERER_VALUE[] = "ClientRenderer";
const char QT3DS_CLIENT_LICENSEKEY_VALUE[] = "LicenseKey";

/*
const char QT3DS_CLIENT_AGREEEULA_VALUE[] = "AgreeEULA";
const char QT3DS_CLIENT_LASTNAME_VALUE[] = "LastName";
const char QT3DS_CLIENT_FIRSTNAME_VALUE[] = "FirstName";
const char QT3DS_CLIENT_USEREMAIL_VALUE[] = "UserEmail";
const char QT3DS_CLIENT_OVER13_VALUE[] = "UserOver13";
const char QT3DS_CLIENT_MAILINGLIST_VALUE[] = "MailingList";
*/

// Uninstall Registry Keys
const char UNINSTALL_REGISTRY_KEY[] =
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Qt3DStudioClient10";

// Uninstall Registry Value Names
const char UNINSTALL_DISPLAYNAME_VALUE[] = "DisplayName";
const char UNINSTALL_COMMANDLINE_VALUE[] = "UninstallString";

// File Names
const char INSTALL_LOG_FILE_NAME[] = "Install.log";
const char UNINSTALL_DATA_FILE_NAME[] = "Uninstal.dat";
const char QT3DS_AUTO_UPDATER_EXE[] = "AMInstal.exe";
const char QT3DS_CONTROL_PANEL[] = "AKCPanel.cpl";

// Directory Names
const char QT3DS_DIRECTORY[] = "\\The Qt Company\\";
const char QT3DS_CLIENT_DIRECTORY[] = "Qt 3D Studio\\";
const char QT3DS_TEMP_DIRECTORY[] = "Install\\";

// Installer command line options
const char REMOTE_COMMAND_LINE_OPTION[] = "remote";
const char UNINSTALL_COMMAND_LINE_OPTION[] = "uninstall";
const char SILENT_COMMAND_LINE_OPTION[] = "silent";
const char INSTALLFROM_COMMAND_LINE_OPTION[] = "installfrom";

// Uninstall file section names
const char VERSION_SECTION_NAME[] = "Version";
const char FILES_SECTION_NAME[] = "Files";
const char DIR_SECTION_NAME[] = "Directories";
const char REGKEY_SECTION_NAME[] = "RegKeys";
const char REGVALUE_SECTION_NAME[] = "RegValues";

// Uninstall file values
const char FILEVERSION_VALUE[] = "FileVersion";
const char REGISTRYROOT_HKLM[] = "HKLM";
const char REGISTRYROOT_HKCU[] = "HKCU";

// Misc
const long CLIENT_UPDATE_INTERVAL = 7; ///< 7 days

// OpenGL
const char OPENGL_REQUIRED_EXTENSIONS[] =
    "GL_EXT_bgra GL_WIN_swap_hint GL_ARB_multitexture GL_EXT_texture_env_combine";

// Unwelcome Video Vendor
const char UNSUPPORT_VIDEO_VENDOR[] = "Microsoft Corporation";

// Action mask ids
const unsigned long ACTION_LOG_BITMASK = 0x00000001; ///< Write to log file
const unsigned long ACTION_DISPLAYMESSAGEBOX_BITMASK = 0x00000002; ///< Display a message box
const unsigned long ACTION_DEBUGGEROUTPUT_BITMASK = 0x00000004; ///< Output to debugger window
const unsigned long ACTION_DEBUGBREAK_BITMASK = 0x00000008; ///< Break at the spot

// 3D constants
const float UIC3D_DEGREES_TO_RADIANS = (float)0.0174532925199;
const float UIC3D_RADIANS_TO_DEGREES = (float)57.2957795131;

// Fullscreen anti-aliasing modes
const long FSAA_OFF = 0x00000000; ///< No fullscreen anti-aliasing
const long FSAA_1X = 0x00000001; ///< 1x sample
const long FSAA_2X = 0x00000002; ///< 2x sample
const long FSAA_4X = 0x00000004; ///< 4x sample
const long FSAA_6X = 0x00000006; ///< 6x sample
const long FSAA_8X = 0x00000008; ///< 8x sample

#endif // __CommonConstants_H__
