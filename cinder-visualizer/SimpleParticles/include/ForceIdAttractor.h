#pragma once

#include <vector>

#include "cinder/app/App.h"
#include "cinder/Timeline.h"
#include "cinder/Vector.h"

#include "Force.h"
#include "Emitter.h"

class ForceIdAttractor : public Force
{
	public:
		ForceIdAttractor( float mag, float duration, const ci::Vec3f &loc, uint32_t id ) :
			Force( mag ), mLoc( loc ), mId( id )
		{
			mLifeSpan = 1.f;
			ci::app::timeline().apply( &mLifeSpan, .0f, duration );
		}

		void apply( std::vector< EmitterRef > &emitters );

		ci::Anim< float > mLifeSpan;
		ci::Vec3f mLoc;
		uint32_t mId;
};

