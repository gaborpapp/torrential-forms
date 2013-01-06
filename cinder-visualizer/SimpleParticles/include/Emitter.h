#pragma once

#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/Vector.h"

class Emitter
{
	public:
		Emitter( ci::Vec3f aLoc, ci::Vec3f aVel );

		void move();
		void update();
		void render();

		void setRadius( float r ) { mRadius = r; mMass = r * r * M_PI; mInvMass = 1.f / mMass; }

		ci::Vec3f mLoc;
		ci::Vec3f mVel;
		ci::Vec3f mAcc;

		float mRadius;
		float mRadiusOrig;
		float mMass;
		float mInvMass;
		float mCharge;
};

typedef std::shared_ptr< Emitter > EmitterRef;

