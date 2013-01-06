#pragma once

#include "cinder/Vector.h"

#include "Emitter.h"

#include <vector>

class Constraint
{
	public:
		Constraint( const ci::Vec3f &normal, const ci::Vec3f &minValue, const ci::Vec3f &maxValue );
		virtual void apply( std::vector< EmitterRef > &emitters );

		ci::Vec3f mNormal;
		ci::Vec3f mMinValue;
		ci::Vec3f mMaxValue;
};
