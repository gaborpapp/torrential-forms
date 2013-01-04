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

	protected:
		size_t mNumFiles;
		float mDownloadDuration;
		size_t mTotalSize;

		std::vector< FileRef > mFiles;
		std::map< int, PeerRef > mPeers;

		friend std::ostream& operator<<( std::ostream &lhs, const Torrent &rhs )
		{
			lhs << "Torrent( files = " << rhs.mNumFiles <<
				", download duration = " << rhs.mDownloadDuration <<
				", size = " << rhs.mTotalSize << " )";
			return lhs;
		}


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
		File( uint32_t fileNum, off_t offset, size_t length, TorrentRef tr ) :
			mFileNum( fileNum ), mOffset( offset ), mLength( length ),
			mTorrentRef( tr )
		{}

		int getFileNum() const { return mFileNum; }
		int getOffset() const { return mOffset; }
		int getLength() const { return mLength; }
		TorrentRef getTorrentRef() const { return mTorrentRef; }

	protected:
		uint32_t mFileNum;
		off_t mOffset;
		size_t mLength;
		TorrentRef mTorrentRef;
};

class FileFactory
{
	public:
		virtual FileRef createFile( uint32_t fileNum, off_t offset, size_t length, TorrentRef tr ) = 0;
		virtual ~FileFactory() {}
};

class DefaultFileFactory : public FileFactory
{
	public:
		FileRef createFile( uint32_t fileNum, off_t offset, size_t length, TorrentRef tr )
		{
			return FileRef( new File( fileNum, offset, length, tr ) );
		}
};

class Peer
{
	public:
		Peer( int id, std::string address, float bearing, std::string location, TorrentRef tr ) :
			mId( id ), mAddress( address ), mBearing( bearing ), mLocation( location ),
			mTorrentRef( tr )
		{}

		int getId() const { return mId; }
		const std::string & getAddress() const { return mAddress; }
		float getBearing() const { return mBearing; }
		const std::string & getLocation() const { return mLocation; }

	protected:
		int mId;
		std::string mAddress;
		float mBearing;
		std::string mLocation;
		TorrentRef mTorrentRef;
};

class PeerFactory
{
	public:
		virtual PeerRef createPeer( int id, std::string address, float bearing,
				std::string location, TorrentRef tr ) = 0;
		virtual ~PeerFactory() {}
};

class DefaultPeerFactory : public PeerFactory
{
	public:
		PeerRef createPeer( int id, std::string address, float bearing, std::string location,
				TorrentRef tr )
		{
			return PeerRef( new Peer( id, address, bearing, location, tr ) );
		}
};

class Chunk
{
	public:
		Chunk( int chunkId, off_t begin, off_t end, FileRef fr, int peerId, float t ) :
			mId( chunkId ), mBegin( begin ), mEnd( end ), mByteSize( end - begin ),
			mFileNum( fr->getFileNum() ), mFileRef( fr ), mPeerId( peerId ),
			mPeerRef( fr->getTorrentRef()->getPeers()[ peerId ] ), mTime( t )
		{}

		int getId() const { return mId; }
		off_t getBegin() const { return mBegin; }
		off_t getEnd() const { return mEnd; }
		size_t getByteSize() const { return mByteSize; }
		uint32_t getFileNum() const { return mFileNum; }
		FileRef getFileRef() const { return mFileRef; }
		int getPeerId() const { return mPeerId; }
		PeerRef getPeerRef() const { return mPeerRef; }
		float getTime() const { return mTime; }

	protected:
		int mId;
		off_t mBegin; //< Position relative to the file.
		off_t mEnd; //< End position relative to the file.
		size_t mByteSize;
		uint32_t mFileNum;
		FileRef mFileRef;
		int mPeerId;
		PeerRef mPeerRef;
		float mTime; //< Chunk's arrival time, where 0 is the start of the transmission log.

		friend std::ostream& operator<<( std::ostream& lhs, const Chunk &rhs )
		{
			lhs << "Chunk( id = " << rhs.mId << ", begin = " << rhs.mBegin <<
				", end = " << rhs.mEnd << ", fileNum = " << rhs.mFileNum << " )";
			return lhs;
		}
};

typedef std::shared_ptr< Chunk > ChunkRef;

class ChunkFactory
{
	public:
		virtual ChunkRef createChunk( int chunkId, off_t begin, off_t end, FileRef fr, int peerId, float t ) = 0;
		virtual ~ChunkFactory() {}
};

class DefaultChunkFactory : public ChunkFactory
{
	public:
		ChunkRef createChunk( int chunkId, off_t begin, off_t end, FileRef fr, int peerId, float t )
		{
			return ChunkRef( new Chunk( chunkId, begin, end, fr, peerId, t ) );
		}
};

class Segment : public Chunk
{
	public:
		Segment( int segmentId, off_t begin, off_t end, FileRef fr, int peerId, float t, float duration ) :
			Chunk( segmentId, begin, end, fr, peerId, t ),
			mDuration( duration)
		{}

		float getDuration() const { return mDuration; }

	protected:
		float mDuration; //< How long the content will be played back acoustically.
};

typedef std::shared_ptr< Segment > SegmentRef;

class SegmentFactory
{
	public:
		virtual SegmentRef createSegment( int segmentId, off_t begin, off_t end, FileRef fr,
				int peerId, float t, float duration ) = 0;
		virtual ~SegmentFactory() {}
};

class DefaultSegmentFactory : public SegmentFactory
{
	public:
		SegmentRef createSegment( int segmentId, off_t begin, off_t end, FileRef fr,
				int peerId, float t, float duration )
		{
			return SegmentRef( new Segment( segmentId, begin, end, fr, peerId, t, duration ) );
		}
};

typedef void( TorrentCallback )( TorrentRef );
typedef void( PeerCallback )( PeerRef );
typedef void( FileCallback )( FileRef );
typedef void( ChunkCallback )( ChunkRef );
typedef void( SegmentCallback )( SegmentRef );
typedef boost::signals2::signal< TorrentCallback > TorrentSignal;
typedef boost::signals2::signal< PeerCallback > PeerSignal;
typedef boost::signals2::signal< FileCallback > FileSignal;
typedef boost::signals2::signal< ChunkCallback > ChunkSignal;
typedef boost::signals2::signal< SegmentCallback > SegmentSignal;

class Visualizer
{
	public:
		Visualizer() :
			mTorrentFactoryRef( new DefaultTorrentFactory() ),
			mPeerFactoryRef( new DefaultPeerFactory() ),
			mFileFactoryRef( new DefaultFileFactory() ),
			mChunkFactoryRef( new DefaultChunkFactory() ),
			mSegmentFactoryRef( new DefaultSegmentFactory() )
		{}

		void setup( std::string serverIp, int serverPort );

		void setTorrentFactory( std::shared_ptr< TorrentFactory > torrentFactoryRef )
		{
			mTorrentFactoryRef = torrentFactoryRef;
		}

		void setPeerFactory( std::shared_ptr< PeerFactory > peerFactoryRef )
		{
			mPeerFactoryRef = peerFactoryRef;
		}

		void setFileFactory( std::shared_ptr< FileFactory > fileFactoryRef )
		{
			mFileFactoryRef = fileFactoryRef;
		}

		void setChunkFactory( std::shared_ptr< ChunkFactory > chunkFactoryRef )
		{
			mChunkFactoryRef = chunkFactoryRef;
		}

		void setSegmentFactory( std::shared_ptr< SegmentFactory > segmentFactoryRef )
		{
			mSegmentFactoryRef = segmentFactoryRef;
		}

		template< typename T >
		boost::signals2::connection connectTorrentReceived( void( T::*fn )( TorrentRef ), T *obj )
		{
			return mTorrentReceivedSig.connect( std::function< TorrentCallback >( std::bind( fn, obj, std::_1 ) ) );
		}

		template< typename T >
		boost::signals2::connection connectPeerReceived( void( T::*fn )( PeerRef ), T *obj )
		{
			return mPeerReceivedSig.connect( std::function< PeerCallback >( std::bind( fn, obj, std::_1 ) ) );
		}
		template< typename T >
		boost::signals2::connection connectFileReceived( void( T::*fn )( FileRef ), T *obj )
		{
			return mFileReceivedSig.connect( std::function< FileCallback >( std::bind( fn, obj, std::_1 ) ) );
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
		std::shared_ptr< PeerFactory > mPeerFactoryRef;
		std::shared_ptr< FileFactory > mFileFactoryRef;
		std::shared_ptr< ChunkFactory > mChunkFactoryRef;
		std::shared_ptr< SegmentFactory > mSegmentFactoryRef;

		mndl::osc::Client mSender;
		mndl::osc::Server mListener;

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
		PeerSignal mPeerReceivedSig;
		FileSignal mFileReceivedSig;
		ChunkSignal mChunkReceivedSig;
		SegmentSignal mSegmentReceivedSig;

		class ExcMissingServerPort : public ci::Exception {};
		class ExcUndeclaredFile : public ci::Exception {};
		class ExcChunkFromUndeclaredFile : public ci::Exception {};
		class ExcSegmentFromUndeclaredFile : public ci::Exception {};
};

} // namespace tf

