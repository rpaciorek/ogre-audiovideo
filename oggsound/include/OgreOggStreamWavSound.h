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
* DESCRIPTION: Implements methods for creating/using a streamed wav sound
*/

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <string>
#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "OgreOggStaticWavSound.h"

namespace OgreOggSound
{
	//! A single streaming sound (WAV)
	/** Handles playing a sound from a wav stream.
	*/
	class _OGGSOUND_EXPORT OgreOggStreamWavSound : public OgreOggISound
	{
	public:
		/** Sets the position of the playback cursor in seconds
		@param seconds
			Play position in seconds 
		 */
		void setPlayPosition(float seconds);	
		/** Gets the position of the playback cursor in seconds
		 */
		float getPlayPosition() const;	
		/** Sets the source to use for playback.
		@remarks
			Sets the source object this sound will use to queue buffers onto
			for playback. Also handles refilling buffers and queuing up.
			@param src
				OpenAL Source ID.
		 */
		void setSource(ALuint src);
		/** Sets looping status.
		@remarks
			Sets wheter the sound should loop
			@param loop
				If true, then sound will loop
		@note
			Bear in mind that this being a streamed sound, 
			the option is not applied immediately but has a delay given by the buffer size.
		 */
		void loop(bool loop) { mLoop = loop; };
		/** Sets the start point of a loopable section of audio.
		@remarks
			Allows user to define any start point for a loopable sound, by default this would be 0, 
			or the entire audio data, but this function can be used to offset the start of the loop. 
			@param startTime
				Position in seconds to offset the loop point.
		@note
			The sound will start playback from the beginning of the audio data but upon looping, 
			if set, it will loop back to the new offset position.
		 */
		void setLoopOffset(float startTime);
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

	protected:	
		/** Constructor
		@remarks
			Creates a streamed sound object for playing audio directly from a WAV file stream.
			@param name
				Unique name for sound.
			@param scnMgr
				SceneManager which created this sound (if the sound was created through the plugin method createMovableobject()).
		 */
		OgreOggStreamWavSound(
			const Ogre::String& name
			#if AV_OGRE_NEXT_VERSION >= 0x20000
			, Ogre::SceneManager* scnMgr, Ogre::IdType id, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
			#endif
		);
		/**
		 * Destructor
		 */
		~OgreOggStreamWavSound();
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
		/** Stops playing sound.
		@remarks
			Stops playing audio immediately and resets playback. 
			If specified to do so its source will be released also.
		*/
		void _stopImpl();
		/** Pauses sound.
		@remarks
			Pauses playback on the source.
		 */
		void _pauseImpl();
		/** Plays the sound.
		@remarks
			Begins playback of all buffers queued on the source. 
			If a source hasn't been setup yet it is requested and initialised
			within this call.
		 */
		void _playImpl();	
		/** Loads data from the stream into a buffer.
		@remarks
			Reads a specified chunk of data from the file stream into a
			designated buffer object.
			@param buffer
				OpenAL buffer ID to load data into.
		 */
		bool _stream(ALuint buffer);
		/** Updates the data buffers with sound information.
		@remarks
			This function refills processed buffers with audio data from
			the stream, it automatically handles looping if set.
		 */
		void _updateAudioBuffers();
		/** Unqueues buffers from the source.
		@remarks
			Unqueues all data buffers currently queued on the associated
			source object.
		 */
		void _dequeue();
		/** Prefills buffers with audio data.
		@remarks
			Loads audio data from the stream into the predefined data
			buffers and queues them onto the source ready for playback.
		 */
		void _prebuffer();		
		/** Calculates buffer size and format.
		@remarks
			Calculates a block aligned buffer size of 250ms using
			sound properties.
		 */
		bool _queryBufferInfo();		
		/** handles a request to set the playback position within the stream
		@remarks
			To ensure thread safety this function performs the request within
			the thread locked update function instead of 'immediate mode' for static sounds.
		 */
		void _updatePlayPosition();		
		/** Releases buffers and OpenAL objects.
		@remarks
			Cleans up this sounds OpenAL objects, including buffers
			and file pointers ready for destruction.
		 */
		void _release();	

		/**
		 * Ogg file variables
		 */
		bool mStreamEOF;					// EOF flag
		WavFormatData mFormatData;			// WAVE format structure
		unsigned int mLoopOffsetBytes;		// Loop offset in bytes
		float mLastOffset;					// Offset in seconds

		friend class OgreOggSoundManager;
	};
}
