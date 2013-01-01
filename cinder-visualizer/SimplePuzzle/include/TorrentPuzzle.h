#pragma once

#include "cinder/gl/Texture.h"
#include "cinder/CinderMath.h"
#include "cinder/Function.h"
#include "cinder/Rect.h"
#include "cinder/Surface.h"

#include "Visualizer.h"

namespace tf {

class TorrentPuzzle : public tf::Torrent
{
	public:
		TorrentPuzzle( size_t numberOfFiles, float downloadDuration, size_t totalSize );

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
		TorrentRef createTorrent( size_t numberOfFiles, float downloadDuration, size_t totalSize )
		{
			return TorrentRef( new TorrentPuzzle( numberOfFiles, downloadDuration, totalSize ) );
		}
};

} // namespace tf

