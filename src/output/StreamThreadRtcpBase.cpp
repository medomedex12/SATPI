/* StreamThreadRtcpBase.cpp

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
#include <output/StreamThreadRtcpBase.h>

#include <Log.h>
#include <Stream.h>
#include <Utils.h>

#include <chrono>
#include <cstring>

namespace output {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamThreadRtcpBase::StreamThreadRtcpBase(
	const std::string &protocol,
	StreamInterface &stream) :
		_clientID(0),
		_protocol(protocol),
		_stream(stream),
 		_thread(
			"RtcpThread",
			std::bind(&StreamThreadRtcpBase::threadExecuteFunction, this)) {}

StreamThreadRtcpBase::~StreamThreadRtcpBase() {}

// =========================================================================
//  -- Other member functions ----------------------------------------------
// =========================================================================

bool StreamThreadRtcpBase::startStreaming(const int clientID) {

	doStartStreaming(clientID);

	const StreamClient &client = _stream.getStreamClient(clientID);

	if (!_thread.startThread()) {
		SI_LOG_ERROR("Frontend: @#1, Start @#2 stream to @#3:@#4 ERROR", _stream.getFeID(),
			_protocol, client.getIPAddressOfStream(), getStreamSocketPort(clientID));
		return false;
	}
	SI_LOG_INFO("Frontend: @#1, Start @#2 stream to @#3:@#4", _stream.getFeID(),
		_protocol, client.getIPAddressOfStream(), getStreamSocketPort(clientID));

	return true;
}

bool StreamThreadRtcpBase::pauseStreaming(const int clientID) {
	_thread.pauseThread();

	doPauseStreaming(clientID);

	const StreamClient &client = _stream.getStreamClient(clientID);
	SI_LOG_INFO("Frontend: @#1, Pause @#2 stream to @#3:@#4", _stream.getFeID(),
		_protocol, client.getIPAddressOfStream(), getStreamSocketPort(clientID));
	return true;
}

bool StreamThreadRtcpBase::restartStreaming(const int clientID) {
	_thread.restartThread();

	doRestartStreaming(clientID);

	const StreamClient &client = _stream.getStreamClient(clientID);
	SI_LOG_INFO("Frontend: @#1, Restart @#2 stream to @#3:@#4", _stream.getFeID(),
		_protocol, client.getIPAddressOfStream(), getStreamSocketPort(clientID));
	return true;
}

bool StreamThreadRtcpBase::threadExecuteFunction() {
	// RTCP compound packets must start with a SR, SDES then APP
	try {
		const auto [sr, srlen]     = getSR();
		const auto [sdes, sdeslen] = getSDES();
		const auto [app, applen]   = getAPP();

		doSendDataToClient(_clientID, sr, srlen, sdes, sdeslen, app, applen);
	} catch (const std::bad_alloc& e) {
		const StreamClient &client = _stream.getStreamClient(_clientID);
		SI_LOG_ERROR("Frontend: @#1, Error (@#2) sending @#3 data to @#4:@#5", _stream.getFeID(),
			_protocol, e.what(), client.getIPAddressOfStream(), getStreamSocketPort(_clientID));
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	const unsigned long interval = 200 * _stream.getRtcpSignalUpdateFrequency();
	std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	return true;
}

std::pair<PacketPtr, int> StreamThreadRtcpBase::getAPP() {
	const uint32_t ssrc = _stream.getSSRC();
	const std::string desc = _stream.attributeDescribeString();

	// total length and align on 32 bits
	int len = 16 + desc.size();
	if ((len % 4) != 0) {
		len += 4 - (len % 4);
	}
	// Application Defined packet  (APP Packet)
	// Alloc and copy data and adjust length
	PacketPtr app(new uint8_t[len]);
	app[0]  = 0x80;                // version: 2, padding: 0, subtype: 0
	app[1]  = 204;                 // payload type: 204 (0xcc) (APP)
	app[2]  = 0;                   // length (total in 32-bit words minus one)
	app[3]  = 0;                   // length (total in 32-bit words minus one)
	app[4]  = (ssrc >> 24) & 0xff; // synchronization source
	app[5]  = (ssrc >> 16) & 0xff; // synchronization source
	app[6]  = (ssrc >>  8) & 0xff; // synchronization source
	app[7]  = (ssrc >>  0) & 0xff; // synchronization source
	app[8]  = 'S';                 // name
	app[9]  = 'E';                 // name
	app[10] = 'S';                 // name
	app[11] = '1';                 // name
	app[12] = 0;                   // identifier (0000)
	app[13] = 0;                   // identifier
	app[14] = 0;                   // string length
	app[15] = 0;                   // string length
	                               // here the App defined data is added

	// Alloc and copy data and adjust length
	std::memcpy(app.get() + 16, desc.data(), desc.size());
	const int ws = (len / 4) - 1;
	app[2] = (ws >> 8) & 0xff;
	app[3] = (ws >> 0) & 0xff;
	const int ss = desc.size();
	app[14] = (ss >> 8) & 0xff;
	app[15] = (ss >> 0) & 0xff;
	return {std::move(app), len};
}

std::pair<PacketPtr, int> StreamThreadRtcpBase::getSR() {
	const long timestamp = _stream.getTimestamp();
	const uint32_t ssrc = _stream.getSSRC();
	const uint32_t spc = _stream.getSPC();
	const uint32_t soc = _stream.getSOC();

	// Sender Report (SR Packet)
	int len = 28;
	PacketPtr sr(new uint8_t[len]);
	sr[0]  = 0x80;                         // version: 2, padding: 0, sr blocks: 0
	sr[1]  = 200;                          // payload type: 200 (0xc8) (SR)
	sr[2]  = 0;                            // length (total in 32-bit words minus one)
	sr[3]  = 0;                            // length (total in 32-bit words minus one)
	sr[4]  = (ssrc >> 24) & 0xff;          // synchronization source
	sr[5]  = (ssrc >> 16) & 0xff;          // synchronization source
	sr[6]  = (ssrc >>  8) & 0xff;          // synchronization source
	sr[7]  = (ssrc >>  0) & 0xff;          // synchronization source

	const std::time_t ntp = std::time(nullptr);
	                                       // NTP integer part
	sr[8]  = (ntp >> 24) & 0xff;           // NTP most sign word
	sr[9]  = (ntp >> 16) & 0xff;           // NTP most sign word
	sr[10] = (ntp >>  8) & 0xff;           // NTP most sign word
	sr[11] = (ntp >>  0) & 0xff;           // NTP most sign word
	                                       // NTP fractional part
	sr[12] = 0;                            // NTP least sign word
	sr[13] = 0;                            // NTP least sign word
	sr[14] = 0;                            // NTP least sign word
	sr[15] = 0;                            // NTP least sign word

	sr[16] = (timestamp >> 24) & 0xff;     // RTP timestamp RTS
	sr[17] = (timestamp >> 16) & 0xff;     // RTP timestamp RTS
	sr[18] = (timestamp >>  8) & 0xff;     // RTP timestamp RTS
	sr[19] = (timestamp >>  0) & 0xff;     // RTP timestamp RTS
	sr[20] = (spc >> 24) & 0xff;           // sender's packet count SPC
	sr[21] = (spc >> 16) & 0xff;           // sender's packet count SPC
	sr[22] = (spc >>  8) & 0xff;           // sender's packet count SPC
	sr[23] = (spc >>  0) & 0xff;           // sender's packet count SPC
	sr[24] = (soc >> 24) & 0xff;           // sender's octet count SOC
	sr[25] = (soc >> 16) & 0xff;           // sender's octet count SOC
	sr[26] = (soc >>  8) & 0xff;           // sender's octet count SOC
	sr[27] = (soc >>  0) & 0xff;           // sender's octet count SOC

	// Alloc and copy data and adjust length
	const int ws = (len / 4) - 1;
	sr[2] = (ws >> 8) & 0xff;
	sr[3] = (ws >> 0) & 0xff;
	return {std::move(sr), len};
}

std::pair<PacketPtr, int> StreamThreadRtcpBase::getSDES() {
	const uint32_t ssrc = _stream.getSSRC();

	// Source Description (SDES Packet)
	int len = 20;
	PacketPtr sdes(new uint8_t[len]);
	sdes[0]  = 0x81;                           // version: 2, padding: 0, sc blocks: 1
	sdes[1]  = 202;                            // payload type: 202 (0xca) (SDES)
	sdes[2]  = 0;                              // length (total in 32-bit words minus one)
	sdes[3]  = 3;                              // length (total in 32-bit words minus one)

	sdes[4]  = (ssrc >> 24) & 0xff;            // synchronization source
	sdes[5]  = (ssrc >> 16) & 0xff;            // synchronization source
	sdes[6]  = (ssrc >>  8) & 0xff;            // synchronization source
	sdes[7]  = (ssrc >>  0) & 0xff;            // synchronization source

	sdes[8]  = 1;                              // CNAME: 1
	sdes[9]  = 6;                              // length: 6
	sdes[10] = 'S';                            // data
	sdes[11] = 'a';                            // data

	sdes[12] = 't';                            // data
	sdes[13] = 'P';                            // data
	sdes[14] = 'I';                            // data
	sdes[15] = 0;                              // data

	sdes[16] = 0;                              // data
	sdes[17] = 0;                              // data
	sdes[18] = 0;                              // data
	sdes[19] = 0;                              // data

	const int ws = (len / 4) - 1;
	sdes[2] = (ws >> 8) & 0xff;
	sdes[3] = (ws >> 0) & 0xff;
	return {std::move(sdes), len};
}

}
