#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "Constraint.h"
#include "ForceIdAttractor.h"
#include "ForceRepulsion.h"
#include "EmitterController.h"

using namespace ci;
using std::vector;
using std::shared_ptr;

EmitterController::EmitterController()
{
}

void EmitterController::createConstraints( const Vec2f &windowDim )
{
	mConstraints.clear();
	mConstraints.push_back( shared_ptr< Constraint >(
				new Constraint( Vec3f( 1.f, 1.f, 0.f ), Vec3f::zero(), Vec3f( windowDim, 0.f ) ) ) );
}

void EmitterController::update( int counter )
{
	for ( int i = 0; i < counter; i++ )
	{
		// update Forces
		for ( auto &forceKV: mForces )
		{
			forceKV.second->apply( mEmitters );
		}

		// update Emitters
		for ( auto emitterRef : mEmitters )
		{
			emitterRef->move();
			emitterRef->update();
		}

		// apply Constraints
		for ( auto contrainRef : mConstraints )
		{
			contrainRef->apply( mEmitters );
		}
	}
}

void EmitterController::render()
{
	renderEmitters();
}

void EmitterController::renderEmitters()
{
	for ( auto emitterRef : mEmitters )
	{
		emitterRef->render();
	}
}

void EmitterController::addEmitter( ci::Vec3f loc, ci::Vec3f vel )
{
	mEmitters.push_back( shared_ptr< Emitter >( new Emitter( loc, vel ) ) );
}

uint32_t EmitterController::addForceRepulsion( float mag )
{
	mForces[ mCurrentForceId ] = shared_ptr< ForceRepulsion >( new ForceRepulsion( mag ) );
	return mCurrentForceId++;
}

uint32_t EmitterController::addForceIdAttractor( float mag, const ci::Vec3f &loc, uint32_t id )
{
	mForces[ mCurrentForceId ] = shared_ptr< ForceIdAttractor >( new ForceIdAttractor( mag, loc, id ) );
	return mCurrentForceId++;
}

void EmitterController::removeForce( uint32_t forceId )
{
	mForces.erase( forceId );
}
