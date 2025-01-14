/* PidTable.h

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
#ifndef MPEGTS_PIDTABLE_H_INCLUDE
#define MPEGTS_PIDTABLE_H_INCLUDE MPEGTS_PIDTABLE_H_INCLUDE

#include <cstdint>
#include <string>

namespace mpegts {

/// The class @c PidTable carries all the PID and DMX information
class PidTable {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		PidTable();

		virtual ~PidTable() = default;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		/// Clear all PID data
		void clear();

		/// Reset that PID has changed
		void resetPIDTableChanged();

		/// Check if the PID has changed
		bool hasPIDTableChanged() const {
			return _changed;
		}

		/// Get the amount of packet that were received of this pid
		uint32_t getPacketCounter(int pid) const;

		/// Get the amount Continuity Counter Error of this pid
		uint32_t getCCErrors(int pid) const;

		/// Get the total amount of Continuity Counter Error
		uint32_t getTotalCCErrors() const {
			return _totalCCErrors - _totalCCErrorsBegin;
		}

		/// Get the CSV of all the requested PID
		std::string getPidCSV() const;

		/// Set the continuity counter for pid
		void addPIDData(int pid, uint8_t cc);

		/// Set pid used or not
		void setPID(int pid, bool use);

		/// Check if this pid is opened
		bool isPIDOpened(int pid) const {
			return _data[pid].state == State::Opened;
		}

		/// Check if this pid should be closed
		bool shouldPIDClose(int pid) const;

		/// Set that this pid is closed
		void setPIDClosed(int pid);

		/// Check if PID should be opened
		bool shouldPIDOpen(int pid) const;

		/// Set that this pid is opened
		void setPIDOpened(int pid);

		/// Set all PID
		void setAllPID(bool use);

		/// Check if all PIDs (full Transport Stream) is on
		bool isAllPID() const {
			return _data[ALL_PIDS].state == State::Opened;
		}

	protected:

		/// Reset the pid data like counters etc.
		void resetPidData(int pid);

		// =========================================================================
		//  -- Data members --------------------------------------------------------
		// =========================================================================
	public:

		static constexpr int MAX_PIDS = 8193;
		static constexpr int ALL_PIDS = 8192;

	protected:

	private:

		enum class State {
			ShouldOpen,
			Opened,
			ShouldClose,
			ShouldCloseReopen,
			Closed
		};

		// PID State
		struct PidData {
			State state;
			uint8_t cc;        /// continuity counter (0 - 15) of this PID
			uint32_t cc_error; /// cc error count
			uint32_t count;    /// the number of times this pid occurred
		};
		uint32_t _totalCCErrors;
		uint32_t _totalCCErrorsBegin;
		bool _changed;
		PidData _data[MAX_PIDS];
};

}

#endif // MPEGTS_PIDTABLE_H_INCLUDE
