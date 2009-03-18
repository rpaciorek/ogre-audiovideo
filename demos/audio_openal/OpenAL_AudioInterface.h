/************************************************************************************
This source file is part of the TheoraVideoPlugin ExternalTextureSource PlugIn 
for OGRE3D (Object-oriented Graphics Rendering Engine)
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright � 2008-2009 Kresimir Spes (kreso@cateia.com)

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
*************************************************************************************/

#ifndef _OpenAL_AudioInterface_h
#define _OpenAL_AudioInterface_h

#include "TheoraAudioInterface.h"
#include "TheoraVideoClip.h"
#include "TheoraTimer.h"

#include "al.h"
#include "alc.h"

namespace Ogre
{
	class OpenAL_AudioInterface : public TheoraAudioInterface, TheoraTimer
	{
		int mMaxBuffSize;
		int mBuffSize;
		short *mTempBuffer;
		struct
		{
			ALuint id;
			bool queued;
			int nSamples;
		}mBuffers[2];
		int mBufferIndex;
		ALuint mSource;
		int mNumProcessedSamples;
		float mSourceTime;
	public:
		OpenAL_AudioInterface(TheoraVideoClip* owner,int nChannels,int freq);
		~OpenAL_AudioInterface();
		void insertData(float** data,int nSamples);

		void update(float time_increase);
	};



	class OpenAL_AudioInterfaceFactory : public TheoraAudioInterfaceFactory
	{
	public:
		OpenAL_AudioInterfaceFactory();
		~OpenAL_AudioInterfaceFactory();
		OpenAL_AudioInterface* createInstance(TheoraVideoClip* owner,int nChannels,int freq);
	};
}

#endif