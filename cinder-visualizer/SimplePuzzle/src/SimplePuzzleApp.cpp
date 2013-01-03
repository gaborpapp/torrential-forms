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

	private:
		void torrentReceived( TorrentRef tr );
		void chunkReceived( ChunkRef cr );
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

