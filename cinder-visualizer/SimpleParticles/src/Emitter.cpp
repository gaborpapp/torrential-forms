#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "Emitter.h"
#include "GlobalSettings.h"

using namespace ci;

Emitter::Emitter( Vec3f aLoc, Vec3f aVel )
{
	mLoc = aLoc;
	mVel = aVel;
	mAcc = Vec3f::zero();

	setRadius( tf::GlobalSettings::get().mEmitterRadiusMin );
	mCharge = Rand::randFloat( 0.35f, 0.75f );
}

void Emitter::move()
{
	mVel += mAcc;
	mLoc += mVel;
}

void Emitter::update()
{
	tf::GlobalSettings &settings = tf::GlobalSettings::get();
	mVel *= 0.975f;
	mAcc.set( 0, 0, 0 );
	float radiusOrig = settings.mEmitterRadiusMin;
	mRadius = radiusOrig + ( mRadius - radiusOrig ) *
		settings.mEmitterRadiusDamping;
}

void Emitter::render()
{
	gl::color( Color::white() );
	gl::drawStrokedCircle( Vec2f( mLoc.xy() ), mRadius );
}
