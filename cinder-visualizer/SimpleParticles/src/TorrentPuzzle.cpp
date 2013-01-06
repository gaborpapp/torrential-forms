#include <cstring>

#include "cinder/app/App.h"

#include "TorrentPuzzle.h"

using namespace ci;

namespace tf {

TorrentPuzzle::TorrentPuzzle( size_t numFiles, float downloadDuration, size_t totalSize,
		size_t numChunks, size_t numSegments ) :
	Torrent( numFiles, downloadDuration, totalSize, numChunks, numSegments ),
	mTextureWidth( 2048 )
{
	// create texture on primary thread
	app::App::get()->dispatchSync( [&] { init(); } );
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
	int fileOffset  = cr->getFileRef()->getOffset();
	int torrentBegin = cr->getBegin() + fileOffset;
	int torrentEnd = cr->getEnd() + fileOffset;
	int x0 = (double)mTextureWidth * (double)torrentBegin / mTotalSize;
	int x1 = (double)mTextureWidth * (double)torrentEnd / mTotalSize;
	if ( math< int >::abs( x1 - x0 ) < 1 )
		x1 = x0 + 1;

	Color8u color( CM_HSV, cr->getPeerId() / (float)cr->getFileRef()->getTorrentRef()->getNumPeers(), .7f, .7f );
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

Vec3f TorrentPuzzle::getChunkTargetPos( ChunkRef cr )
{
	int fileOffset  = cr->getFileRef()->getOffset();
	double torrentPos = .5 * ( cr->getBegin() + cr->getEnd() ) + fileOffset;
	float x0 = (double)app::getWindowWidth() * (double)torrentPos / (double)mTotalSize;

	return Vec3f( x0, app::getWindowHeight() * .5f, 0.f );
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

