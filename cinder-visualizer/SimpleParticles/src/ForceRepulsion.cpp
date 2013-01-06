#include "ForceRepulsion.h"

using namespace ci;
using std::vector;

void ForceRepulsion::apply( std::vector< EmitterRef > &emitters )
{
	for ( auto pit0 = emitters.begin(); pit0 != emitters.end(); ++pit0 )
	{
		EmitterRef p0 = *pit0;

		for ( auto pit1 = pit0; pit1 != emitters.end(); ++pit1 )
		{
			EmitterRef p1 = *pit1;
			if ( p0 != p1 )
			{
				Vec3f dir = p0->mLoc - p1->mLoc;
				float distSqrd = dir.lengthSquared();
				float radiusSum = ( p0->mRadius + p1->mRadius ) * 1.5f;
				float radiusSqrd = radiusSum * radiusSum;

				if ( distSqrd < radiusSqrd && distSqrd > .1f )
				{
					float per = 1.f - distSqrd / radiusSqrd;
					float E = p0->mMass * p1->mMass * p0->mCharge * p1->mCharge / distSqrd;
					float F = E;

					if ( F > 50.0f )
						F = 50.0f;

					dir.normalize();
					dir *= F * per * mMagnitude;

					p0->mAcc += dir * p0->mInvMass;
					p1->mAcc -= dir * p1->mInvMass;
				}
			}
		}
	}
}
