'''
/****************************************************************************
**
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
'''
'''Compile the string input (stri) into string output (stro) + Strings.h'''
import os, os.path, re
import xml.sax.handler, xml.sax, codecs


class CGenericXMLFactory:
        '''Generic factory for xml generation'''
        def __init__( self ):
                self.m_Elements = {}

        def AddTagElement( self, inName, inFunction ):
                self.m_Elements[inName] = inFunction

        def CreateObject( self, inName, inAttrs, inParent ):
                theElement = self.m_Elements[inName](inName, inAttrs, inParent )
                return theElement


class CBaseXMLHandler( xml.sax.handler.ContentHandler ):
        """Base xml parsing class. Can be used to form a dom system"""
        def __init__( self, inFactory ):
                self.m_Objects = [] #stack of objects used for parsing (targets require the latest project)
                self.m_Factory = inFactory
                self.m_CurrentObject = None
                self.m_CharacterHandler = None
                self.m_TopObject = None

        def startElement( self, name, attrs ):
                theParent = None
                if( not len(self.m_Objects) == 0 ):
                        theParent = self.m_Objects[-1]
                self.m_CurrentObject = self.m_Factory.CreateObject( name, attrs, theParent )
                try:
                        self.m_CurrentObject.startElement( self )
                except AttributeError:
                        pass #Don't care if the object does not support the method call
                self.OnObjectCreated( self.m_CurrentObject )

        def endElement( self, name ):
                theElement = self.m_Objects.pop()
                try:
                        theElement.endElement( self )
                except AttributeError:
                        pass #Don't care if the object does not support the method call

                if( len( self.m_Objects ) != 0 ):
                        self.m_CurrentObject = self.m_Objects[-1]

        def characters( self, content ):
                if( self.m_CharacterHandler ):
                        self.m_CharacterHandler( self.m_CurrentObject, content )

        def SetCharacterHandler( self, inHandler ):
                self.m_CharacterHandler = inHandler

        def OnObjectCreated( self, theObject ):
                '''Callback every time a factory returns an object'''
                self.m_Objects.append( theObject )
                if( len( self.m_Objects ) == 1 ):
                        self.m_TopObject = self.m_CurrentObject

        def SetFactory( self, inFactory ):
                self.m_Factory = inFactory

        def Parse( self, inFilename ):
                '''Parse the file, grabbing the extension'''
                theDirectory = os.path.dirname( inFilename )
                theFile = os.path.basename( inFilename )
                self.m_BasePath = theDirectory
                theReturn = None
                theExcept = None
                self.m_TopObject = None
                self.m_CurrentObject = None
                self.m_Objects = []
                theParse = xml.sax.parse( inFilename, self )
                return self.m_TopObject


class CStringObject:
        '''Basic string object'''
        def __init__( self ):
                self.m_Name = ''
                self.m_Value = ''

        def SetName( self, inName ):
                self.m_Name = inName

        def GetName( self ):
                return self.m_Name

        def SetValue( self, inValue ):
                self.m_Value = inValue

        def GetValue( self ):
                return self.m_Value

def XMLStringObjectInit( inName, inAttrs, inParent ):
        '''Create a string object from xml'''
        theObject = CStringObject( )
        try:
                theObject.SetName( inAttrs['name'] )
                theObject.SetValue( inAttrs['value'] )
        except KeyError:
                print("Failed to parse string object")

        inParent.AddObject( theObject )
        return theObject


class CStringObjectList:
        '''Create a list of string objects'''
        def __init__( self ):
                self.m_StringObjects = []

        def AddObject( self, inStringObject ):
                self.m_StringObjects.append( inStringObject )

        def GetObjects( self ):
                return self.m_StringObjects

def XMLStringObjectListInit( inName, inAttrs, inParent ):
        '''Create a list from xml'''
        theStringList = CStringObjectList()
        return theStringList

def ExportObjects( inFactory ):
        inFactory.AddTagElement( 'stringresourceinput', XMLStringObjectListInit )
        inFactory.AddTagElement( 'string', XMLStringObjectInit )




###Main Code Below###########################################
theLists = []
#parse into a named group the matches anything up to the period and an ending of stri
theSourceRegex = re.compile( r'(?P<stem>.*)\.stri$' ) # $ means end of line/input
theTotalFiles = os.listdir( '.' ) #List files in the current working directory


#Create xml parsing context
theFactory =  CGenericXMLFactory()
theHandler = CBaseXMLHandler( theFactory )
ExportObjects( theFactory ) #Export the dom objects we would like to use

theStringCount = 0

for theFile in theTotalFiles:
        theMatch = theSourceRegex.match( theFile )
        if( theMatch ):
                theList = None
                try:
                        print( 'parsing %s' % ( theFile ) )
                        theList = theHandler.Parse( theFile )
                        theLists.append( ( theMatch.group( 'stem' ), theList ) ) #append a touple of the source name
                        theStringCount += len( theList.GetObjects() )
                except Exception as e:
                        print( 'Failed to parse %s: %s' % ( theFile, e ) )
                        #salvage anything possible
                        theList = theHandler.m_TopObject
                        if( theList ):
                                theLists.append( ( theMatch.group( 'stem' ), theList ) ) #append a touple of the source name
                                theStringCount += len( theList.GetObjects() )

theIndex = 1
theHeaderName = 'Strings.h'
theHeaderFile = open( 'Strings.h', 'w' )

theHeaderLine = r'''
/****************************************************************************
**
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
//===============================================================================================================================
//	CODEGEN -- FILE CREATED BY STUDIO CODE GENERATION SYSTEM
//
//	Do not modify the contents of this file.  Your changes will be destroyed by
//	code generation.
//
//	Please see the CodeGen folder for more information or to make changes.
//  StringsReadme.htm
//===============================================================================================================================
//{{AFX_INSERT_LOCATION}}

#ifndef STUDIOSTRINGSH
#define STUDIOSTRINGSH

#define STRING_RESOURCE_COUNT %s
''' % ( theStringCount )

theResultLine = r'''
<!-- ==============================================================================================================================	-->
<!--          CODEGEN - FILE CREATED BY STUDIO CODE GENERATION SYSTEM																-->
<!--                                                                                												-->
<!-- 	Do not modify the contents of this file.  Your changes will be destroyed    												-->
<!--    by code generation.                                                         												-->
<!--																																-->
<!-- 	Please see the CodeGen folder for more information or to make changes.														-->
<!--    StringsReadme.htm  -->
<!-- ============================================================================================================================== -->

<stringresourceinput>
'''

theHeaderFile.write( theHeaderLine )
for theName, theList in theLists:
        theFileName = theName  + '.stro'
        print('write file %s' % ( theFileName ))
        theResultFile = codecs.open( theFileName, 'w', 'utf-16' )
        theResultFile.write( theResultLine )
        for theObject in theList.GetObjects():
                theName = theObject.GetName()
                theValue = theObject.GetValue()
                theResultFile.write( '\t<string name="%s" ID="%s" value="%s"/>\n' % ( theName, theIndex, theValue ) )
                theHeaderFile.write( '#define %s %s\n' % ( theName, theIndex ) )
                theIndex+= 1
        theResultFile.write( '</stringresourceinput>' )
        theResultFile.close()

theHeaderFile.write( '\n#endif //STUDIOSTRINGSH' )
theHeaderFile.close()
