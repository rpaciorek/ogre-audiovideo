/**
* @author  Ian Stangoe
*
* LICENSE:
* 
* This source file is part of OgreOggSound, an OpenAL wrapper library for   
* use with the Ogre Rendering Engine.										 
*                                                                           
* Copyright (c) 2017 Ian Stangoe
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.  
*
* DESCRIPTION: Implements methods for creating/using a static wav sound
*/

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <string>
#include <vector>
#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "OgreOggISound.h"

namespace OgreOggSound
{
	//! CHUNK header information
	// Chunk section within a wav file ('data'/'fact'/'cue' etc..)
	typedef struct
	{
		char chunkID[4];	// 'data' or 'fact'
		int length;
	} ChunkHeader;
  
	//! WAVEFORMATEX header information
	/** Structure defining a WAVE sounds format.
	*/
	typedef struct
	{
		char mRIFF[4];					// 'RIFF'
		unsigned int mLength;
		char mWAVE[4];					// 'WAVE'
		char mFMT[4];					// 'fmt '
		unsigned int mHeaderSize;		// varies...
		unsigned short mFormatTag;
		unsigned short mChannels;		// 1,2 for stereo data is (l,r) pairs
		unsigned int mSamplesPerSec;
		unsigned int mAvgBytesPerSec;
		unsigned short mBlockAlign;      
		unsigned short mBitsPerSample;
	} WaveHeader;

	//! WAVEFORMATEXTENSIBLE sound information
	/** Structure defining a WAVE sounds format.
	*/
	typedef struct
	{
		WaveHeader*	mFormat;
		unsigned short mSamples;
		unsigned int mChannelMask;
		char mSubFormat[16];	
	} WavFormatData;

	//! A single static buffer sound (WAV)
	/** Handles playing a sound from memory.
	 */
	class _OGGSOUND_EXPORT OgreOggStaticWavSound : public OgreOggISound
	{
	public:
		/** Sets the loop status.
		@remarks
			Immediately sets the loop status if a source is associated
			@param loop
				If true, then sound will loop
		 */
		void loop(bool loop);
		/** Sets the source to use for playback.
		@remarks
			Sets the source object this sound will use to queue buffers onto
			for playback. Also handles refilling buffers and queuing up.
			@param src
				OpenAL Source ID.
		 */
		void setSource(ALuint src);	
		/** Returns whether sound is mono
		*/
		bool isMono();
		/** Returns the buffer sample rate
		 */
		unsigned int getSampleRate() { return mFormatData.mFormat->mSamplesPerSec; };
		/** Returns the buffer number of channels
		 */
		unsigned short getChannels()  { return mFormatData.mFormat->mChannels; };
		/** Returns the buffer bits per sample
		 */
		unsigned int getBitsPerSample() { return mFormatData.mFormat->mBitsPerSample; };
		/** Gets the sounds file name
		 */
		virtual const Ogre::String& getFileName( void ) const { return mAudioName; }

	protected:	
		/**
		 * Constructor
		@remarks
			Creates a static sound object for playing audio from a WAV file.
			@param name
				Unique name for sound.
			@param scnMgr
				SceneManager which created this sound (if the sound was created through the plugin method createMovableobject()).
		 */
		OgreOggStaticWavSound(
			const Ogre::String& name
			#if AV_OGRE_NEXT_VERSION >= 0x20000
			, Ogre::SceneManager* scnMgr, Ogre::IdType id, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
			#endif
		);
		/**
		 * Destructor
		 */
		~OgreOggStaticWavSound();
		/** Releases buffers.
		@remarks
			Cleans up and releases OpenAL buffer objects.
		 */
		void release();
		/** Opens audio file.
		@remarks
			Opens a specified file and checks validity.
			Reads first chunks of audio data into buffers.
			@param fileStream
				File stream pointer
		 */
		void _openImpl(Ogre::DataStreamPtr& fileStream);
		/** Opens audio file.
		@remarks
			Uses a shared buffer.
			@param fName
				Audio file name
			@param buffer
				Shared buffer reference
		 */
		void _openImpl(const Ogre::String& fName, sharedAudioBuffer* buffer);
		/** Stops playing sound.
		@remarks
			Stops playing audio immediately. If specified to do so its source
			will be released also.
		*/
		void _stopImpl();
		/** Pauses sound.
		@remarks
			Pauses playback on the source.
		 */
		void _pauseImpl();
		/** Plays the sound.
		@remarks
			Begins playback of all buffers queued on the source. If a
			source hasn't been setup yet it is requested and initialised
			within this call.
		 */
		void _playImpl();	
		/** Updates the data buffers with sound information.
		@remarks
			This function refills processed buffers with audio data from
			the stream, it automatically handles looping if set.
		 */
		void _updateAudioBuffers();
		/** Prefills buffer with audio data.
		@remarks
			Loads audio data onto the source ready for playback.
		 */
		void _prebuffer();		
		/** Calculates buffer size and format.
		@remarks
			Calculates a block aligned buffer size of 250ms using
			sound properties.
		 */
		bool _queryBufferInfo();		
		/** Releases buffers and OpenAL objects.
		@remarks
			Cleans up this sounds OpenAL objects, including buffers
			and file pointers ready for destruction.
		 */
		void _release();	

		std::vector<char> mBufferData;		// Sound data buffer
		Ogre::String	mAudioName;			// Name of audio file stream (Used with shared buffers)
		ALint mPreviousOffset;				// Current play position
		WavFormatData mFormatData;			// WAVE format structure

		friend class OgreOggSoundManager;	
	};
}
