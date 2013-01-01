#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Exception.h"

#include "OscClient.h"
#include "OscServer.h"

namespace tf {

struct Torrent
{
	Torrent( int numberOfFiles, float downloadDuration, int totalSize ) :
		mNumberOfFiles( numberOfFiles ), mDownloadDuration( downloadDuration ),
		mTotalSize( totalSize )
	{}

	int mNumberOfFiles;
	float mDownloadDuration;
	int mTotalSize;
};

typedef std::shared_ptr< Torrent > TorrentRef;

struct File
{
	File( int fileNum, int offset, int length ) :
		mFileNum( fileNum ), mOffset( offset ), mLength( length )
	{}

	int mFileNum;
	int mOffset;
	int mLength;
};

typedef std::shared_ptr< File > FileRef;

struct Peer
{
	Peer( int id, std::string address, float bearing, std::string location ) :
		mId( id ), mAddress( address ), mBearing( bearing ), mLocation( location )
	{}

	int mId;
	std::string mAddress;
	float mBearing;
	std::string mLocation;
};

typedef std::shared_ptr< Peer > PeerRef;

struct Chunk
{
	Chunk( int chunkId, int begin, int end, FileRef fr, int peerId, float t ) :
		mId( chunkId ), mBegin( begin ), mEnd( end ), mByteSize( end - begin ),
		mFileNum( fr->mFileNum ), mFileRef( fr ), mPeerId( peerId ), mT( t )
	{}

	int mId;
	int mBegin;
	int mEnd;
	int mByteSize;
	int mFileNum;
	FileRef mFileRef;
	int mPeerId;
	float mT;

	friend std::ostream& operator<<( std::ostream& lhs, const Chunk rhs )
	{
		lhs << "Chunk( id = " << rhs.mId << ", begin = " << rhs.mBegin <<
			", end = " << rhs.mEnd << ", fileNum = " << rhs.mFileNum << " )";
		return lhs;
	}
};

typedef std::shared_ptr< Chunk > ChunkRef;

struct Segment
{
	Segment( int segmentId, int begin, int end, FileRef fr, int peerId, float t, float duration ) :
		mId( segmentId ), mBegin( begin ), mEnd( end ), mByteSize( end - begin ),
		mFileNum( fr->mFileNum ), mFileRef( fr ), mPeerId( peerId ), mT( t ), mDuration( duration)
	{}

	int mId;
	int mBegin;
	int mEnd;
	int mByteSize;
	int mFileNum;
	FileRef mFileRef;
	int mPeerId;
	float mT;
	float mDuration;

	friend std::ostream& operator<<( std::ostream& lhs, const Segment rhs )
	{
		lhs << "Segment( id = " << rhs.mId << ", begin = " << rhs.mBegin <<
			", end = " << rhs.mEnd << ", fileNum = " << rhs.mFileNum << " )";
		return lhs;
	}
};

typedef std::shared_ptr< Segment > SegmentRef;

class Visualizer
{
	public:
		void setup( std::string serverIp, int serverPort );

		bool oscReceived( const mndl::osc::Message &message );
		bool handleTorrentMessage( const mndl::osc::Message &message );
		bool handleFileMessage( const mndl::osc::Message &message );
		bool handleChunkMessage( const mndl::osc::Message &message );
		bool handleSegmentMessage( const mndl::osc::Message &message );
		bool handlePeerMessage( const mndl::osc::Message &message );
		bool handleResetMessage( const mndl::osc::Message &message );
		bool handleShutdownMessage( const mndl::osc::Message &message );

		virtual void torrentReceived( TorrentRef tr ) {};
		virtual void chunkReceived( ChunkRef cr ) {};
		virtual void segmentReceived( SegmentRef sr ) {};

		void reset();
		void registerVisualizer( int port );

	protected:
		mndl::osc::Client mSender;
		mndl::osc::Server mListener;

		static const int LISTENER_PORT = 12110;

		TorrentRef mTorrentRef;
		std::vector< FileRef > mFiles;
		std::map< int, PeerRef > mPeers;

		class ExcUndeclaredFile : public ci::Exception {};
		class ExcChunkFromUndeclaredFile : public ci::Exception {};
		class ExcSegmentFromUndeclaredFile : public ci::Exception {};
};

} // namespace tf

