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
* DESCRIPTION: Base class for a single sound
*/
#pragma once

#include "OgreOggSoundPrereqs.h"
#include <string>
#include <deque>
#include <vorbis/vorbisfile.h>
#include "OgreOggSoundCallback.h"
	
/**
 * Number of buffers to use for streaming
 */
#define NUM_BUFFERS 4

namespace OgreOggSound
{
	//! Action to perform after a fade has completed.
	/** 
	@remarks
		Use this to specify what to do on a sound after it has finished fading.	i.e. after fading out pause.
	*/
	enum FadeControl
	{
		FC_NONE		= 0x00,
		FC_PAUSE	= 0x01,
		FC_STOP		= 0x02
	};

	//! The current state of the sound.
	/** 
	@remarks
		This is separate from what OpenAL thinks the current state of the sound is. A separate state is maintained in order to make
		sure the correct state is available when using multi-threaded sound streaming since the OpenAL sound is stopped and started
		multiple times while it is still technically in a "playing" state.
	*/
	enum SoundState
	{
		SS_NONE,
		SS_PLAYING,
		SS_PAUSED,
		SS_STOPPED,
		SS_DESTROYED
	};

	//!Structure describing an ogg stream
	struct SOggFile
	{
		char* dataPtr;    // Pointer to the data in memory
		int   dataSize;   // Size of the data
		int   dataRead;   // How much data we have read so far
	};


	//! A single sound object
	/** provides functions for setting audio properties
	 *	on a 3D sound as well as stop/pause/play operations.
	 */
	class _OGGSOUND_EXPORT OgreOggISound : public Ogre::MovableObject
	{

	public:

		//! Listener callback
		/** provides hooks into various sound states.
		*/	
		class _OGGSOUND_EXPORT SoundListener
		{
		public:

			/** Constructor
			*/
			SoundListener(){}
			/** Destructor
			*/
			virtual ~SoundListener(){}
			/** Called when sound data has been loaded
			*/
			virtual void soundLoaded(OgreOggISound* sound) {}
			/** Called when sound is about to be destroyed
			*/
			virtual void soundDestroyed(OgreOggISound* sound) {}
			/** Called when sound is about to play
			*/
			virtual void soundPlayed(OgreOggISound* sound) {}		   
			/** Called when sound is stopped
			*/
			virtual void soundStopped(OgreOggISound* sound) {}
			/** Called when sound has finished playing in its entirety
			*/
			virtual void soundFinished(OgreOggISound* sound) {}
			/** Called when sound is paused
			*/
			virtual void soundPaused(OgreOggISound* sound) {}
			/** Called when sound loops
			*/
			virtual void soundLooping(OgreOggISound* sound) {}

		};
	
		/** Plays sound.
		 */
		void play(bool immediate=false);
		/** Pauses sound.
		 */
		void pause(bool immediate=false);
		/** Stops sound.
		 */
		void stop(bool immediate=false);
		/** Sets the loop status.
		@remarks
			Sets wheter the sound should loop
			@param loop
				If true, then sound will loop
		 */
		virtual void loop(bool loop) = 0;
		/** Gets looping status.
		@remarks
			Gets whether the looping status is enabled for this sound
		 */
		inline bool isLooping() { return mLoop; }
		/** Sets the start point of a loopable section of audio.
		@remarks
			Allows user to define any start point for a loopable sound, by default this would be 0, 
			or the entire audio data, but this function can be used to offset the start of the loop. 
			@param startTime
				Position in seconds to offset the loop point.
		@note
			The sound will start playback from the beginning of the audio data but upon looping, 
			if set, it will loop back to the new offset position.
		@note
			(Streamed sounds ONLY)
		 */
		virtual void setLoopOffset(float startTime) {}
		/** Gets the start point of a loopable section of audio in seconds.
		 */
		inline float getLoopOffset() { return mLoopOffset; }
		/** Sets the source object for playback.
		@remarks
			Abstract function
		 */
		virtual void setSource(ALuint src) = 0;
		/** Starts a fade in/out of the sound volume
		@remarks
			Triggers a fade in/out of the sounds volume over time. 
			Uses the current volume as the initial level to fade from, 
			then either 0 or mMaxGain will be used as the fade to level.

			@param dir 
				Direction to fade. (true=in | false=out)
			@param fadeTime 
				Time over which to fade (>0)
			@param actionOnCompletion 
				Optional action to perform when fading has finished (default: NONE)
		*/
		void startFade(bool dir, float fadeTime, FadeControl actionOnCompletion=OgreOggSound::FC_NONE);
		/** Returns whether this sound is temporary
		 */
		inline bool isTemporary() const { return mTemporary; }
		/** Returns whether this sound is mono
		 */
		virtual bool isMono()=0;
		/** Returns the buffer sample rate
		 */
		virtual unsigned int getSampleRate()=0;
		/** Returns the buffer number of channels
		 */
		virtual unsigned short getChannels()=0;
		/** Returns the buffer bits per sample
		@note
			In the case of OGG files this is the sample rate divided by the bitrate
		 */
		virtual unsigned int getBitsPerSample()=0;
		/** Marks sound as temporary
		@remarks
			Auto-destroys itself after finishing playing.
		*/
		inline void markTemporary() { mTemporary=true; }
		/** Allows switchable spatialisation for this sound.
		@remarks
			Switch's spatialisation on/off for mono sounds, no-effect for stereo sounds.
		@note
			If disabling spatialisation, reference distance is set to 1 and Positon is set to ZERO, 
			so may need resetting should spatialisation be re-enabled later.\n
			Note also that node inherited positioning/orientation is disabled in this mode,
			however manual positioning/orientation is still available allowing some control over speaker output.
		 */
		void disable3D(bool disable);
		/** Queries switchable spatialisation for this sound.
		@remarks
			Get whether spatialisation is on/off for mono sounds (for stereo sounds the effect does not apply).
		 */
		bool is3Ddisabled();
		/** Sets the position of the playback cursor in seconds
		@param seconds
			Play position in seconds 
		 */
		virtual void setPlayPosition(float seconds);
		/** Gets the position of the playback cursor in seconds
		 */
		virtual float getPlayPosition() const;
		/** Gets the current state the sound is in
		 */
		inline SoundState getState() const { return mState; }
		/** Returns play status.
		@remarks
			Checks for a valid source before checking the state value
		 */
		inline bool isPlaying() const { return mSource != AL_NONE && mState == SS_PLAYING; }
		/** Returns pause status.
		@remarks
			Checks for a valid source before checking the state value
		 */
		inline bool isPaused() const { return mSource != AL_NONE && mState == SS_PAUSED; }
		/** Returns stop status.
		@remarks
			Checks for a valid source before checking the state value
		 */
		inline bool isStopped() const { return mSource != AL_NONE && mState == SS_STOPPED; }
		/** Returns position status.
		@remarks
			Returns whether position is local to listener or in world-space
		 */
		inline bool isRelativeToListener() const { return mSourceRelative; }
		/** Sets whether source is given up when stopped.
		@remarks
			This flag indicates that the sound should immediately give up its source if finished playing or manually stopped.
			Useful for infrequent sounds or sounds which only play once.
			Allows other sounds immediate access to a playable source object.
			@param giveup 
				If true, releases source immediately (default: false)
		 */
		inline void setGiveUpSourceOnStop(bool giveup=false) { mGiveUpSource=giveup; }
		/** Sets the sounds velocity.
		@remarks
			Sets the 3D velocity of the sound.
		@note
			Even if attached to a SceneNode this will *NOT* automatically be handled for you, 
			unlike the position and direction
			@param velx
				x velocity
			@param vely
				y velocity
			@param velz
				z velocity
		 */
		void setVelocity(float velx, float vely, float velz) { setVelocity(Ogre::Vector3(velx,vely,velz)); }
		/** Sets the sounds velocity.
		@remarks
			Sets the 3D velocity of the sound.
		@note
			Even if attached to a SceneNode this will *NOT* automatically be handled for you, 
			unlike the position and direction
			@param vel
				3D vector velocity
		 */
		void setVelocity(const Ogre::Vector3 &vel);
		/** Sets sounds volume
		@remarks
			Sets sounds current gain value (0..1).
			@param gain
				volume scalar.
		 */
		void setVolume(float gain);
		/** Gets sounds volume
		@remarks
			Gets the current gain value.
		 */
		float getVolume() const;
		/** Sets sounds maximum attenuation volume
		@remarks
			This value sets the maximum volume level of the sound when closest to the listener.
			@param maxGain
				Volume scalar (0..1)
		 */
		void setMaxVolume(float maxGain);
		/** Gets sounds maximum attenuation volume
		@remarks
			This function gets the maximum volume level of the sound when closest to the listener.
		 */
		float getMaxVolume();
		/** Sets sounds minimum attenuation volume
		@remarks
			This value sets the minimum volume level of the sound when furthest away from the listener.
			@param minGain
				Volume scalar (0..1)
		 */
		void setMinVolume(float minGain);
		/** Gets sounds minimum attenuation volume
		@remarks
			This function gets the minimum volume level of the sound when furthest away from the listener.
		 */
		float getMinVolume();
		/** Sets sounds cone angles
		@remarks
			This value sets the angles of the sound cone used by this sound.
			@param insideAngle 
				Angle over which the volume is at maximum (in degrees)
			@param outsideAngle 
				Angle over which the volume is at minimum (in degrees)
		 */
		void setConeAngles(float insideAngle=360.f, float outsideAngle=360.f);
		/** Gets sounds cone inside angle
		@remarks
			This function gets the inside angle (in degrees) of the sound cone used by this sound.
		 */
		float getConeInsideAngle();
		/** Gets sounds cone outside angle
		@remarks
			This function gets the outside angle (in degrees) of the sound cone used by this sound.
		 */
		float getConeOutsideAngle();
		/** Sets sounds outer cone volume
		@remarks
			This value sets the volume level heard at the outer cone angle.
			Usually 0 so no sound heard when not within sound cone.
			@param gain
				Volume scalar (0..1)
		 */
		void setOuterConeVolume(float gain=0.f);
		/** Gets sounds outer cone volume
		@remarks
			This function gets the volume level heard at the outer cone angle.
		 */
		float getOuterConeVolume();
		/** Sets sounds maximum distance
		@remarks
			This value sets the maximum distance at which attenuation is stopped.
			Beyond this distance the volume remains constant.
			@param maxDistance 
				Distance.
		*/
		void setMaxDistance(float maxDistance);
		/** Gets sounds maximum distance
		@remarks
			Returns -1 on error
		*/
		const float getMaxDistance() const;
		/** Sets sounds rolloff factor
		@remarks
			This value sets the rolloff factor applied to the attenuation of the volume over distance.
			Effectively scales the volume change affect.
			@param rolloffFactor
				Factor (>0).
		*/
		void setRolloffFactor(float rolloffFactor);
		/** Gets sounds rolloff factor
		@remarks
			Returns -1 on error
		*/
		const float getRolloffFactor() const;
		/** Sets sounds reference distance
		@remarks
			This value sets the half-volume distance.
			The distance at which the volume would be reduced by half.
			@param referenceDistance 
				Distance (>0).
		*/
		void setReferenceDistance(float referenceDistance);
		/** Gets sounds reference distance
		@remarks
			Returns -1 on error
		*/
		const float getReferenceDistance() const;
		/** Sets sounds pitch
		@remarks
			This affects the playback speed of the sound
			@param pitch 
				Pitch (>0).
		*/
		void setPitch(float pitch);	
		/** Gets sounds pitch
		@remarks
			Returns -1 on error
		*/
		const float getPitch() const;	
		/** Sets whether the positional information is relative to the listener
		@remarks
			This specifies whether the sound is attached to listener or in world-space.
			Default: world-space
			@param relative 
				Boolean yes/no.
		*/
		void setRelativeToListener(bool relative);
		/** Gets the sounds velocity
		 */
		inline const Ogre::Vector3& getVelocity() const {return mVelocity;}
		/** Returns fade status.
		 */
		inline bool isFading() const { return mFade; }
		/** Updates sund
		@remarks
			Updates sounds position, buffers and state
			@param fTime
				Elapsed frametime.
		*/
		virtual void update(float fTime);
		/** Gets the sounds source
		 */
		inline ALuint getSource() const { return mSource; }
		/** Gets the sounds file name
		 */
		virtual const Ogre::String& getFileName( void ) const { return mAudioStream ? Ogre::BLANKSTRING : mAudioStream->getName(); }
		/** Gets the sounds priority
		 */
		inline Ogre::uint8 getPriority() const { return mPriority; }
		/** Sets the sounds priority
		@remarks
			This can be used to specify a priority to the sound which will be checked when re-using sources.
			Higher priorities will tend to keep their sources.
			@param priority 
				Priority (between 0..255)
		 */
		inline void setPriority(Ogre::uint8 priority) { mPriority=priority; }
		/** Adds a time position in a sound as a cue point
		@remarks
			Allows the setting of a 'jump-to' point within an audio file.
			Returns true on success.
			@param seconds
				Cue point in seconds 
		 */
		bool addCuePoint(float seconds);
		/** Removes a cue point
		 */
		void removeCuePoint(unsigned short index);
		/** Clears entire list of cue points
		 */
		inline void clearCuePoints() { mCuePoints.clear(); }
		/** Shifts the play position to a previously set cue point position.
		@param index
			Position in cue point list to apply
		 */
		void setCuePoint(unsigned short index);
		/** Gets a previously set cue point by index
		@param index
			Position in cue point list to get
		 */
		float getCuePoint(unsigned short index);
		/** Returns number of cue points
		 */
		inline unsigned int getNumCuePoints() { return static_cast<int>(mCuePoints.size()); }
		/** Gets the length of the audio file in seconds
		@remarks
			Only valid after file has been opened AND file is seekable.
		 */
		inline float getAudioLength() const { return mPlayTime; }
		/** Gets movable type string
		@remarks
			Overridden from MovableObject.
		 */
		virtual const Ogre::String& getMovableType(void) const;
		/** Gets bounding box
		@remarks
			Overridden from MovableObject.
		 */
		virtual const Ogre::AxisAlignedBox& getBoundingBox(void) const;
		/** Gets bounding radius
		@remarks
			Overridden from MovableObject.
		 */
		virtual float getBoundingRadius(void) const;

		/** Sets a listener object to be notified of events.
		@remarks
			Allows state changes to be signaled to an interested party.
			@param listener
				Listener object pointer.
		*/
		inline void setListener(SoundListener* listener) { mSoundListener = listener; }

		/** Sets properties of a shared resource.
		@remarks
			Sets a number of properties relating to audio of a shared resource.
			@param buffer
				Pointer to the shared buffer to copy the properties from.
		*/
		void _setSharedProperties(sharedAudioBuffer* buffer);
	
		/** Gets properties of a shared resource.
		@remarks
			Gets a number of properties relating to audio of a shared resource.
			@param buffers
				Reference to an array of OpenAL buffers
			@param length
				Reference to an object to store audio length
			@param format
				Reference to an object to store buffer format
		*/
		void _getSharedProperties(BufferListPtr& buffers, float& length, ALenum& format); 
	
		#if AV_OGRE_NEXT_VERSION >= 0x20000
		/** Gets name
		 */
		virtual Ogre::String getName();
		#endif

	protected:

		/** Superclass describing a single sound object.
		@param name
			Name of sound within manager
		@param scnMgr
			SceneManager which created this sound (if the sound was created through the plugin method createMovableobject()).
		 */
		OgreOggISound(
			const Ogre::String& name
			#if AV_OGRE_NEXT_VERSION >= 0x20000
			, Ogre::SceneManager* scnMgr, Ogre::IdType id, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
			#endif
		);
		/** Superclass destructor.
		 */
		virtual ~OgreOggISound();
		/** Open implementation.
		@remarks
			Abstract function
		 */
		virtual void _openImpl(Ogre::DataStreamPtr& fileStream) = 0;
		/** Open implementation.
		@remarks
			Abstract function
			Optional opening function for (Static sounds only)
		 */
		virtual void _openImpl(const Ogre::String& fName, sharedAudioBuffer* buffer=0) {};
		/** Play implementation.
		@remarks
			Abstract function
		 */
		virtual void _playImpl() = 0;
		/** Pause implementation.
		@remarks
			Abstract function
		 */
		virtual void _pauseImpl() = 0;
		/** Stop implementation.
		@remarks
			Abstract function
		 */
		virtual void _stopImpl() = 0;
		/** Release implemenation
		@remarks
			Cleans up buffers and prepares sound for destruction.
		 */
		virtual void _release() = 0;
		/** Updates RenderQueue
		@remarks
			Overridden from MovableObject.
		 */
		virtual void _updateRenderQueue(Ogre::RenderQueue *queue);
		/** Notifys object its been attached to a node
		@remarks
			Overridden from MovableObject.
		 */
		virtual void _notifyAttached(
			Ogre::Node* node
			#if !AV_OGRE_NEXT_VERSION
			, bool isTagPoint = false
			#endif
		);
		#if !AV_OGRE_NEXT_VERSION
		/** Notifys object its been moved
		@remarks
			Overridden from MovableObject.
		 */
		virtual void _notifyMoved(void);
		#else
		/** Does nothing, but needed for being derived from MovableObject
		 */
		virtual void _updateRenderQueue(Ogre::RenderQueue *queue, Ogre::Camera *camera, const Ogre::Camera *lodCamera);
		#endif
		#if !AV_OGRE_NEXT_VERSION
		/** Renderable callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables);
		#endif

		/** Inits source object
		@remarks
			Initialises all the source objects states ready for playback.
		 */
		void _initSource();
		/** Stores the current play position of the sound
		@remarks
			Only for static sounds at present so that when re-activated it begins 
			exactly where it left off.
		 */
		void _markPlayPosition();
		/** Sets the previous play position of the sound
		@remarks
			Uses a previously stored play position to ensure sound playback
			starts where it left off
		 */
		void _recoverPlayPosition();
		/** Updates a fade
		@remarks
			Updates a fade action.
		 */
		void _updateFade(float fTime=0.f);
		/** Updates audio buffers 
		@remarks
			Abstract function.
		*/
		virtual void _updateAudioBuffers() = 0;
		/** Prefills buffers with audio data.
		@remarks
			Loads audio data from the stream into the predefined data
			buffers and queues them onto the source ready for playback.
		 */
		virtual void _prebuffer() = 0;		
		/** Calculates buffer size and format.
		@remarks
			Calculates a block aligned buffer size of 250ms using
			sound properties.
		 */
		virtual bool _queryBufferInfo() = 0;		
		
		/**
		 * Variables used to fade sound
		 */
		float mFadeTimer;
		float mFadeTime;
		float mFadeInitVol;
		float mFadeEndVol;
		bool mFade;
		FadeControl mFadeEndAction;

		// Ogre resource stream pointer
		Ogre::DataStreamPtr mAudioStream;
		ov_callbacks mOggCallbacks;

		SoundListener* mSoundListener;	// Callback object
		size_t mBufferSize;				// Size of audio buffer (250ms)

		/** Sound properties 
		 */
		ALuint mSource;					// OpenAL Source
		Ogre::uint8 mPriority;			// Priority assigned to source 
		Ogre::Vector3 mVelocity;		// 3D velocity
		float mGain;					// Current volume
		float mMaxGain;					// Minimum volume
		float mMinGain;					// Maximum volume
		float mMaxDistance;				// Maximum attenuation distance
		float mRolloffFactor;			// Rolloff factor for attenuation
		float mReferenceDistance;		// Half-volume distance for attenuation
		float mPitch;					// Current pitch 
		float mOuterConeGain;			// Outer cone volume
		float mInnerConeAngle;			// Inner cone angle
		float mOuterConeAngle;			// outer cone angle
		float mPlayTime;				// Time in seconds of sound file
		SoundState mState;				// Sound state
		bool mLoop;						// Loop status
		bool mDisable3D;				// 3D status
		bool mGiveUpSource;				// Flag to indicate whether sound should release its source when stopped
		bool mStream;					// Stream flag
		bool mSourceRelative;			// Relative position flag
		#if !AV_OGRE_NEXT_VERSION
		bool mLocalTransformDirty;		// Transformation update flag
		#else
		Ogre::Vector3 mPosition;		// 3D position
		Ogre::Vector3 mDirection;		// 3D direction
		Ogre::String mName;				// Sound Name (Ogre-Next don't internal store real name for movable objects)
		#endif
		bool mPlayPosChanged;			// Flag indicating playback position has changed
		bool mSeekable;					// Flag indicating seeking available
		bool mTemporary;				// Flag indicating sound is temporary
		bool mInitialised;				// Flag indicating sound is initailised
		Ogre::uint8 mAwaitingDestruction; // Imminent destruction flag
	
		BufferListPtr mBuffers;			// Audio buffer(s)
		ALenum mFormat;					// OpenAL format

		unsigned long mAudioOffset;		// offset to audio data
		unsigned long mAudioEnd;		// offset to end of audio data
		float mLoopOffset;				// offset to start of loop point		 
		float mLoopStart;				// offset in seconds to start of loopable audio data

		ALfloat mPlayPos;				// Playback position in seconds
		std::deque<float> mCuePoints;	// List of play position points

#if OGGSOUND_THREADED
		/** Returns flag indicating an imminent destruction call
		@remarks
			Multi-threaded calls are delayed, therefore its possible to cue a destruction 
			then subsequently request a handle to the same sound object, which would yield 
			a valid pointer but could potentially invalidate itself at any moment...
			For now this flag will be used to assess the validation a subsequent get() call 
			to prevent, as much as possible, this occurence.
		*/
		inline bool _isDestroying() const { return mAwaitingDestruction!=0; }

		/** Sets flag indicating an imminent destruction call
		@remarks
			Multi-threaded calls are delayed, therefore its possible to cue a destruction 
			then subsequently request a handle to the same sound object, which would yield 
			a valid pointer but could potentially invalidate itself at any moment...
			For now this flag will be used to assess the validation of a subsequent getSound() call 
			to prevent, as much as possible, this occurence.
		*/
		inline void _notifyDestroying()  { mAwaitingDestruction=2; }
#endif

		friend class OgreOggSoundManager;
	};
}															  
