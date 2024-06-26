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
*/

#include "OgreOggStaticWavSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"

namespace OgreOggSound
{

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticWavSound::OgreOggStaticWavSound(
		const Ogre::String& name
		#if AV_OGRE_NEXT_VERSION >= 0x20000
		, Ogre::SceneManager* scnMgr, Ogre::IdType id, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
		#endif
	) : OgreOggISound(
			name
			#if AV_OGRE_NEXT_VERSION >= 0x20000
			, scnMgr, id, objMemMgr, renderQueueId
			#endif
		)
		,mAudioName("")
		,mPreviousOffset(0)
		{
			mStream=false;
			mFormatData.mFormat=0;
			mBufferData.clear();											 
			mBuffers.reset(new BufferList(1, AL_NONE));
		}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticWavSound::~OgreOggStaticWavSound()
	{
		// Notify listener
		if ( mSoundListener ) mSoundListener->soundDestroyed(this);

		_release();
		mBufferData.clear();
		if (mFormatData.mFormat) OGRE_FREE(mFormatData.mFormat, Ogre::MEMCATEGORY_GENERAL);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_openImpl(Ogre::DataStreamPtr& fileStream)
	{
		// WAVE descriptor vars
		char*			sound_buffer=0;
		int				bytesRead=0;
		ChunkHeader		c;

		// Store stream pointer
		mAudioStream = fileStream;

		// Store file name
		mAudioName = mAudioStream->getName();

		// Allocate format structure
		mFormatData.mFormat = OGRE_NEW_T(WaveHeader, Ogre::MEMCATEGORY_GENERAL);

		// Read in "RIFF" chunk descriptor (4 bytes)
		mAudioStream->read(mFormatData.mFormat, sizeof(WaveHeader));

		Ogre::String format;
		switch(mFormatData.mFormat->mFormatTag)
		{
			case 0x0001:
				format = "PCM";
				break;
			case 0x0003:
				format = "IEEE float (unsupported)";
				break;
			case 0x0006:
				format = "8-bit ITU-T G.711 A-law (unsupported)";
				break;
			case 0x0007:
				format = "8-bit ITU-T G.711 µ-law (unsupported)";
				break;
			default:
				format = "*unknown* (unsupported)";
				break;
		}

		Ogre::LogManager::getSingleton().logMessage("Sound '" + mAudioName + "': Loading WAV with " +
			Ogre::StringConverter::toString(mFormatData.mFormat->mChannels) + " channels, " +
			Ogre::StringConverter::toString(mFormatData.mFormat->mSamplesPerSec) + " Hz, " +
			Ogre::StringConverter::toString(mFormatData.mFormat->mBitsPerSample) + " bps " +
			format + " format.");

		// Valid 'RIFF'?
		if ( strncmp(mFormatData.mFormat->mRIFF, "RIFF", 4) != 0 )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND, mAudioName + " - Not a valid RIFF file!", "OgreOggStaticWavSound::_openImpl()");
		}

		// Valid 'WAVE'?
		if ( strncmp(mFormatData.mFormat->mWAVE, "WAVE", 4) != 0 )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, mAudioName + " - Not a valid WAVE file!", "OgreOggStaticWavSound::_openImpl()");
		}

		// Valid 'fmt '?
		if ( strncmp(mFormatData.mFormat->mFMT, "fmt", 3) != 0 )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, mAudioName + " - Invalid Format!", "OgreOggStaticWavSound::_openImpl()");
		}

		// mFormatData.mFormat: Should be 16 unless compressed ( compressed NOT supported )
		if ( !mFormatData.mFormat->mHeaderSize >= 16 )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, mAudioName + " - Compressed WAV NOT supported!", "OgreOggStaticWavSound::_openImpl()");
		}

		// PCM == 1
		if ( (mFormatData.mFormat->mFormatTag != 0x0001) && (mFormatData.mFormat->mFormatTag != 0xFFFE) )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, mAudioName + " - WAV file NOT in PCM format!", "OgreOggStaticWavSound::_openImpl()");
		}

		// Bits per sample check..
		if ( (mFormatData.mFormat->mBitsPerSample != 16) && (mFormatData.mFormat->mBitsPerSample != 8) )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, mAudioName + " - BitsPerSample NOT 8/16, unsupported format!", "OgreOggStaticWavSound::_openImpl()");
		}

		// Calculate extra WAV header info
		unsigned int extraBytes = mFormatData.mFormat->mHeaderSize - (sizeof(WaveHeader) - 20);

		// If WAVEFORMATEXTENSIBLE read attributes
		if (mFormatData.mFormat->mFormatTag==0xFFFE)
		{
			extraBytes-=static_cast<unsigned int>(mAudioStream->read(&mFormatData.mSamples, 2));
			extraBytes-=static_cast<unsigned int>(mAudioStream->read(&mFormatData.mChannelMask, 2));
			extraBytes-=static_cast<unsigned int>(mAudioStream->read(&mFormatData.mSubFormat, 16));
		}

		// Skip
		mAudioStream->skip(extraBytes);

		do
		{
			// Read in chunk header
			mAudioStream->read(&c, sizeof(ChunkHeader));

			// 'data' chunk...
			//if ( c.chunkID[0]=='d' && c.chunkID[1]=='a' && c.chunkID[2]=='t' && c.chunkID[3]=='a' )
			if ( strncmp(c.chunkID, "data", 4) == 0 )
			{
				// Store byte offset of start of audio data
				mAudioOffset = static_cast<unsigned int>(mAudioStream->tell());

				// Check data size
				int fileCheck = c.length % mFormatData.mFormat->mBlockAlign;

				// Store end pos
				mAudioEnd = mAudioOffset+(c.length-fileCheck);

				// Allocate array
				sound_buffer = OGRE_ALLOC_T(char, mAudioEnd-mAudioOffset, Ogre::MEMCATEGORY_GENERAL);

				// Read entire sound data
				bytesRead = static_cast<int>(mAudioStream->read(sound_buffer, mAudioEnd-mAudioOffset));

				// Jump out
				break;
			}
			// Skip unsupported chunk...
			else {
				if( (mAudioStream->tell() / sizeof(ChunkHeader)) % 100000 == 0)
					Ogre::LogManager::getSingleton().logMessage("OgreOggStaticWavSound::_openImpl() - Looking for 'data' chunk in: " + fileStream->getName());

				mAudioStream->skip(c.length);
			}
		}
		while ( mAudioStream->eof() || ( strncmp(c.chunkID, "data", 4) != 0 ));

		// Create OpenAL buffer
		alGetError();
		alGenBuffers(1, &(*mBuffers)[0]);
		if ( alGetError() != AL_NO_ERROR )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Unable to create OpenAL buffer.", "OgreOggStaticWavSound::_openImpl()");
			return;
		}

#if OGGSOUND_HAVE_EFX == 1
		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(1, &(*mBuffers)[0]);
#endif

		// Check format support
		if (!_queryBufferInfo())
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Format NOT supported.", "OgreOggStaticWavSound::_openImpl()");

		// Calculate length in seconds
		mPlayTime = static_cast<float>(((mAudioEnd-mAudioOffset) * 8.f) / static_cast<float>((mFormatData.mFormat->mSamplesPerSec * mFormatData.mFormat->mChannels * mFormatData.mFormat->mBitsPerSample)));

		alGetError();
		alBufferData((*mBuffers)[0], mFormat, sound_buffer, static_cast<ALsizei>(bytesRead), mFormatData.mFormat->mSamplesPerSec);
		if ( alGetError() != AL_NO_ERROR )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Unable to load audio data into buffer!", "OgreOggStaticWavSound::_openImpl()");
			return;
		}

		OGRE_FREE(sound_buffer, Ogre::MEMCATEGORY_GENERAL);

		// Register shared buffer
		OgreOggSoundManager::getSingleton()._registerSharedBuffer(mAudioName, (*mBuffers)[0], this);

		// Notify listener
		if ( mSoundListener ) mSoundListener->soundLoaded(this);
	}
	/*/////////////////////////////////////////////////////////////////*/	  
	void OgreOggStaticWavSound::_openImpl(const Ogre::String& fName, sharedAudioBuffer* buffer)
	{
		if ( !buffer ) return;

		// Set buffer
		_setSharedProperties(buffer);

		// Filename
		mAudioName = fName;

		// Notify listener
		if ( mSoundListener ) mSoundListener->soundLoaded(this);
	}   
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStaticWavSound::isMono()
	{
		if ( !mInitialised ) return false;

		return ( (mFormat==AL_FORMAT_MONO16) || (mFormat==AL_FORMAT_MONO8) );
	}					   
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStaticWavSound::_queryBufferInfo()
	{
		if ( !mFormatData.mFormat ) return false;

		switch(mFormatData.mFormat->mChannels)
		{
		case 1:
			{
				if ( mFormatData.mFormat->mBitsPerSample == 8 )
				{
					// 8-bit mono
					mFormat = AL_FORMAT_MONO8;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize = mFormatData.mFormat->mSamplesPerSec/4;
				}
				else
				{
					// 16-bit mono
					mFormat = AL_FORMAT_MONO16;

					// Queue 250ms of audio data
					mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
				}
			}
			break;
		case 2:
			{
				if ( mFormatData.mFormat->mBitsPerSample == 8 )
				{
					// 8-bit stereo
					mFormat = AL_FORMAT_STEREO8;

					// Set BufferSize to 250ms (Frequency * 2 (8bit stereo) divided by 4 (quarter of a second))
					mBufferSize = mFormatData.mFormat->mSamplesPerSec >> 1;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % 2);
				}
				else
				{
					// 16-bit stereo
					mFormat = AL_FORMAT_STEREO16;

					// Queue 250ms of audio data
					mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
				}
			}
			break;
		case 4:
			{
				// 16-bit Quad surround
				mFormat = alGetEnumValue("AL_FORMAT_QUAD16");
				if (!mFormat) return false;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		case 6:
			{
				// 16-bit 5.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_51CHN16");
				if (!mFormat) return false;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		case 7:
			{
				// 16-bit 7.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_61CHN16");
				if (!mFormat) return false;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		case 8:
			{
				// 16-bit 8.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_71CHN16");
				if (!mFormat) return false;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		default:
			{
				// Error message
				Ogre::LogManager::getSingleton().logMessage("Unable to determine number of channels: defaulting to 16-bit stereo");

				// 16-bit stereo
				mFormat = AL_FORMAT_STEREO16;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		}
		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_release()
	{
		setSource(AL_NONE);
		OgreOggSoundManager::getSingleton()._releaseSharedBuffer(mAudioName, (*mBuffers)[0]);
		mPlayPosChanged = false;
		mPlayPos = 0.f;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_prebuffer()
	{
		if (mSource==AL_NONE) return;

		// Queue buffer
		alSourcei(mSource, AL_BUFFER, (*mBuffers)[0]);
	}					
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::setSource(ALuint src)
	{
		if (src != AL_NONE)
		{
			// Attach new source
			mSource=src;

			// Load audio data onto source
			_prebuffer();

			// Init source properties
			_initSource();
		}
		else
		{
			// Validity check
			if ( mSource != AL_NONE )
			{
				// Need to stop sound BEFORE unqueuing
				alSourceStop(mSource);

				// Unqueue buffer
				alSourcei(mSource, AL_BUFFER, 0);
			}

			// Attach new source
			mSource=src;

			// Cancel initialisation
			mInitialised = false;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_pauseImpl()
	{
		assert(mState != SS_DESTROYED);

		if ( mSource==AL_NONE ) return;

		alSourcePause(mSource);
		mState = SS_PAUSED;

		// Notify listener
		if ( mSoundListener ) mSoundListener->soundPaused(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_playImpl()
	{
		assert(mState != SS_DESTROYED);

		if(isPlaying())
			return;

		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton()._requestSoundSource(this) )
				return;

		// Pick up position change
		if ( mPlayPosChanged )
			setPlayPosition(mPlayPos);

		alSourcePlay(mSource);
		mState = SS_PLAYING;

		// Notify listener
		if ( mSoundListener ) mSoundListener->soundPlayed(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_stopImpl()
	{
		assert(mState != SS_DESTROYED);

		if ( mSource==AL_NONE ) return;

		alSourceStop(mSource);
		alSourceRewind(mSource);
		mState = SS_STOPPED;
		mPreviousOffset = 0;

		if (mTemporary)
		{
			mState = SS_DESTROYED;
			OgreOggSoundManager::getSingleton()._destroyTemporarySound(this);
		}
		// Give up source immediately if specfied
		else if (mGiveUpSource) 
			OgreOggSoundManager::getSingleton()._releaseSoundSource(this);

		// Notify listener
		if ( mSoundListener ) mSoundListener->soundStopped(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::loop(bool loop)
	{
		mLoop = loop;

		if(mSource != AL_NONE)
		{
			alSourcei(mSource, AL_LOOPING, loop);

			if ( alGetError() != AL_NO_ERROR )
				OGRE_LOG_ERROR("OgreOggStaticWavSound::loop() - Unable to set looping status!");
		}
		else
			Ogre::LogManager::getSingleton().logMessage("OgreOggStaticWavSound::loop() - No source attached to sound!");
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_updateAudioBuffers()
	{
		if (!isPlaying())
			return;

		ALenum state;
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);

		if (state == AL_STOPPED)
		{
			stop();
			
			// Finished callback
			if ( mSoundListener ) 
				mSoundListener->soundFinished(this);
		}
		else
		{
			ALint bytes=0;

			// Use byte offset to work out current position
			alGetSourcei(mSource, AL_BYTE_OFFSET, &bytes);

			// Has the audio looped?
			if ( mPreviousOffset>bytes )
			{
				// Notify listener
				if ( mSoundListener ) mSoundListener->soundLooping(this);
			}

			// Store current offset position
			mPreviousOffset=bytes;
		}
	}
}
