#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Cinder.h"
#include "cinder/Color.h"
#include "cinder/Rand.h"

#include "PeerCircle.h"

using namespace ci;

namespace tf {

const Rectf PeerCircle::sWorldRect = Rectf( 0.f, 0.f, 1000.f, 1000.f );

PeerCircle::PeerCircle( int id, std::string address, float bearing, std::string location, TorrentRef tr ) :
	Peer( id, address, bearing, location, tr)
{
	Vec2f bv = sWorldRect.getSize() * Vec2f( .3f, .0f );
	bv.rotate( bearing );
	mOldPos = mPos = sWorldRect.getCenter() + bv;
	//mDecay = Rand::randFloat( .95f, .99f );
	mAttractorPos = sWorldRect.getCenter();
}

void PeerCircle::update()
{
	// attraction to torrent position
	Vec2f dir = mAttractorPos - mPos;
	float distSqrd = dir.lengthSquared();

	float strength = 5.f;

	if ( distSqrd > 0.f )
	{
		float minDistSqrd = 1000.f;
		if ( distSqrd < minDistSqrd )
		{
			strength = lmap< float >( 1.f, minDistSqrd, 0.f, strength, distSqrd );
			strength = math< float >::clamp( strength, 0.f, 5.f );
		}
		float force = strength * mMass / distSqrd;
		Vec2f deltaForce = dir * force;
		moveBy( deltaForce * mInvMass, false );
	}

	// repel other peers
	const std::vector< PeerRef > &peers = mTorrentRef->getPeers();
	for ( PeerRef peer : peers )
	{
		if ( peer->getId() == mId )
			continue;

		auto other = std::static_pointer_cast< PeerCircle >( peer );
		Vec2d dir = mPos - other->getPos();
		float distSqrd = dir.lengthSquared();
		if ( distSqrd > .0f )
		{
			float force = .1f * mMass * other->getMass() / distSqrd;
			Vec2f deltaForce = dir * force;
			moveBy( deltaForce * mInvMass, false );
			other->moveBy( - deltaForce * other->getInvMass(), false );
		}
	}

	// do verlet
	Vec2f curPos = mPos;
	Vec2f vel = mPos - mOldPos;
	mPos += vel * mDrag;
	mOldPos = curPos;

	/*
	// TODO: attract actual torrent position
	Vec2d dir = Vec2d( 500, 500 ) - mPos;
	double distSqrd = dir.lengthSquared();

	if ( distSqrd > 0. )
	{
		double f = math< double >::sqrt( distSqrd ) * .00001;
		mAcc += dir.normalized() * f;
	}

	mVel += mAcc;
	mPos += mVel;
	mVel *= mDecay;
	mAcc.set( 0., 0. );
	//mRadius = lerp< double, double >( mRadius, mRadiusOrig, .01 );
	*/
}

void PeerCircle::draw( const Rectf &rect )
{
	RectMapping rm( sWorldRect, rect, true );
	//app::console() << mId << ": " << mRadius << std::endl;

	gl::color( Color::white() );
	gl::drawStrokedCircle( rm.map( mPos ),
			lmap< float >( 0.f, sWorldRect.getHeight(), 0.f, rect.getHeight(), mRadius ) );
}

} // namespace tf

