#include "cinder/Rand.h"

#include "Constraint.h"

using namespace ci;

Constraint::Constraint( const Vec3f &normal, const Vec3f &minValue, const Vec3f &maxValue )
{
	mNormal = normal;
	mMinValue = minValue;
	mMaxValue = maxValue;
}

void Constraint::apply( std::vector< EmitterRef > &emitters )
{
	for ( auto p : emitters )
	{
		float velMulti = Rand::randFloat( -.5f, -.1f );
		Vec3f minLim = mMinValue + Vec3f( p->mRadius, p->mRadius, p->mRadius );
		Vec3f maxLim = mMaxValue - Vec3f( p->mRadius, p->mRadius, p->mRadius );
		if ( mNormal.x > .0f )
		{
			if ( p->mLoc.x < minLim.x )
			{
				p->mLoc.x = minLim.x;
				p->mVel.x *= velMulti;
			}
			else
			if ( p->mLoc.x > maxLim.x ){
				p->mLoc.x = maxLim.x;
				p->mVel.x *= velMulti;
			}
		}

		if ( mNormal.y > .0f )
		{
			if ( p->mLoc.y < minLim.y )
			{
				p->mLoc.y = minLim.y;
				p->mVel.y *= velMulti;
			}
			else
			if ( p->mLoc.y > maxLim.y ){
				p->mLoc.y = maxLim.y;
				p->mVel.y *= velMulti;
			}
		}

		if ( mNormal.z > .0f )
		{
			if ( p->mLoc.z < minLim.z )
			{
				p->mLoc.z = minLim.z;
				p->mVel.z *= velMulti;
			}
			else
			if ( p->mLoc.z > maxLim.z ){
				p->mLoc.z = maxLim.z;
				p->mVel.z *= velMulti;
			}
		}
	}
}
