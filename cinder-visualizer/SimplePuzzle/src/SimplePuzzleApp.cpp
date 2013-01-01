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

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/Cinder.h"

#include "TorrentPuzzle.h"
#include "Visualizer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace tf {

class SimplePuzzleApp : public AppBasic, Visualizer
{
	public:
		void setup();

		void keyDown( KeyEvent event );

		void update();
		void draw();

		void torrentReceived( TorrentRef tr );
		void chunkReceived( ChunkRef cr );
		void segmentReceived( SegmentRef sr );

	private:
};

void SimplePuzzleApp::setup()
{
	gl::disableVerticalSync();

	int port = Visualizer::getServerPort( getArgs() );
	Visualizer::setTorrentFactory( std::shared_ptr< TorrentFactory >( new TorrentPuzzleFactory() ) );
	Visualizer::setup( "127.0.0.1", port );

	connectTorrentReceived< SimplePuzzleApp >( &SimplePuzzleApp::torrentReceived, this );
	connectChunkReceived< SimplePuzzleApp >( &SimplePuzzleApp::chunkReceived, this );
}

void SimplePuzzleApp::torrentReceived( TorrentRef tr )
{
	console() << *tr << endl;
}

void SimplePuzzleApp::chunkReceived( ChunkRef cr )
{
	if ( mTorrentRef )
		std::dynamic_pointer_cast< TorrentPuzzle >( mTorrentRef )->addChunk( cr );
}

void SimplePuzzleApp::update()
{
}

void SimplePuzzleApp::draw()
{
	gl::clear( Color::black() );

	if ( mTorrentRef )
	{
		Rectf rect( getWindowBounds() );
		rect.scaleCentered( Vec2f( .95f, .05f ) );
		std::dynamic_pointer_cast< TorrentPuzzle >( mTorrentRef )->draw( rect );
	}
}

void SimplePuzzleApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		default:
			break;
	}
}

} // namespace tf

CINDER_APP_BASIC( tf::SimplePuzzleApp, RendererGl )

