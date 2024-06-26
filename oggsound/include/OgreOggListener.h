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
* DESCRIPTION: Listener object (The users 'ears')
*/

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <Ogre.h>
#include <OgreMovableObject.h>

namespace OgreOggSound
{
	//! Listener object (Users ears)
	/** Handles properties associated with the listener.
	*/
	class  _OGGSOUND_EXPORT OgreOggListener : public Ogre::MovableObject
	{

	public:		

		/** Constructor
		@remarks
			Creates a listener object to act as the ears of the user. 
		 */
		OgreOggListener(
			#if AV_OGRE_NEXT_VERSION >= 0x20000
			Ogre::IdType id, Ogre::SceneManager *scnMgr, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
			#else
			Ogre::SceneManager* scnMgr = NULL
			#endif
		): 
			#if AV_OGRE_NEXT_VERSION >= 0x20100
				MovableObject(id, objMemMgr, scnMgr, renderQueueId),
			#endif
			  mVelocity(Ogre::Vector3::ZERO)
			#if !AV_OGRE_NEXT_VERSION
			, mLocalTransformDirty(false)
			#endif
			, mSceneMgr(scnMgr)
		{
			for (int i=0; i<6; ++i ) mOrientation[i]=0.f;
			mName = "OgreOggListener";
			#if AV_OGRE_NEXT_VERSION >= 0x20000
			setLocalAabb(Ogre::Aabb::BOX_NULL);
			setQueryFlags(0);
			#endif
		};
		/** Sets sounds velocity.
		@param
			velx/vely/velz componentes of 3D vector velocity
		 */
		void setVelocity(float velx, float vely, float velz) { setVelocity(Ogre::Vector3(velx,vely,velz)); }
		/** Sets sounds velocity.
		@param vel
			3D vector velocity
		 */
		void setVelocity(const Ogre::Vector3 &vel);
		/** Updates the listener.
		@remarks
			Handles positional updates to the listener either automatically
			through the SceneGraph attachment or manually using the 
			provided functions.
		 */
		void update();
		/** Gets the movable type string for this object.
		@remarks
			Overridden function from MovableObject, returns a 
			Sound object string for identification.
		 */
		virtual const Ogre::String& getMovableType(void) const;
		/** Gets the bounding box of this object.
		@remarks
			Overridden function from MovableObject, provides a
			bounding box for this object.
		 */
		virtual const Ogre::AxisAlignedBox& getBoundingBox(void) const;
		/** Gets the bounding radius of this object.
		@remarks
			Overridden function from MovableObject, provides the
			bounding radius for this object.
		 */
		virtual float getBoundingRadius(void) const;
		#if !AV_OGRE_NEXT_VERSION
		void _updateRenderQueue(Ogre::RenderQueue *queue) override {}
		void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables) override {}
		#else
		void _updateRenderQueue(Ogre::RenderQueue *queue) {}
		#endif
		/** Attach callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void _notifyAttached(
			Ogre::Node* node
			#if !AV_OGRE_NEXT_VERSION
			, bool isTagPoint = false
			#endif
		);
		#if !AV_OGRE_NEXT_VERSION
		/** Moved callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void _notifyMoved(void);
		#else
		/** Does nothing but is need for being derived from MovableObject
		 */
		virtual void _updateRenderQueue(Ogre::RenderQueue *queue, Ogre::Camera *camera, const Ogre::Camera *lodCamera) {}
		#endif
		/** Returns scenemanager which created this listener.
		 */
		Ogre::SceneManager* getSceneManager() { return mSceneMgr; }
		#if !AV_OGRE_NEXT_VERSION
		/** Sets scenemanager which created this listener.
		 */
		void setSceneManager(Ogre::SceneManager& m) { mSceneMgr=&m; }
		#endif
	private:

#if OGGSOUND_THREADED
		OGRE_WQ_MUTEX(mMutex);
#endif

		/**
		 * Positional variables
		 */
		Ogre::Vector3 mVelocity;		// 3D velocity
		float mOrientation[6];			// 3D orientation
		#if AV_OGRE_NEXT_VERSION >= 0x20000
		Ogre::Vector3 mPosition;		// 3D position
		Ogre::Quaternion mOrient;		// 3D orientation as Quaternion
		#else
		bool mLocalTransformDirty;		// Dirty transforms flag
		#endif
		Ogre::SceneManager* mSceneMgr;	// Creator 

	};
}
