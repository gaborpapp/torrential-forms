#include <string>

#include "cinder/app/App.h"
#include "cinder/Utilities.h"

#include "Visualizer.h"

using namespace ci;
using namespace std;

namespace tf {

void Visualizer::setup( std::string serverIp, int serverPort )
{
	reset();

	mListener = mndl::osc::Server( LISTENER_PORT, mndl::osc::PROTO_TCP );

	mListener.registerOscReceived< Visualizer >( &Visualizer::handleTorrentMessage, this, "/torrent", "ifi" );
	mListener.registerOscReceived< Visualizer >( &Visualizer::handleFileMessage, this, "/file", "iii" );
	mListener.registerOscReceived< Visualizer >( &Visualizer::handleChunkMessage, this, "/chunk", "iiiiif" );
	mListener.registerOscReceived< Visualizer >( &Visualizer::handleSegmentMessage, this, "/segment", "iiiiiff" );
	mListener.registerOscReceived< Visualizer >( &Visualizer::handlePeerMessage, this, "/peer", "isfs" );
	mListener.registerOscReceived< Visualizer >( &Visualizer::handleResetMessage, this, "/reset" );
	mListener.registerOscReceived< Visualizer >( &Visualizer::handleShutdownMessage, this, "/shutdown" );

	mSender = mndl::osc::Client( serverIp, serverPort, mndl::osc::PROTO_TCP );
	registerVisualizer( LISTENER_PORT );
}

bool Visualizer::handleTorrentMessage( const mndl::osc::Message &message )
{
	int numFiles = message.getArg< int32_t >( 0 );
	float downloadDuration = message.getArg< float >( 1 );
	int totalSize = message.getArg< int32_t >( 2 );

	mTorrentRef = mTorrentFactoryRef->createTorrent( numFiles, downloadDuration, totalSize );
	mTorrentRef->mFiles.resize( mTorrentRef->getNumFiles() );
	mTorrentReceivedSig( mTorrentRef );

	return false;
}

bool Visualizer::handleFileMessage( const mndl::osc::Message &message )
{
	int fileNum = message.getArg< int32_t >( 0 );
	int offset = message.getArg< int32_t >( 1 );
	int length = message.getArg< int32_t >( 2 );

	if ( fileNum < mTorrentRef->getNumFiles() )
	{
		FileRef fr = mFileFactoryRef->createFile( fileNum, offset, length, mTorrentRef );
		mTorrentRef->mFiles[ fileNum ] = fr;
		mFileReceivedSig( fr );
	}
	else
	{
		throw ExcUndeclaredFile();
	}

	return false;
}

bool Visualizer::handleChunkMessage( const mndl::osc::Message &message )
{
	int chunkId = message.getArg< int32_t >( 0 );
	int torrentPosition = message.getArg< int32_t >( 1 );
	int byteSize = message.getArg< int32_t >( 2 );
	int fileNum = message.getArg< int32_t >( 3 );
	int peerId = message.getArg< int32_t >( 4 );
	float t = message.getArg< float >( 5 );

	if ( ( fileNum >= mTorrentRef->getNumFiles() ) ||
			!mTorrentRef->mFiles[ fileNum ] )
	{
		throw ExcChunkFromUndeclaredFile();
	}
	else
	{
		FileRef f = mTorrentRef->mFiles[ fileNum ];
		int begin = torrentPosition - f->getOffset();
		int end = begin + byteSize;
		ChunkRef cr = mChunkFactoryRef->createChunk( chunkId, begin, end, f, peerId, t );
		// TODO: add chunk to file?
		mChunkReceivedSig( cr );
	}

	return false;
}

bool Visualizer::handleSegmentMessage( const mndl::osc::Message &message )
{
	int segmentId = message.getArg< int32_t >( 0 );
	int torrentPosition = message.getArg< int32_t >( 1 );
	int byteSize = message.getArg< int32_t >( 2 );
	int fileNum = message.getArg< int32_t >( 3 );
	int peerId = message.getArg< int32_t >( 4 );
	float t = message.getArg< float >( 5 );
	float duration = message.getArg< float >( 6 );

	if ( ( fileNum >= mTorrentRef->getNumFiles() ) ||
			!mTorrentRef->mFiles[ fileNum ] )
	{
		throw ExcSegmentFromUndeclaredFile();
	}
	else
	{
		FileRef f = mTorrentRef->mFiles[ fileNum ];
		int begin = torrentPosition - f->getOffset();
		int end = begin + byteSize;
		SegmentRef sr( new Segment( segmentId, begin, end, f, peerId, t, duration ) );
		// TODO: add segment to file?
		mSegmentReceivedSig( sr );
	}
	return false;
}

bool Visualizer::handlePeerMessage( const mndl::osc::Message &message )
{
	int id = message.getArg< int32_t >( 0 );
	std::string address = message.getArg< std::string >( 1 );
	float bearing = message.getArg< float >( 2 );
	std::string location = message.getArg< std::string >( 3 );

	PeerRef pr = mPeerFactoryRef->createPeer( id, address, bearing, location, mTorrentRef );
	mTorrentRef->mPeers[ id ] = pr;
	mPeerReceivedSig( pr );
	return false;
}

bool Visualizer::handleResetMessage( const mndl::osc::Message &message )
{
	reset();
	return false;
}

bool Visualizer::handleShutdownMessage( const mndl::osc::Message &message )
{
	ci::app::App::get()->quit();
	return false;
}

void Visualizer::reset()
{
	mTorrentRef.reset();
}

void Visualizer::registerVisualizer( int port )
{
	mndl::osc::Message msg( "/register" );
	msg.addArg( LISTENER_PORT );
	mSender.send( msg );
}

int Visualizer::getServerPort( const vector< string > &args )
{
	int port = 0;
	auto argIt = args.begin();
	while ( argIt != args.end() )
	{
		if ( *argIt == "-port" )
		{
			++argIt;
			if ( argIt != args.end() )
				port = fromString< int >( *argIt );
		}
		++argIt;
	}

	if ( port == 0 )
		throw ExcMissingServerPort();

	return port;
}

} // namespace tf

