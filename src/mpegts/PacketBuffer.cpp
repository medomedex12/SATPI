/* PacketBuffer.cpp

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
#include <mpegts/PacketBuffer.h>

#include <Log.h>

#include <cassert>
#include <cstring>
#include <climits>

namespace mpegts {

static_assert(PacketBuffer::MTU_MAX_TS_PACKET_SIZE < PacketBuffer::MTU, "TS Packet size bigger then MTU");
static_assert(CHAR_BIT == 8, "Error CHAR_BIT != 8");

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void PacketBuffer::initialize(const uint32_t ssrc, const long timestamp) {
	// initialize RTP header
	_buffer[0]  = 0x80;                     // version: 2, padding: 0, extension: 0, CSRC: 0
	_buffer[1]  = 33;                       // marker: 0, payload type: 33 (MP2T)
	_buffer[2]  = (0 >> 8) & 0xff;          // sequence number
	_buffer[3]  = (0 >> 0) & 0xff;          // sequence number
	_buffer[4]  = (timestamp >> 24) & 0xff; // timestamp
	_buffer[5]  = (timestamp >> 16) & 0xff; // timestamp
	_buffer[6]  = (timestamp >>  8) & 0xff; // timestamp
	_buffer[7]  = (timestamp >>  0) & 0xff; // timestamp
	_buffer[8]  = (ssrc >> 24) & 0xff;      // synchronization source
	_buffer[9]  = (ssrc >> 16) & 0xff;      // synchronization source
	_buffer[10] = (ssrc >>  8) & 0xff;      // synchronization source
	_buffer[11] = (ssrc >>  0) & 0xff;      // synchronization source

	_initialized = true;
}

bool PacketBuffer::trySyncing() {
	const std::size_t size = getCurrentBufferSize();
	if (size < (TS_PACKET_SIZE * 3)) {
		return false;
	}
	if (isSynced()) {
		return true;
	}
	for (size_t i = RTP_HEADER_LEN; i < MTU_MAX_TS_PACKET_SIZE + RTP_HEADER_LEN - (TS_PACKET_SIZE * 2); ++i) {
		const unsigned char *cData = _buffer + i;
		if (cData[TS_PACKET_SIZE * 0] == 0x47 &&
			cData[TS_PACKET_SIZE * 1] == 0x47 &&
			cData[TS_PACKET_SIZE * 2] == 0x47) {
			// found sync, now move it to begin of buffer
			const size_t cpySize = (MTU_MAX_TS_PACKET_SIZE + RTP_HEADER_LEN) - i;
			_writeIndex = _writeIndex - (i - RTP_HEADER_LEN);
			_processedIndex = _writeIndex;
			std::memmove(_buffer + RTP_HEADER_LEN, cData, cpySize);
			return true;
		}
	}
	// did not find a sync, so flush buffer
	reset();
	return false;
}

void PacketBuffer::markTSForPurging(std::size_t packetNumber) {
	if (packetNumber <= NUMBER_OF_TS_PACKETS) {
		// Invalid TS packets are labeled 0xFF _after_ the first SYNC Byte.
		unsigned char *cData = getTSPacketPtr(packetNumber);
		cData[1] = 0xFF;
		++_purgePending;
	}
}

void PacketBuffer::purge() {
	if (_purgePending == 0) {
		return;
	}
	_purgePending = 0;
	const std::size_t bufSize = getCurrentBufferSize();
	if (bufSize == 0) {
		return;
	}
	// i: represents the packet number, and not the packet position!
	std::size_t skipData = 0;
	for (std::size_t i = (bufSize / TS_PACKET_SIZE); i > 0 ; --i) {
		unsigned char *cData = getTSPacketPtr(i - 1);
		if (cData[1] == 0xFF) {
			// Next packet needs to be purged as well, then continue to next
			if (i > 1 && (cData - TS_PACKET_SIZE)[1] == 0xFF) {
				skipData += TS_PACKET_SIZE;
				continue;
			}
			skipData += TS_PACKET_SIZE;
			// Remove current (plus other) packet from the buffer
			unsigned char *endData = getWriteBufferPtr();
			unsigned char *nextData = cData + skipData;
			if (nextData < endData) {
				std::memcpy(cData, nextData, endData - nextData);
			}
			_writeIndex -= skipData;
			skipData = 0;
		}
	}
}

void PacketBuffer::tagRTPHeaderWith(const uint16_t cseq, const long timestamp) {
	// update sequence number
	_buffer[2] = ((cseq >> 8) & 0xFF); // sequence number
	_buffer[3] =  (cseq & 0xFF);       // sequence number

	// update timestamp
	_buffer[4] = (timestamp >> 24) & 0xFF; // timestamp
	_buffer[5] = (timestamp >> 16) & 0xFF; // timestamp
	_buffer[6] = (timestamp >>  8) & 0xFF; // timestamp
	_buffer[7] = (timestamp >>  0) & 0xFF; // timestamp
}

}
