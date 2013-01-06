#pragma once

#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/Vector.h"

class Emitter
{
	public:
		Emitter( uint32_t id, ci::Vec3f loc, ci::Vec3f vel );

		void move();
		void update();
		void render();

		void setRadius( float r ) { mRadius = r; mMass = r * r * M_PI; mInvMass = 1.f / mMass; }

		ci::Vec3f mLoc;
		ci::Vec3f mVel;
		ci::Vec3f mAcc = ci::Vec3f::zero();

		uint32_t mId;
		float mRadius;
		float mRadiusOrig;
		float mMass;
		float mInvMass;
		float mCharge;
};

typedef std::shared_ptr< Emitter > EmitterRef;

