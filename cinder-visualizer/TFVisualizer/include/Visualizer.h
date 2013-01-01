#pragma once

#include <iostream>
#include <map>
#include <vector>

#include <boost/signals2/signal.hpp>

#include "cinder/Cinder.h"
#include "cinder/Exception.h"

#include "OscClient.h"
#include "OscServer.h"

namespace tf {

class Visualizer;
class File;
class Peer;
typedef std::shared_ptr< File > FileRef;
typedef std::shared_ptr< Peer > PeerRef;

class Torrent
{
	public:
		Torrent( int numFiles, float downloadDuration, int totalSize ) :
			mNumFiles( numFiles ), mDownloadDuration( downloadDuration ),
			mTotalSize( totalSize )
		{}

		virtual ~Torrent()
		{
			mFiles.clear();
		}

		int getNumFiles() const { return mNumFiles; }
		float getDownloadDuration() const { return mDownloadDuration; }
		int getTotalSize() const { return mTotalSize; }

		friend std::ostream& operator<<( std::ostream &lhs, const Torrent &rhs )
		{
			lhs << "Torrent( files = " << rhs.mNumFiles <<
				", download duration = " << rhs.mDownloadDuration <<
				", size = " << rhs.mTotalSize << " )";
			return lhs;
		}

		friend class Visualizer;

	protected:
		int mNumFiles;
		float mDownloadDuration;
		int mTotalSize;

		std::vector< FileRef > mFiles;
};

typedef std::shared_ptr< Torrent > TorrentRef;

class TorrentFactory
{
	public:
		virtual TorrentRef createTorrent( int numberOfFiles, float downloadDuration, int totalSize ) = 0;
		virtual ~TorrentFactory() {}
};

class DefaultTorrentFactory : public TorrentFactory
{
	public:
		TorrentRef createTorrent( int numberOfFiles, float downloadDuration, int totalSize )
		{
			return TorrentRef( new Torrent( numberOfFiles, downloadDuration, totalSize ) );
		}
};

class File
{
	public:
		File( int fileNum, int offset, int length ) :
			mFileNum( fileNum ), mOffset( offset ), mLength( length )
		{}

		int getFileNum() const { return mFileNum; }
		int getOffset() const { return mOffset; }
		int getLength() const { return mLength; }

	protected:
		int mFileNum;
		int mOffset;
		int mLength;
};

class FileFactory
{
	public:
		virtual FileRef createFile( int fileNum, int offset, int length ) = 0;
		virtual ~FileFactory() {}
};

class DefaultFileFactory : public FileFactory
{
	public:
		FileRef createFile( int fileNum, int offset, int length )
		{
			return FileRef( new File( fileNum, offset, length ) );
		}
};

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

class Chunk
{
	public:
		Chunk( int chunkId, int begin, int end, FileRef fr, int peerId, float t ) :
			mId( chunkId ), mBegin( begin ), mEnd( end ), mByteSize( end - begin ),
			mFileNum( fr->getFileNum() ), mFileRef( fr ), mPeerId( peerId ), mTime( t )
		{}

	int mId;
	int mBegin; //< Position relative to the file.
	int mEnd; //< End position relative to the file.
	int mByteSize;
	int mFileNum;
	FileRef mFileRef;
	int mPeerId;
	float mTime; //< Chunk's arrival time, where 0 is the start of the transmission log.

	friend std::ostream& operator<<( std::ostream& lhs, const Chunk &rhs )
	{
		lhs << "Chunk( id = " << rhs.mId << ", begin = " << rhs.mBegin <<
			", end = " << rhs.mEnd << ", fileNum = " << rhs.mFileNum << " )";
		return lhs;
	}
};

typedef std::shared_ptr< Chunk > ChunkRef;

class Segment : public Chunk
{
	public:
		Segment( int segmentId, int begin, int end, FileRef fr, int peerId, float t, float duration ) :
			Chunk( segmentId, begin, end, fr, peerId, t ),
			mDuration( duration)
		{}

		float mDuration; //< How long the content will be played back acoustically.
};

typedef std::shared_ptr< Segment > SegmentRef;

typedef void( TorrentCallback )( TorrentRef );
typedef void( ChunkCallback )( ChunkRef );
typedef void( SegmentCallback )( SegmentRef );
typedef boost::signals2::signal< TorrentCallback > TorrentSignal;
typedef boost::signals2::signal< ChunkCallback > ChunkSignal;
typedef boost::signals2::signal< SegmentCallback > SegmentSignal;

class Visualizer
{
	public:
		Visualizer() :
			mTorrentFactoryRef( new DefaultTorrentFactory() ),
			mFileFactoryRef( new DefaultFileFactory() )
		{}

		void setup( std::string serverIp, int serverPort );

		void setTorrentFactory( std::shared_ptr< TorrentFactory > torrentFactoryRef )
		{
			mTorrentFactoryRef = torrentFactoryRef;
		}

		template< typename T >
		boost::signals2::connection connectTorrentReceived( void( T::*fn )( TorrentRef ), T *obj )
		{
			return mTorrentReceivedSig.connect( std::function< TorrentCallback >( boost::bind( fn, obj, ::_1 ) ) );
		}
		template< typename T >
		boost::signals2::connection connectChunkReceived( void( T::*fn )( ChunkRef ), T *obj )
		{
			return mChunkReceivedSig.connect( std::function< ChunkCallback >( boost::bind( fn, obj, ::_1 ) ) );
		}
		template< typename T >
		boost::signals2::connection connectSegmentReceived( void( T::*fn )( SegmentRef ), T *obj )
		{
			return mSegmentReceivedSig.connect( std::function< SegmentCallback >( boost::bind( fn, obj, ::_1 ) ) );
		}

		void reset();

		size_t getNumPeers() const;

		int getServerPort( const std::vector< std::string > &args );

	protected:
		std::shared_ptr< TorrentFactory > mTorrentFactoryRef;
		std::shared_ptr< FileFactory > mFileFactoryRef;

		mndl::osc::Client mSender;
		mndl::osc::Server mListener;

		static const int LISTENER_PORT = 12110;

		TorrentRef mTorrentRef;
		std::map< int, PeerRef > mPeers;

		bool handleTorrentMessage( const mndl::osc::Message &message );
		bool handleFileMessage( const mndl::osc::Message &message );
		bool handleChunkMessage( const mndl::osc::Message &message );
		bool handleSegmentMessage( const mndl::osc::Message &message );
		bool handlePeerMessage( const mndl::osc::Message &message );
		bool handleResetMessage( const mndl::osc::Message &message );
		bool handleShutdownMessage( const mndl::osc::Message &message );

		void registerVisualizer( int port );

		TorrentSignal mTorrentReceivedSig;
		ChunkSignal mChunkReceivedSig;
		SegmentSignal mSegmentReceivedSig;

		class ExcMissingServerPort : public ci::Exception {};
		class ExcUndeclaredFile : public ci::Exception {};
		class ExcChunkFromUndeclaredFile : public ci::Exception {};
		class ExcSegmentFromUndeclaredFile : public ci::Exception {};
};

} // namespace tf

