/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 5, D-52074 Aachen, Germany
 * phone: ++49-241-80-27910,
 * fax: ++49-241-80-22242
 * email: info@openwns.org
 * www: http://www.openwns.org
 * _____________________________________________________________________________
 *
 * openWNS is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 2 as published by the
 * Free Software Foundation;
 *
 * openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifndef _CANDI_HPP
#define _CANDI_HPP

#include <WNS/PowerRatio.hpp>
#include <cmath>
namespace wns
{
    /**
     *  @brief class to encapsulate carrier and interference power levels separately
	 *  for SDMA purposes intra-cell interference can be given separately
     */
    class CandI
    {
    public:
		// Default constructor
		CandI() :
			C(wns::Power::from_mW(0.0)),
			I(wns::Power::from_mW(0.0))
			{
				sdma.iIntra = wns::Power::from_mW(0.0);
			}

		CandI(wns::Power carrier, wns::Power interference) :
			C(carrier),
			I(interference)
			{
				sdma.iIntra = wns::Power::from_mW(0.0);
			}

		bool operator <(const CandI& candi) const {
			return ( (C/I) < (candi.C / candi.I) );
		}

 		// Default destructor
 		~CandI(){}
		wns::Power C;
		wns::Power I;

		struct {
			// estimated intra-cell interference
			wns::Power iIntra;
		} sdma;
	};
}
#endif // _CANDI_HPP