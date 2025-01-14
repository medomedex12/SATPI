/* StreamThreadRtcp.cpp

   Copyright (C) 2014 - 2023 Marc Postema (mpostema09 -at- gmail.com)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   Or, point your browser to http://www.gnu.org/copyleft/gpl.html
*/
#include <output/StreamThreadRtcp.h>

#include <StreamClient.h>
#include <Stream.h>
#include <Log.h>

#include <cstring>

#include <sys/socket.h>

namespace output {

// =========================================================================
// -- Constructors and destructor ------------------------------------------
// =========================================================================

StreamThreadRtcp::StreamThreadRtcp(StreamInterface &stream) :
		StreamThreadRtcpBase("RTCP/UDP", stream) {}

StreamThreadRtcp::~StreamThreadRtcp() {
	_thread.terminateThread();
	const FeID id = _stream.getFeID();
	const StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Frontend: @#1, Destroy @#2 stream to @#3:@#4", id,
		_protocol, client.getIPAddressOfStream(), getStreamSocketPort(_clientID));

	_stream.getStreamClient(_clientID).getRtcpSocketAttr().closeFD();
}

// =============================================================================
//  -- output::StreamThreadRtcpBase --------------------------------------------
// =============================================================================

void StreamThreadRtcp::doStartStreaming(int clientID) {
	SocketAttr &rtcp = _stream.getStreamClient(clientID).getRtcpSocketAttr();

	if (!rtcp.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
		SI_LOG_ERROR("Frontend: @#1, Get RTCP handle failed", _stream.getFeID());
	}
}

int StreamThreadRtcp::getStreamSocketPort(const int clientID) const {
	return  _stream.getStreamClient(clientID).getRtcpSocketAttr().getSocketPort();
}

void StreamThreadRtcp::doSendDataToClient(const int clientID,
	const PacketPtr& sr, const int srlen,
	const PacketPtr& sdes, const int sdeslen,
	const PacketPtr& app, const int applen) {
	StreamClient &client = _stream.getStreamClient(clientID);

	const int len = srlen + sdeslen + applen;
	PacketPtr data(new uint8_t[len]);
	std::memcpy(data.get(), sr.get(), srlen);
	std::memcpy(data.get() + srlen, sdes.get(), sdeslen);
	std::memcpy(data.get() + srlen + sdeslen, app.get(), applen);

	// send the RTCP/UDP packet
	if (!client.getRtcpSocketAttr().sendDataTo(data.get(), len, 0)) {
		SI_LOG_ERROR("Frontend: @#1, Error sending @#2 data to @#3:@#4", _stream.getFeID(),
			_protocol, client.getIPAddressOfStream(), getStreamSocketPort(clientID));
	}
}

}
