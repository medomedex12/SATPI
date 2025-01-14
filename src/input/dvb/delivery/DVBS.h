/* DVBS.h

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
#ifndef INPUT_DVB_DELIVERY_DVBS_H_INCLUDE
#define INPUT_DVB_DELIVERY_DVBS_H_INCLUDE INPUT_DVB_DELIVERY_DVBS_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <input/dvb/delivery/System.h>

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <map>

FW_DECL_UP_NS3(input, dvb, delivery, DiSEqc);

namespace input::dvb::delivery {

/// The class @c DVBS specifies DVB-S/S2 delivery system
class DVBS :
	public input::dvb::delivery::System {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		explicit DVBS(FeIndex index, FeID id, const std::string &fePath, unsigned int dvbVersion);
		virtual ~DVBS() = default;

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =========================================================================
		// -- input::dvb::delivery::System -----------------------------------------
		// =========================================================================
	public:

		virtual bool tune(
			int feFD,
			const input::dvb::FrontendData &frontendData) final;

		virtual bool isCapableOf(input::InputSystem system) const final {
			return system == input::InputSystem::DVBS2X ||
				system == input::InputSystem::DVBS2 ||
				system == input::InputSystem::DVBS;
		}

		///
		virtual void teardown(int feFD) const;

		// =========================================================================
		// -- Other member functions -----------------------------------------------
		// =========================================================================
	private:

		///
		bool setProperties(int feFD, uint32_t freq, const input::dvb::FrontendData &frontendData);

		// =========================================================================
		// -- FBC member functions -------------------------------------------------
		// =========================================================================
		///
		int readProcData(FeIndex index, const std::string &procEntry) const;

		///
		void writeProcData(FeIndex index, const std::string &procEntry, int value);

		///
		void readConnectionChoices(FeIndex index, int offset);

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		enum class DiseqcType {
			Switch,
			EN50494,
			EN50607,
			Lnb
		};
		DiseqcType _diseqcType;
		UpDiSEqc _diseqc;
		bool _turnoffLnbVoltage;
		bool _higherLnbVoltage;

		// =========================================================================
		// -- FBC Data members -----------------------------------------------------
		// =========================================================================
		using ConnectionChoices = std::map<int, std::string>;
		ConnectionChoices _choices;
		bool _fbcTuner;
		bool _fbcRoot;
		int _fbcSetID;
		int _fbcConnect;
		bool _fbcLinked;
		bool _sendDiSEqcViaRootTuner;

};

}

#endif // INPUT_DVB_DELIVERY_DVBS_H_INCLUDE
