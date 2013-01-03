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
		Torrent( size_t numFiles, float downloadDuration, size_t totalSize ) :
			mNumFiles( numFiles ), mDownloadDuration( downloadDuration ),
			mTotalSize( totalSize )
		{}

		virtual ~Torrent() { mFiles.clear(); }

		size_t getNumFiles() const { return mNumFiles; }
		std::vector< FileRef > & getFiles() { return mFiles; }
		const std::vector< FileRef > & getFiles() const { return mFiles; }

		float getDownloadDuration() const { return mDownloadDuration; }
		size_t getTotalSize() const { return mTotalSize; }

		size_t getNumPeers() const { return mPeers.size(); }
		std::map< int, PeerRef > & getPeers() { return mPeers; }
		const std::map< int, PeerRef > & getPeers() const { return mPeers; }

		friend std::ostream& operator<<( std::ostream &lhs, const Torrent &rhs )
		{
			lhs << "Torrent( files = " << rhs.mNumFiles <<
				", download duration = " << rhs.mDownloadDuration <<
				", size = " << rhs.mTotalSize << " )";
			return lhs;
		}

	protected:
		size_t mNumFiles;
		float mDownloadDuration;
		size_t mTotalSize;

		std::vector< FileRef > mFiles;
		std::map< int, PeerRef > mPeers;

		friend class Visualizer;
};

typedef std::shared_ptr< Torrent > TorrentRef;

class TorrentFactory
{
	public:
		virtual TorrentRef createTorrent( size_t numberOfFiles, float downloadDuration, size_t totalSize ) = 0;
		virtual ~TorrentFactory() {}
};

class DefaultTorrentFactory : public TorrentFactory
{
	public:
		TorrentRef createTorrent( size_t numberOfFiles, float downloadDuration, size_t totalSize )
		{
			return TorrentRef( new Torrent( numberOfFiles, downloadDuration, totalSize ) );
		}
};

class File
{
	public:
		File( int fileNum, int offset, int length, TorrentRef tr ) :
			mFileNum( fileNum ), mOffset( offset ), mLength( length ),
			mTorrentRef( tr )
		{}

		int getFileNum() const { return mFileNum; }
		int getOffset() const { return mOffset; }
		int getLength() const { return mLength; }
		TorrentRef getTorrentRef() const { return mTorrentRef; }

	protected:
		int mFileNum;
		int mOffset;
		int mLength;
		TorrentRef mTorrentRef;
};

class FileFactory
{
	public:
		virtual FileRef createFile( int fileNum, int offset, int length, TorrentRef tr ) = 0;
		virtual ~FileFactory() {}
};

class DefaultFileFactory : public FileFactory
{
	public:
		FileRef createFile( int fileNum, int offset, int length, TorrentRef tr )
		{
			return FileRef( new File( fileNum, offset, length, tr ) );
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
			return mTorrentReceivedSig.connect( std::function< TorrentCallback >( std::bind( fn, obj, std::_1 ) ) );
		}
		template< typename T >
		boost::signals2::connection connectChunkReceived( void( T::*fn )( ChunkRef ), T *obj )
		{
			return mChunkReceivedSig.connect( std::function< ChunkCallback >( std::bind( fn, obj, std::_1 ) ) );
		}
		template< typename T >
		boost::signals2::connection connectSegmentReceived( void( T::*fn )( SegmentRef ), T *obj )
		{
			return mSegmentReceivedSig.connect( std::function< SegmentCallback >( std::bind( fn, obj, std::_1 ) ) );
		}

		void reset();

		int getServerPort( const std::vector< std::string > &args );

	protected:
		std::shared_ptr< TorrentFactory > mTorrentFactoryRef;
		std::shared_ptr< FileFactory > mFileFactoryRef;

		mndl::osc::Client mSender;
		mndl::osc::Server mListener;

		static const int LISTENER_PORT = 12110;

		TorrentRef mTorrentRef;

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

