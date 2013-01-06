#pragma once
#include <map>

#include "cinder/Cinder.h"
#include "cinder/Vector.h"

#include "Constraint.h"
#include "Force.h"
#include "Emitter.h"

class EmitterController
{
	public:
		EmitterController();

		void update( int counter = 1 );

		void render();
		void renderEmitters();

		void addEmitter( ci::Vec3f loc, ci::Vec3f vel );

		void createConstraints( const ci::Vec2f &windowDim );

		uint32_t addForceRepulsion( float mag );
		uint32_t addForceIdAttractor( float mag, float dur, const ci::Vec3f &loc, uint32_t id );
		void removeForce( uint32_t forceId );
		ForceRef getForceRef( uint32_t forceId ) { return mForces[ forceId ]; }

		std::vector< EmitterRef > mEmitters;
		std::map< uint32_t, ForceRef > mForces;
		std::vector< std::shared_ptr< Constraint > > mConstraints;

	protected:

		uint32_t mCurrentForceId = 1;
};

