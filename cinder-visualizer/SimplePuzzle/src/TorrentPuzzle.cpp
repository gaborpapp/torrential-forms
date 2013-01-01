#include <cstring>

#include "cinder/app/App.h"

#include "TorrentPuzzle.h"

using namespace ci;

namespace tf {

TorrentPuzzle::TorrentPuzzle( size_t numberOfFiles, float downloadDuration, size_t totalSize ) :
	Torrent( numberOfFiles, downloadDuration, totalSize ),
	mTextureWidth( 2048 )
{
	// create texture on primary thread
	app::App::get()->dispatchSync( std::bind( &TorrentPuzzle::init, this ) );
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

	Color8u color( CM_HSV, cr->mPeerId / (float)cr->mFileRef->getTorrentRef()->getNumPeers(), .7f, .7f );
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

} // namespace tf

