#include "cinder/app/App.h"

#include "ForceIdAttractor.h"
#include "GlobalSettings.h"

using namespace ci;
using std::vector;

void ForceIdAttractor::apply( std::vector< EmitterRef > &emitters )
{
	EmitterRef e = emitters[ mId ];
	Vec3f dir = mLoc - e->mLoc;
	float distSqrd = dir.lengthSquared();

	float radius = tf::GlobalSettings::get().mEmitterAttractionRadius;
	float radiusSqrd = radius * radius;

	if ( distSqrd > .1f )
	{
		// repulsion if inside
		if ( distSqrd < radiusSqrd )
		{
			float per = 1.f - distSqrd / radiusSqrd;
			float E = e->mCharge / distSqrd;
			float F = E * e->mInvMass;

			if ( F > 50.0f )
				F = 50.0f;
			dir.normalize();
			dir *= F * per * 100.f * mMagnitude * mLifeSpan;
			e->mAcc += -dir;
		}
		else // constant attraction if outside
		{
			float F = e->mCharge * e->mInvMass;
			if ( F > 50.0f )
				F = 50.0f;
			dir.normalize();
			dir *= F * mMagnitude * mLifeSpan;
			e->mAcc += dir;
		}

		if ( mLifeSpan == 0.f )
			mMagnitude = 0.f;
	}
}
