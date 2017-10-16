/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

namespace Q3DStudio
{

//==============================================================================
/**
 *	Destructor
 */
inline CThreadSafeScratchPad::~CThreadSafeScratchPad()
{
	FOR_ARRAY( SBufferItem, theObject, m_Buffers )
	{
		free( theObject->m_Buffer.m_Data );
	}
	m_Buffers.Clear();
}

//==============================================================================
/**
*	Ask the scratch pad that for a to use
*
*	Possible cases:
*		1. if there are unused buffers
			1.a use the buffer that fits, and wastes the least amount of memory
			1.b if no buffers fit, resize the biggest unused buffer
		2. if no unused buffers
			2.a allocate a new buffer
*/
inline SThreadScratchPadBuffer CThreadSafeScratchPad::GetBuffer( INT32 inMinSize )
{
	CSyncGuard theGuard( m_Primitive );
	INT32	theBestIndex			= -1;
	UINT32	theBestIndexWasteSize	= static_cast<UINT32>( -1 );		// uint32 max

	INT32 theLargestUnusedBuffer		= -1;
	INT32 theLargestUnusedBufferSize	= 0;

	SThreadScratchPadBuffer theReturnBuffer = {0,0};

	if ( inMinSize < 0 )
	{
		return theReturnBuffer;
	}

	INT32 theEnd = m_Buffers.GetCount();
	for ( INT32 theIndex = 0; theIndex < theEnd; ++theIndex )
	{
		SBufferItem& theBufferItem = m_Buffers[theIndex];

		// Unused buffers...
		if ( false == theBufferItem.m_InUse )
		{
			// Best fit unused buffer that will fit the requested size
			if ( theBufferItem.m_Buffer.m_Size >= inMinSize && static_cast<UINT32>( theBufferItem.m_Buffer.m_Size - inMinSize ) < theBestIndexWasteSize )
			{
				theBestIndex = theIndex;
				theBestIndexWasteSize = static_cast<UINT32>( theBufferItem.m_Buffer.m_Size - inMinSize);
			}
			// Biggest unused buffer to resize...
			else if ( theBufferItem.m_Buffer.m_Size > theLargestUnusedBufferSize )
			{
				theLargestUnusedBuffer = theIndex;
				theLargestUnusedBufferSize = theBufferItem.m_Buffer.m_Size;
			}
		}
	}
	

	// 1.a
	if ( -1 != theBestIndex  )
	{
		m_Buffers[theBestIndex].m_InUse = true;
		theReturnBuffer = m_Buffers[theBestIndex].m_Buffer;
	}
	// 1.b
	else if ( -1 != theLargestUnusedBuffer )
	{
		m_Buffers[theLargestUnusedBuffer].m_InUse = true;
		m_Buffers[theLargestUnusedBuffer].m_Buffer.m_Data = 
					static_cast<CHAR*>(	realloc( m_Buffers[theLargestUnusedBuffer].m_Buffer.m_Data, 
					static_cast<size_t>( inMinSize ) ) 
				);

		m_Buffers[theLargestUnusedBuffer].m_Buffer.m_Size = inMinSize;
		theReturnBuffer = m_Buffers[theLargestUnusedBuffer].m_Buffer;
	}
	// 2.a
	else
	{
		// Add a new buffer
		SBufferItem theNewBuffer;
		theNewBuffer.m_InUse			= true; 
		theNewBuffer.m_Buffer.m_Data	= static_cast<CHAR*>( malloc( static_cast<size_t>( inMinSize ) ) );
		theNewBuffer.m_Buffer.m_Size	= inMinSize;
		m_Buffers.Push( theNewBuffer );	
	
		theReturnBuffer					= theNewBuffer.m_Buffer;
	}
	theReturnBuffer.m_RequestedSize = inMinSize;
	return theReturnBuffer;
}

//==============================================================================
/**
 *	Notify the scratch pad that the buffer is now safe to reuse
 */
inline void CThreadSafeScratchPad::ReleaseBuffer( SThreadScratchPadBuffer inBuffer )
{
	CSyncGuard theGuard( m_Primitive );
	if ( inBuffer.m_Data == 0 )
		return;
	INT32 theIndex;
	INT32 theEnd = m_Buffers.GetCount();
	for ( theIndex = 0; theIndex < theEnd; ++theIndex )
	{
		SBufferItem& theBufferItem = m_Buffers[theIndex];
		if ( theBufferItem.m_Buffer.m_Size == inBuffer.m_Size && theBufferItem.m_Buffer.m_Data == inBuffer.m_Data )
		{
			theBufferItem.m_InUse = false;
			inBuffer.m_RequestedSize = 0;
			break;
		}
	}

        if ( theIndex == theEnd ) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "CThreadSafeScratchPad::ReleaseBuffer: attempting to remove "
                    << "invalid/corrupt buffer. Buffer size: " << inBuffer.m_Size;
        }
}

} // namespace Q3DStudio
