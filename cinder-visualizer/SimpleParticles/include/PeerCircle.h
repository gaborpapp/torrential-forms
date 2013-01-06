#pragma once

#include <string>

#include "cinder/Rect.h"

#include "Visualizer.h"

namespace tf {

class PeerCircle : public tf::Peer
{
	public:
		PeerCircle( int id, std::string address, float bearing, std::string location, TorrentRef tr );

		void update();
		void draw( const ci::Rectf &rect );

		float getRadius() const { return mRadius; }
		void setRadius( float r ) { mRadius = r; }

		void setMass( float m ) { mMass = m; mInvMass = 1.f / m; }
		float getMass() const { return mMass; }
		float getInvMass() const { return mInvMass; }

		const ci::Vec2f &getPos() const { return mPos; }
		void accelerate( const ci::Vec2f &acc ) { mAcc += acc; }

		void setAttractorPos( const ci::Vec2f &pos ) { mAttractorPos = pos; }

		void moveBy( ci::Vec2f offset, bool preserveVelocity )
		{
			mPos += offset;
			if ( preserveVelocity )
				mOldPos += offset;
		}

	private:
		//void init();
		ci::Vec2f mPos;
		ci::Vec2f mOldPos;

		float mMass = 1.f;
		float mInvMass = 1.f;
		float mDrag = 1.f;

		float mRadius = 5.f;
		float mRadiusOrig = 5.f;

		ci::Vec2f mVel = ci::Vec2f( 0.f, 0.f );
		ci::Vec2f mAcc = ci::Vec2f( 0.f, 0.f );
		ci::Vec2f mAttractorPos;

		static const ci::Rectf sWorldRect;
};

class PeerCircleFactory : public PeerFactory
{
	public:
		PeerRef createPeer( int id, std::string address, float bearing, std::string location,
				TorrentRef tr )
		{
			return PeerRef( new PeerCircle( id, address, bearing, location, tr ) );
		}
};

} // namespace tf

