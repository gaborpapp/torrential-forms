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

#include <cstring>
#include <string>
#include <vector>

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/Function.h"
#include "cinder/Surface.h"

#include "Visualizer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace tf {

class TorrentPuzzle : public tf::Torrent
{
	public:
		TorrentPuzzle( int numberOfFiles, float downloadDuration, int totalSize );

		void draw( const ci::Rectf &rect );
		void addChunk( ChunkRef cr );

	private:
		void init();

		int mTextureWidth;
		ci::gl::Texture mTexture;
		ci::Surface8u mSurface;

		bool mTextureNeedsUpdate;
};

class TorrentPuzzleFactory : public TorrentFactory
{
	public:
		std::shared_ptr< Torrent > createTorrent( int numberOfFiles, float downloadDuration, int totalSize )
		{
			return std::shared_ptr< Torrent >( new TorrentPuzzle( numberOfFiles, downloadDuration, totalSize ) );
		}
};

class SimplePuzzleApp : public AppBasic, Visualizer
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );

		void update();
		void draw();

		void torrentReceived( TorrentRef tr );
		void chunkReceived( ChunkRef cr );
		void segmentReceived( SegmentRef sr );

	private:
};

TorrentPuzzle::TorrentPuzzle( int numberOfFiles, float downloadDuration, int totalSize ) :
	Torrent( numberOfFiles, downloadDuration, totalSize ),
	mTextureWidth( 2048 )
{
	// create texture on primary thread
	app::App::get()->dispatchSync( bind( &TorrentPuzzle::init, this ) );
}

void TorrentPuzzle::init()
{
	mSurface = Surface8u( mTextureWidth, 1, false );
	memset( mSurface.getData(), 100, mSurface.getHeight() * mSurface.getRowBytes() );

	mTexture = gl::Texture( mSurface );
	mTextureNeedsUpdate = false;
}

void TorrentPuzzle::addChunk( ChunkRef cr )
{
	int fileOffset  = cr->mFileRef->getOffset();
	int torrentBegin = cr->mBegin + fileOffset;
	int torrentEnd = cr->mEnd + fileOffset;
	int x0 = (double)mTextureWidth * (double)torrentBegin / mTotalSize;
	int x1 = (double)mTextureWidth * (double)torrentEnd / mTotalSize;
	if ( math< int >::abs( x1 - x0 ) < 1 )
		x1 = x0 + 1;

	//Color8u color( CM_HSV, cr->mPeerId / (float)getNumPeers(), .7f, .7f );
	Color8u color = Color::white();
	Area area( x0, 0, x1, 1 );
	Surface::Iter iter = mSurface.getIter( area );
	while ( iter.line() )
	{
		while ( iter.pixel() )
		{
			iter.r() = color.r;
			iter.g() = color.g;
			iter.b() = color.b;
		}
	}
	mTextureNeedsUpdate = true;
}

void TorrentPuzzle::draw( const Rectf &rect )
{
	if ( mTextureNeedsUpdate )
	{
		mTexture.update( mSurface );
		mTextureNeedsUpdate = false;
	}
	mTexture.enableAndBind();
	gl::color( Color::white() );
	gl::drawSolidRect( rect );
	mTexture.unbind();
	mTexture.disable();
}

void SimplePuzzleApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

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

