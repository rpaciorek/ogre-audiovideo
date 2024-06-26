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

#include "OgreOggISound.h"
#include "OgreOggSound.h"
#include <OgreMovableObject.h>

namespace OgreOggSound
{
	/*
	** These next four methods are custom accessor functions to allow the Ogg Vorbis
	** libraries to be able to stream audio data directly from an Ogre::DataStreamPtr
	*/
	size_t	OOSStreamRead(void *ptr, size_t size, size_t nmemb, void *datasource)
	{
		Ogre::DataStreamPtr dataStream = *reinterpret_cast<Ogre::DataStreamPtr*>(datasource);
		return dataStream->read(ptr, size * nmemb);
	}
	int		OOSStreamSeek(void *datasource, ogg_int64_t offset, int whence)
	{
		Ogre::DataStreamPtr dataStream = *reinterpret_cast<Ogre::DataStreamPtr*>(datasource);
		switch(whence)
		{
		case SEEK_SET:
			dataStream->seek(offset);
			break;
		case SEEK_END:
			dataStream->seek(dataStream->size());
			// Falling through purposefully here
		case SEEK_CUR:
			dataStream->skip(offset);
			break;
		}

		return 0;
	}
	int		OOSStreamClose(void *datasource)
	{
		return 0;
	}
	long	OOSStreamTell(void *datasource)
	{
		Ogre::DataStreamPtr dataStream = *reinterpret_cast<Ogre::DataStreamPtr*>(datasource);
		return static_cast<long>(dataStream->tell());
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound::OgreOggISound(
		const Ogre::String& name
		#if AV_OGRE_NEXT_VERSION >= 0x20000
		, Ogre::SceneManager* scnMgr, Ogre::IdType id, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
		#endif
	) : 
	#if AV_OGRE_NEXT_VERSION >= 0x20100
	MovableObject(id, objMemMgr, scnMgr, renderQueueId),
	mPosition(0,0,0),
	mDirection(0,0,0),
	mName(name),
	#else
	MovableObject(name),
	#endif
	mSource(0) 
	,mLoop(false) 
	,mState(SS_NONE) 
	,mReferenceDistance(1.0f) 
	,mVelocity(0,0,0) 
	,mGain(1.0f) 
	,mMaxDistance(1E10) 
	,mMaxGain(1.0f) 
	,mMinGain(0.0f)
	,mPitch(1.0f) 
	,mRolloffFactor(1.0f) 
	,mInnerConeAngle(360.0f) 
	,mOuterConeAngle(360.0f) 
	,mOuterConeGain(0.0f) 
	,mPlayTime(0.0f) 
	,mFadeTimer(0.0f) 
	,mFadeTime(1.0f) 
	,mFadeInitVol(0) 
	,mFadeEndVol(1) 
	,mFade(false) 
	,mFadeEndAction(OgreOggSound::FC_NONE)  
	,mStream(false) 
	,mGiveUpSource(false)  
	,mPlayPosChanged(false)  
	,mPlayPos(0.f) 
	,mPriority(0)
	,mAudioOffset(0)
	,mAudioEnd(0)
	,mLoopOffset(0)
	#if !AV_OGRE_NEXT_VERSION
	,mLocalTransformDirty(true)
	#endif
	,mDisable3D(false)
	,mSeekable(true)
	,mSourceRelative(false)
	,mTemporary(false)
	,mInitialised(false)
	,mAwaitingDestruction(0)
	,mSoundListener(0)
	{
		// Init some oggVorbis callbacks
		mOggCallbacks.read_func	= OOSStreamRead;
		mOggCallbacks.close_func= OOSStreamClose;
		mOggCallbacks.seek_func	= OOSStreamSeek;
		mOggCallbacks.tell_func	= OOSStreamTell;
		mBuffers.reset();
		#if AV_OGRE_NEXT_VERSION >= 0x20000
		setLocalAabb(Ogre::Aabb::BOX_NULL);
		setQueryFlags(0);
		#endif
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound::~OgreOggISound() 
	{
		OgreOggSoundManager::getSingletonPtr()->_releaseSoundImpl(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	#if AV_OGRE_NEXT_VERSION >= 0x20000
	Ogre::String OgreOggISound::getName() {
		return mName;
	}
	#endif
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::_getSharedProperties(BufferListPtr& buffers, float& length, ALenum& format) 
	{
		buffers = mBuffers;
		length = mPlayTime;
		format = mFormat;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::_setSharedProperties(sharedAudioBuffer* buffer) 
	{
		mBuffers = buffer->mBuffers;
		mPlayTime = buffer->mPlayTime;
		mFormat = buffer->mFormat;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::play(bool immediate)
	{
		assert(mState != SS_DESTROYED);

#if OGGSOUND_THREADED
		SoundAction action;
		action.mSound = mName;
		action.mAction = LQ_PLAY;
		action.mImmediately = immediate;
		action.mParams = 0;
		OgreOggSoundManager::getSingletonPtr()->_requestSoundAction(action);
#else
		_playImpl();
#endif
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::stop(bool immediate)
	{
		assert(mState != SS_DESTROYED);

#if OGGSOUND_THREADED
		SoundAction action;
		action.mSound = mName;
		action.mAction = LQ_STOP;
		action.mImmediately = immediate;
		action.mParams = 0;
		OgreOggSoundManager::getSingletonPtr()->_requestSoundAction(action);
#else
		_stopImpl();
#endif
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::pause(bool immediate)
	{
		assert(mState != SS_DESTROYED);

#if OGGSOUND_THREADED
		SoundAction action;
		action.mSound = mName;
		action.mAction = LQ_PAUSE;
		action.mImmediately = immediate;
		action.mParams = 0;
		OgreOggSoundManager::getSingletonPtr()->_requestSoundAction(action);
#else
		_pauseImpl();
#endif
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::disable3D(bool disable)
	{
		// Set flag
		mDisable3D = disable;

		/** Disable 3D
		@remarks
			Requires setting listener relative to AL_TRUE
			Position to ZERO.
			Reference distance is set to 1.
			If Source available then settings are applied immediately
			else they are applied next time the sound is initialised.
		*/
		if ( mDisable3D )
		{
			mSourceRelative = true;

			if ( mSource!=AL_NONE ) 
			{
				alSourcei(mSource, AL_SOURCE_RELATIVE, mSourceRelative);
				alSource3f(mSource, AL_POSITION, 0, 0, 0);
				alSource3f(mSource, AL_VELOCITY, 0, 0, 0);
			}
		}
		/** Enable 3D
		@remarks
			Set listener relative to AL_FALSE
			If Source available then settings are applied immediately
			else they are applied next time the sound is initialised.
			NOTE:- If previously disabled, Reference distance will still be set to 1.
			Should be reset as required by user AFTER calling this function.
		*/
		else
		{
			mSourceRelative = false;

			if ( mSource!=AL_NONE ) 
			{
				alSourcei(mSource, AL_SOURCE_RELATIVE, mSourceRelative);
			}
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggISound::is3Ddisabled()
	{
		return mDisable3D;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setVelocity(const Ogre::Vector3 &vel)
	{
		mVelocity = vel;	

		if(mSource != AL_NONE)
		{
			alSource3f(mSource, AL_VELOCITY, vel.x, vel.y, vel.z);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setVolume(float gain)
	{
		if(gain < 0) return;

		mGain = gain;

		if(mSource != AL_NONE)
		{
			alSourcef(mSource, AL_GAIN, mGain);		
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setMaxVolume(float maxGain)
	{
		if(maxGain < 0 || maxGain > 1) return;

		mMaxGain = maxGain;

		if(mSource != AL_NONE)
		{
			alSourcef(mSource, AL_MAX_GAIN, mMaxGain);	
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getMaxVolume()
	{
		return mMaxGain;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setMinVolume(float minGain)
	{
		if(minGain < 0 || minGain > 1) return;

		mMinGain = minGain;

		if(mSource != AL_NONE)
		{
			alSourcef(mSource, AL_MIN_GAIN, mMinGain);		
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getMinVolume()
	{
		return mMinGain;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setConeAngles(float insideAngle, float outsideAngle)
	{
		if(insideAngle < 0 || insideAngle > 360)	return;
		if(outsideAngle < 0 || outsideAngle > 360)	return;

		mInnerConeAngle = insideAngle;
		mOuterConeAngle = outsideAngle;

		if(mSource != AL_NONE)
		{
			alSourcef (mSource, AL_CONE_INNER_ANGLE,	mInnerConeAngle);
			alSourcef (mSource, AL_CONE_OUTER_ANGLE,	mOuterConeAngle);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getConeInsideAngle()
	{
		return mInnerConeAngle;
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getConeOutsideAngle()
	{
		return mOuterConeAngle;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setOuterConeVolume(float gain)
	{
		if(gain < 0 || gain > 1)	return;

		mOuterConeGain = gain;

		if(mSource != AL_NONE)
		{
			alSourcef (mSource, AL_CONE_OUTER_GAIN, mOuterConeGain);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getOuterConeVolume()
	{
		return mOuterConeGain;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setMaxDistance(float maxDistance)
	{
		if(maxDistance < 0) return;

		mMaxDistance = maxDistance;

		if(mSource != AL_NONE)
		{
			alSourcef(mSource, AL_MAX_DISTANCE, mMaxDistance);		
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	const float OgreOggISound::getMaxDistance() const
	{
		float val=-1.f;
		if(mSource != AL_NONE)
		{
			alGetSourcef(mSource, AL_MAX_DISTANCE, &val);		
		}
		return val;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setRolloffFactor(float rolloffFactor)
	{
		if(rolloffFactor < 0) return;

		mRolloffFactor = rolloffFactor;

		if(mSource != AL_NONE)
		{
			alSourcef(mSource, AL_ROLLOFF_FACTOR, mRolloffFactor);		
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	const float OgreOggISound::getRolloffFactor() const
	{
		float val=-1.f;
		if(mSource != AL_NONE)
		{
			alGetSourcef(mSource, AL_ROLLOFF_FACTOR, &val);		
		}
		return val;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setReferenceDistance(float referenceDistance)
	{
		if(referenceDistance < 0) return;

		mReferenceDistance = referenceDistance;

		if(mSource != AL_NONE)
		{
			alSourcef(mSource, AL_REFERENCE_DISTANCE, mReferenceDistance);		
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	const float OgreOggISound::getReferenceDistance() const
	{
		float val=-1.f;

		if(mSource != AL_NONE)
		{
			alGetSourcef(mSource, AL_REFERENCE_DISTANCE, &val);		
		}
		return val;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setPitch(float pitch)
	{
		if ( pitch<=0.f ) return;

		mPitch = pitch;

		if(mSource != AL_NONE)
		{
			alSourcef(mSource, AL_PITCH, mPitch);		
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	const float OgreOggISound::getPitch() const
	{
		float val=-1.f;
		if(mSource != AL_NONE)
		{
			alGetSourcef(mSource, AL_PITCH, &val);		
		}
		return val;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::_initSource()
	{
		//'reset' the source properties 		
		if(mSource != AL_NONE)
		{
			alSourcef (mSource, AL_GAIN, mGain);
			alSourcef (mSource, AL_MAX_GAIN, mMaxGain);
			alSourcef (mSource, AL_MIN_GAIN, mMinGain);
			alSourcef (mSource, AL_MAX_DISTANCE, mMaxDistance);	
			alSourcef (mSource, AL_ROLLOFF_FACTOR, mRolloffFactor);
			alSourcef (mSource, AL_REFERENCE_DISTANCE, mReferenceDistance);
			alSourcef (mSource, AL_CONE_OUTER_GAIN, mOuterConeGain);
			alSourcef (mSource, AL_CONE_INNER_ANGLE,	mInnerConeAngle);
			alSourcef (mSource, AL_CONE_OUTER_ANGLE,	mOuterConeAngle);
			alSource3f(mSource, AL_POSITION, 0, 0, 0);
			alSource3f(mSource, AL_DIRECTION, 0, 0, -1);
			alSource3f(mSource, AL_VELOCITY, mVelocity.x, mVelocity.y, mVelocity.z);
			alSourcef (mSource, AL_PITCH, mPitch);
			alSourcei (mSource, AL_SOURCE_RELATIVE, mSourceRelative);
			alSourcei (mSource, AL_LOOPING, mStream ? AL_FALSE : mLoop);
			mInitialised = true;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getVolume() const
	{
		float vol=0.f;

		// Check if a source is available
		if(mSource != AL_NONE)
		{
			alGetSourcef(mSource, AL_GAIN, &vol);		
			return vol;
		}
		// If not - return requested setting.
		else
			return mGain;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::startFade(bool fDir, float fadeTime, FadeControl actionOnComplete)
	{
		mFade			= true;
	    mFadeInitVol	= getVolume();
		mFadeEndVol		= fDir ? mMaxGain : 0.f;
		mFadeTimer		= 0.f;
		mFadeEndAction	= actionOnComplete;
		mFadeTime		= fadeTime;
		// Automatically start if not currently playing
		if ( mFadeEndVol==mMaxGain )
			if ( !isPlaying() )
				this->play();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::_updateFade(float fTime)
	{
		if (mFade)
		{
			if ( (mFadeTimer+=fTime) >= mFadeTime )
			{
				setVolume(mFadeEndVol);
				mFade = false;
				// Perform requested action on completion
				// NOTE:- Must go through SoundManager when using threads to avoid any corruption/mutex issues.
				switch ( mFadeEndAction ) 
				{
				case FC_PAUSE: 
					{ 
						pause(); 
					} 
					break;
				case FC_STOP: 
					{ 
						stop(); 
					} 
					break;
				}
			}
			else
			{
				float vol = (mFadeEndVol-mFadeInitVol)*(mFadeTimer/mFadeTime);
				setVolume(mFadeInitVol + vol);
			}
		} 
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::_markPlayPosition()
	{
		/** Ignore if no source available.
			With stream sounds the buffers will hold the audio data
			at the position it is kicked off at, although there is potential
			to be 1/4 second out, so may need to look at this..
			for now just re-use buffers
		*/
		if ( !mSeekable || (mSource==AL_NONE) || mStream )
			return;

		alSourcePause(mSource);
		alGetSourcef(mSource, AL_SEC_OFFSET, &mPlayPos);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::_recoverPlayPosition()
	{
		if ( !mSeekable || (mSource==AL_NONE) || mStream )
			return;

		alGetError();
		alSourcef(mSource, AL_SEC_OFFSET, mPlayPos);
		if (alGetError())
		{
			OGRE_LOG_ERROR("OgreOggISound::_recoverPlayPosition() - Unable to set play position");
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setPlayPosition(float seconds)
	{
		if (mSource == AL_NONE)
		{
			// Mark it so it can be applied when sound receives a source
			mPlayPosChanged = true;
			mPlayPos = seconds;
			return;
		}

		// Reset flag
		mPlayPosChanged = false;

		// Invalid time - exit
		if ( !mSeekable || mPlayTime<=0.f || seconds<0.f ) 
			return;

		// Wrap time
		seconds = std::fmod(seconds, mPlayTime);

		alGetError();
		alSourcef(mSource, AL_SEC_OFFSET, seconds);
		if (alGetError())
		{
			OGRE_LOG_ERROR("OgreOggISound::setPlayPosition() - Error setting play position");
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getPlayPosition() const
	{
		// Invalid time - exit
		if ( !mSeekable || !mSource ) 
			return -1.f;

		// Set offset if source available
		ALfloat offset=-1.f;
		alGetError();
		alGetSourcef(mSource, AL_SEC_OFFSET, &offset);
		if (alGetError())
		{
			OGRE_LOG_ERROR("OgreOggISound::setPlayPosition() - Error getting play position");
			return -1.f;
		}
			
		return offset;
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggISound::addCuePoint(float seconds)
	{
		// Valid time?
		if ( seconds > 0.f )
		{
			mCuePoints.push_back(seconds);
			return true;
		}
		else
			return false;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::removeCuePoint(unsigned short index)
	{
		if ( mCuePoints.empty() || ((int)mCuePoints.size()<=index) )
			return;

		// Erase element
		mCuePoints.erase(mCuePoints.begin()+index);
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getCuePoint(unsigned short index)
	{
		if ( mCuePoints.empty() || ((int)mCuePoints.size()<=index) )
			return -1.f;

		// get element
		return mCuePoints.at(index);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setCuePoint(unsigned short index)
	{
		if ( mCuePoints.empty() || ((int)mCuePoints.size()<=index) )
			return;

		// set cue point
		setPlayPosition(mCuePoints.at(index));
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::setRelativeToListener(bool relative)
	{
		mSourceRelative = relative;
		
		if(mSource != AL_NONE)
		{
			alSourcei(mSource, AL_SOURCE_RELATIVE, mSourceRelative);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::update(float fTime)
	{
		#if !AV_OGRE_NEXT_VERSION
		if (mLocalTransformDirty)
		{
			Ogre::Vector3 position(0, 0, 0);
			Ogre::Vector3 direction(0, 0, -1);
			if (!mDisable3D && mParentNode)
			{
				position = mParentNode->_getDerivedPosition();
				direction = -mParentNode->_getDerivedOrientation().zAxis();
			}

			if(mSource != AL_NONE)
			{
				alSource3f(mSource, AL_POSITION, position.x, position.y, position.z);
				alSource3f(mSource, AL_DIRECTION, direction.x, direction.y, direction.z);
				mLocalTransformDirty = false;
			}
		}
		#else
		if (!mDisable3D && mParentNode && mSource != AL_NONE) {
			Ogre::Vector3    newPos    = mParentNode->_getDerivedPosition();
			if (newPos != mPosition) {
				mPosition = newPos;
				alSource3f(mSource, AL_POSITION, mPosition.x, mPosition.y, mPosition.z);
			}
			
			Ogre::Vector3    newDir = -mParentNode->_getDerivedOrientation().zAxis();
			if (newDir != mDirection) {
				mDirection = newDir;
				alSource3f(mSource, AL_DIRECTION, mDirection.x, mDirection.y, mDirection.z);
			}
		}
		#endif

		_updateFade(fTime);
	}
	/*/////////////////////////////////////////////////////////////////*/
	const Ogre::String& OgreOggISound::getMovableType(void) const
	{
		return OgreOggSoundFactory::FACTORY_TYPE_NAME;
	}
	/*/////////////////////////////////////////////////////////////////*/
	const Ogre::AxisAlignedBox& OgreOggISound::getBoundingBox(void) const
	{
		static Ogre::AxisAlignedBox aab;
		return aab;
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggISound::getBoundingRadius(void) const
	{
		return 0;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::_updateRenderQueue(Ogre::RenderQueue *queue)
	{
		return;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::_notifyAttached(
		Ogre::Node* node
		#if !AV_OGRE_NEXT_VERSION
		, bool isTagPoint
		#endif
	)
	{
		// Call base class notify
		Ogre::MovableObject::_notifyAttached(
			node
			#if !AV_OGRE_NEXT_VERSION
			, isTagPoint
			#endif
		);

		#if !AV_OGRE_NEXT_VERSION
		// Immediately set position/orientation when attached
		mLocalTransformDirty = true;
		#endif
		
		update(0);

		return;
	}
	/*/////////////////////////////////////////////////////////////////*/
	#if !AV_OGRE_NEXT_VERSION
	void OgreOggISound::_notifyMoved(void) 
	{ 
		// Call base class notify
		Ogre::MovableObject::_notifyMoved();

		mLocalTransformDirty=true; 
	}
	#else
	void OgreOggISound::_updateRenderQueue(Ogre::RenderQueue *queue, Ogre::Camera *camera, const Ogre::Camera *lodCamera) {
	}
	#endif
	#if !AV_OGRE_NEXT_VERSION
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggISound::visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables)
	{
		return;
	}
	#endif
}
