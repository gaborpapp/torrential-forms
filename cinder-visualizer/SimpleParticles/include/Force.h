#pragma once

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Vector.h"

#include "Emitter.h"

class Force
{
	public:
		Force( float mag ) : mMagnitude( mag ) {};

		virtual void update() {};
		virtual void apply( std::vector< EmitterRef > &emitters ) {};

		float mMagnitude;
};

typedef std::shared_ptr< Force > ForceRef;

