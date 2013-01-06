#pragma once

#include <vector>

#include "cinder/Vector.h"

#include "Force.h"
#include "Emitter.h"

class ForceIdAttractor : public Force
{
	public:
		ForceIdAttractor( float aMagnitude, const ci::Vec3f &loc, uint32_t id ) :
			Force( aMagnitude ), mLoc( loc ), mId( id )
		{}

		void apply( std::vector< EmitterRef > &emitters );

		ci::Vec3f mLoc;
		uint32_t mId;
};

