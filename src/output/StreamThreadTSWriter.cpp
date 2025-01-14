/* StreamThreadTSWriter.cpp

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
#include <output/StreamThreadTSWriter.h>

#include <InterfaceAttr.h>
#include <Log.h>
#include <StreamInterface.h>
#include <StreamClient.h>
#include <Unused.h>
#include <base/TimeCounter.h>

#include <chrono>
#include <thread>

namespace output {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamThreadTSWriter::StreamThreadTSWriter(
	StreamInterface &stream,
	const std::string &file) :
	StreamThreadBase("TSWRITER", stream),
	_filePath(file) {}

StreamThreadTSWriter::~StreamThreadTSWriter() {
	terminateThread();
}

// =============================================================================
//  -- output::StreamThreadBase ------------------------------------------------
// =============================================================================

void StreamThreadTSWriter::doStartStreaming(int UNUSED(clientID)) {
	_file.open(_filePath, std::ofstream::binary);
}

bool StreamThreadTSWriter::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &UNUSED(client)) {
	const unsigned char *tsBuffer = buffer.getTSReadBufferPtr();

	const long timestamp = base::TimeCounter::getTicks() * 90;
	const size_t dataSize = buffer.getCurrentBufferSize();

	// RTP packet octet count (Bytes)
	_stream.addRtpData(dataSize, timestamp);

	// write TS packets to file
	if (_file.is_open()) {
		_file.write(reinterpret_cast<const char *>(tsBuffer), dataSize);
	}
	return true;
}

} // namespace output
