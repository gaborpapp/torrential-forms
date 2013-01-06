#pragma once

#include <vector>

#include "cinder/Vector.h"

#include "Force.h"
#include "Emitter.h"

class ForceRepulsion : public Force
{
	public:
		ForceRepulsion( float aMagnitude ) : Force( aMagnitude ) {}

		void apply( std::vector< EmitterRef > &emitters );
};

