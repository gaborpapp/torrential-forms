#include <string>

#include "cinder/app/App.h"

#include "Visualizer.h"

using namespace ci;
using namespace std;

namespace tf {

void Visualizer::setup( string serverIp, int serverPort )
{
	reset();

	mListener = mndl::osc::Server( LISTENER_PORT, mndl::osc::PROTO_TCP );

	//mListener.registerOscReceived< Visualizer >( &Visualizer::oscReceived, this );
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

void Visualizer::reset()
{
	mTorrentRef.reset();
	mFiles.clear();
	mPeers.clear();
}

bool Visualizer::handleTorrentMessage( const mndl::osc::Message &message )
{
	int numberOfFiles = message.getArg< int32_t >( 0 );
	float downloadDuration = message.getArg< float >( 1 );
	int totalSize = message.getArg< int32_t >( 2 );

	mTorrentRef = TorrentRef( new Torrent( numberOfFiles, downloadDuration, totalSize ) );
	mFiles.resize( mTorrentRef->mNumberOfFiles );

	return false;
}

bool Visualizer::handleFileMessage( const mndl::osc::Message &message )
{
	int fileNum = message.getArg< int32_t >( 0 );
	int offset = message.getArg< int32_t >( 1 );
	int length = message.getArg< int32_t >( 2 );

	if ( fileNum < mTorrentRef->mNumberOfFiles )
		mFiles[ fileNum ] = FileRef( new File( fileNum, offset, length ) );
	else
		throw ExcUndeclaredFile();

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

	if ( ( fileNum >= mTorrentRef->mNumberOfFiles ) ||
		 !mFiles[ fileNum ] )
	{
		throw ExcChunkFromUndeclaredFile();
	}
	else
	{
		FileRef f = mFiles[ fileNum ];
		int begin = torrentPosition - f->mOffset;
		int end = begin + byteSize;
		ChunkRef cr( new Chunk( chunkId, begin, end, f, peerId, t ) );
		chunkReceived( cr );
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

	if ( ( fileNum >= mTorrentRef->mNumberOfFiles ) ||
		 !mFiles[ fileNum ] )
	{
		throw ExcSegmentFromUndeclaredFile();
	}
	else
	{
		FileRef f = mFiles[ fileNum ];
		int begin = torrentPosition - f->mOffset;
		int end = begin + byteSize;
		SegmentRef sr( new Segment( segmentId, begin, end, f, peerId, t, duration ) );
		segmentReceived( sr );
	}
	return false;
}

bool Visualizer::handlePeerMessage( const mndl::osc::Message &message )
{
	int id = message.getArg< int32_t >( 0 );
	string address = message.getArg< string >( 1 );
	float bearing = message.getArg< float >( 2 );
	string location = message.getArg< string >( 3 );

	mPeers[ id ] = PeerRef( new Peer( id, address, bearing, location ) );
	return false;
}

bool Visualizer::handleResetMessage( const mndl::osc::Message &message )
{
	reset();
	return false;
}

bool Visualizer::handleShutdownMessage( const mndl::osc::Message &message )
{
	app::App::get()->quit();
	return false;
}

bool Visualizer::oscReceived( const mndl::osc::Message &message )
{
	app::console() << app::getElapsedSeconds() << " message received " << message.getAddressPattern() << endl;
	for ( size_t i = 0; i < message.getNumArgs(); i++ )
	{
		app::console() <<  " argument: " << i;
		app::console() <<  " type: " << message.getArgType( i ) << " value: ";
		switch ( message.getArgType( i ) )
		{
			case 'i':
				app::console() << message.getArg< int32_t >( i );
				break;

			case 'f':
				app::console() << message.getArg< float >( i );
				break;

			case 's':
				app::console() << message.getArg< string >( i );
				break;

			default:
				break;
		}
		app::console() << endl;
	}
	return true;
}

void Visualizer::registerVisualizer( int port )
{
	mndl::osc::Message msg( "/register" );
	msg.addArg( LISTENER_PORT );
	mSender.send( msg );
}

} // namespace tf

