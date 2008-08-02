/*
-----------------------------------------------------------------------------
This source file is part of the TheoraVideoSystem ExternalTextureSource PlugIn 
for OGRE (Object-oriented Graphics Rendering Engine)
For the latest info, see www.wreckedgames.com or www.ogre3d.org
*****************************************************************************
				This PlugIn uses the following resources:

Ogre - see above
Ogg / Vorbis / Theora www.xiph.org
C++ Portable Types Library (PTypes - http://www.melikyan.com/ptypes/ )

*****************************************************************************
Copyright � 2000-2004 pjcast@yahoo.com

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the 
Free Software Foundation; either version 2 of the License, or (at your option) 
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
***************************************************************************/

#ifndef _TheoraOgreVideoDriverHeader_
#define _TheoraOgreVideoDriverHeader_

//#include "OgrePrerequisites.h"
#include "OgreTexture.h"
#include "OgreString.h"
#include "OgreImage.h"
#include "theora/theora.h"
#include "TheoraExport.h"
#include "TheoraPlayerPreReqs.h"

namespace Ogre 
{
	class _OgreTheoraExport TheoraVideoDriver
	{
	public:
		TheoraVideoDriver();
		~TheoraVideoDriver();
		
		/** 
			@remarks
				A method to test the texture... Also creates a nice video
				static effect 
		*/
		void randomizeTexture();

		/** 
			@remarks
				Attach our video to a texture unit (must already exist)
			@param sMaterialName
				String name of the material
			@param sTextureName
				String name for the new Texture
			@param sGroupName
				String name for the resource group this should belong too.
				Also NOTE: The material and Movie File MUST reside in same group
			@param TechniqueLevel
				The texhnique level this goes on
			@param PassLevel
				The pass level this goes on
			@param TextureUnitStateLevel
				The Texture Unit State this goes on
			@param width
				Width of movie
			@param height
				Height of movie	
			@param renderMode
				Special FX modes (eg. alpha render)
		*/
		void attachVideoToTextureUnit( const String &sMaterialName, 
									   const String &sTextureName,
									   const String &sGroupName,
									   int TechniqueLevel, 
									   int PassLevel, int TextureUnitStateLevel,
									   int width, int height,
									   TextureSpecialRenderFX renderMode = render_normal );
		
		/** 
			@remarks
				Get a pointer to the OGRE texture we are using
		*/
		TexturePtr& getTexture() { return mTexture; }
		
		/** 
			@remarks
				Takes a yuv_buffer (4:2:0 YCrBr) and dispatches to the appropriate
				renderer
			@param
				YUV Buffer
		*/
		void renderToTexture( yuv_buffer *buffer );

		unsigned int getWidth() { return m_Width; }
		unsigned int getHeight() { return m_Height; }

		/** 
			@remarks
				Called one time to init the lookup table used in color space 
				conversion 
		*/
		static void createCoefTables();
		
		//Lookup tables for ColorSpace conversions
		static unsigned int YTable[ 256 ];
		static unsigned int BUTable[ 256 ];
		static unsigned int GUTable[ 256 ];
		static unsigned int GVTable[ 256 ];
		static unsigned int RVTable[ 256 ];

	protected:
		/** 
			@remarks
				Takes a yuv_buffer (4:2:0 YCrBr) and decodes it to our 
				24Bit RGB texture
			@param
				YUV Buffer
		*/
		void decodeYUVtoTexture( yuv_buffer *yuv );

		/** 
			@remarks
				Takes a yuv_buffer (4:2:0 YCrBr) and renders the Y component ONLY
				to the alpha channel of this texture
			@param
				YUV Buffer
		*/
		void decodeYtoTexture( yuv_buffer *yuv );

		//! The image class we use
		Image m_Image;
		//! Pointer to the Ogre Texture
		TexturePtr mTexture;
		//! Pointer to bitmap memory
		unsigned char *m_RGBBitmap;

		//! Width of the movie (possibly not the width of the texture)
		unsigned int m_Width,
		//! Height of the movie (possibly not the hight of the texture)
					 m_Height;

		unsigned char m_BytesPerPixel;

		unsigned short m_Tec, m_Pass, m_Unit;

		String mTextureName;	//ALso the same as the file name given
		String mMaterialName;

		TextureSpecialRenderFX mRenderModeFx;
	};

} //end namespace

#endif //_TheoraOgreVideoDriverHeader_
