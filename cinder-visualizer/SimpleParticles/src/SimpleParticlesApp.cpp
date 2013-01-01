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
#include "cinder/params/Params.h"
#include "cinder/Exception.h"
#include "cinder/Utilities.h"

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

		void keyDown( KeyEvent event );

		void update();
		void draw();

		class ExcMissingServerPort : public Exception {};

		void chunkReceived( ChunkRef cr );
		void segmentReceived( SegmentRef sr );

	private:
		params::InterfaceGl mParams;
};

void SimpleParticlesApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void SimpleParticlesApp::setup()
{
	gl::disableVerticalSync();

	// get server port
	const vector< string > &args = getArgs();
	int port = 0;
	auto argIt = args.begin();
	while ( argIt != args.end() )
	{
		if ( *argIt == "-port" )
		{
			++argIt;
			if ( argIt != args.end() )
				port = fromString< int >( *argIt );
		}
		++argIt;
	}

	if ( port == 0 )
		throw ExcMissingServerPort();

	Visualizer::setup( "127.0.0.1", port );

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
}

void SimpleParticlesApp::chunkReceived( ChunkRef cr )
{
	console() << "added " << *cr << endl;
}

void SimpleParticlesApp::segmentReceived( SegmentRef sr )
{
	console() << "added " << *sr << endl;
}

void SimpleParticlesApp::update()
{
}

void SimpleParticlesApp::draw()
{
	gl::clear( Color::black() );

	params::InterfaceGl::draw();
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

