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
		TorrentPuzzle( size_t numFiles, float downloadDuration, size_t totalSize,
				size_t numChunks, size_t numSegments );

		void draw( const ci::Rectf &rect );
		void addChunk( ChunkRef cr );
		ci::Vec3f getChunkTargetPos( ChunkRef cr );

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
		TorrentRef createTorrent( size_t numFiles, float downloadDuration, size_t totalSize,
				size_t numChunks, size_t numSegments )
		{
			return TorrentRef( new TorrentPuzzle( numFiles, downloadDuration, totalSize,
						numChunks, numSegments ) );
		}
};

} // namespace tf

