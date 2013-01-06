/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Exception.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

//#include "PeerCircle.h"
#include "EmitterController.h"
#include "GlobalSettings.h"
#include "PParams.h"
#include "TorrentPuzzle.h"
#include "Visualizer.h"


using namespace ci;
using namespace ci::app;
using namespace std;

namespace tf {

class SimpleParticlesApp : public AppBasic, Visualizer
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void resize();
		void shutdown();

		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );

		void update();
		void draw();

	private:
		params::PInterfaceGl mParams;

		float mFps;

		void torrentReceived( TorrentRef tr );
		void peerReceived( PeerRef cr );
		void chunkReceived( ChunkRef cr );
		void segmentReceived( SegmentRef sr );

		EmitterController mEmitterController;
		uint32_t mForceRepulsionId;
};

void SimpleParticlesApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1200, 800 );
}

void SimpleParticlesApp::setup()
{
	gl::disableVerticalSync();

	int port = Visualizer::getServerPort( getArgs() );
	Visualizer::setTorrentFactory( std::shared_ptr< TorrentFactory >( new TorrentPuzzleFactory() ) );
	//Visualizer::setPeerFactory( std::shared_ptr< PeerFactory >( new PeerCircleFactory() ) );
	Visualizer::setup( "127.0.0.1", port );

	connectTorrentReceived< SimpleParticlesApp >( &SimpleParticlesApp::torrentReceived, this );
	connectPeerReceived< SimpleParticlesApp >( &SimpleParticlesApp::peerReceived, this );
	connectChunkReceived< SimpleParticlesApp >( &SimpleParticlesApp::chunkReceived, this );

	mForceRepulsionId = mEmitterController.addForceRepulsion( 10.f );
	mEmitterController.createConstraints( getWindowSize() );

	params::PInterfaceGl::load( "params.xml" );
	mParams = params::PInterfaceGl( "Parameters", Vec2i( 300, 420 ) );
	mParams.addPersistentSizeAndPosition();
	mParams.addText( "Emitters" );
	mParams.addPersistentParam( "Radius min",
			&GlobalSettings::get().mEmitterRadiusMin, 10.f,
			"min=1 max=50 step=.5" );
	mParams.addPersistentParam( "Radius max",
			&GlobalSettings::get().mEmitterRadiusMax, 50.f,
			"min=1 max=250 step=.5" );
	mParams.addPersistentParam( "Radius increase / chunk",
			&GlobalSettings::get().mEmitterRadiusStep, .1f,
			"min=0 max=5. step=.01" );
	mParams.addPersistentParam( "Radius damping",
			&GlobalSettings::get().mEmitterRadiusDamping, .975f,
			"min=.9 max=1. step=.001" );
	mParams.addPersistentParam( "Repulsion",
			&GlobalSettings::get().mEmitterRepulsion, 10.f,
			"min=0 max=100 step=.5" );
	mParams.addPersistentParam( "Repulsion radius multiplier",
			&GlobalSettings::get().mEmitterRepulsionRadius, 1.5f,
			"min=1 max=20 step=.1" );
	mParams.addPersistentParam( "Attraction radius",
			&GlobalSettings::get().mEmitterAttractionRadius, 50.f,
			"min=0 max=200 step=.5" );
	mParams.addPersistentParam( "Attraction magnitude",
			&GlobalSettings::get().mEmitterAttractionMagnitude, 10.f,
			"min=0 max=100 step=.5" );
	mParams.addPersistentParam( "Attraction duration",
			&GlobalSettings::get().mEmitterAttractionDuration, 2.f,
			"min=0 max=50 step=.2" );

	mParams.addSeparator();
	mParams.addPersistentParam( "Peer labels",
			&GlobalSettings::get().mDebugPeerIds, false );

	mParams.addSeparator();
	mParams.addParam( "Fps", &mFps, "", true );
}

void SimpleParticlesApp::resize()
{
	mEmitterController.createConstraints( getWindowSize() );
}

void SimpleParticlesApp::shutdown()
{
	params::PInterfaceGl::save();
}

void SimpleParticlesApp::torrentReceived( TorrentRef tr )
{
	console() << *tr << endl;
}

void SimpleParticlesApp::peerReceived( PeerRef cr )
{
	//console() << "added " << *cr << endl;
	Vec2f bv = Vec2f( getWindowSize() ) * Vec2f( .1f, .0f );
	bv.rotate( cr->getBearing() );
	Vec2f pos = getWindowCenter() + bv;
	app::App::get()->dispatchSync( [&] {
			mEmitterController.addEmitter( Vec3f( Vec2f( pos ), 0.f ), Vec3f::zero() ); } );
}

void SimpleParticlesApp::chunkReceived( ChunkRef cr )
{
	// increase the radius of the peer emitter
	EmitterRef e = mEmitterController.mEmitters[ cr->getPeerId() ];
	float r = e->mRadius;
	if ( r < GlobalSettings::get().mEmitterRadiusMax )
	{
		e->setRadius( r + GlobalSettings::get().mEmitterRadiusStep );
	}

	// pull the emitter towards the chunk position in the torrent
	uint32_t id = Rand::randInt( cr->getPeerId() );
	auto tr = std::dynamic_pointer_cast< TorrentPuzzle >( mTorrentRef );
	Vec3f loc = tr->getChunkTargetPos( cr );
	app::App::get()->dispatchSync( [&] { 
			mEmitterController.addForceIdAttractor(
				GlobalSettings::get().mEmitterAttractionMagnitude,
				GlobalSettings::get().mEmitterAttractionDuration,
				loc, id ); } );
}

void SimpleParticlesApp::segmentReceived( SegmentRef sr )
{
	//console() << "added " << *sr << endl;
}

void SimpleParticlesApp::mouseDown( MouseEvent event )
{
	if ( !mTorrentRef )
		return;
	uint32_t id = Rand::randInt( mTorrentRef->getNumPeers() );
	Vec3f loc( Vec2f( event.getPos() ), 0.f );
	mEmitterController.addForceIdAttractor(
		GlobalSettings::get().mEmitterAttractionMagnitude,
		GlobalSettings::get().mEmitterAttractionDuration,
		loc, id );
}

void SimpleParticlesApp::update()
{
	mFps = getAverageFps();

	mEmitterController.getForceRef( mForceRepulsionId )->mMagnitude = GlobalSettings::get().mEmitterRepulsion;
	mEmitterController.update();

	/*
	if ( mTorrentRef )
	{
		for ( auto peer: mTorrentRef->getPeers() )
		{
			std::static_pointer_cast< PeerCircle >( peer )->update();
		}
	}
	*/
}


void SimpleParticlesApp::draw()
{
	gl::clear( Color::black() );

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	mEmitterController.render();

	/*
	if ( mTorrentRef )
	{
		Rectf bounds = getWindowBounds();
		int i = 0;
		for ( auto peer: mTorrentRef->getPeers() )
		{
			shared_ptr< PeerCircle > pcr = std::static_pointer_cast< PeerCircle >( peer );
			pcr->draw( bounds );
			//console() << "peer " << i << " " << pcr->getPos() << endl;
			i++;
		}
	}
	*/

	params::PInterfaceGl::draw();
}

void SimpleParticlesApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			mParams.show( !mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

} // namespace tf

CINDER_APP_BASIC( tf::SimpleParticlesApp, RendererGl )

