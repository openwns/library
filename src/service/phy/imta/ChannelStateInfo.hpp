/*******************************************************************************
 * This file is part of IMTAphy / openWNS
 * _____________________________________________________________________________
 *
 * Copyright (C) 2010
 * Institute of Communication Networks (LKN)
 * Department of Electrical Engineering and Information Technology (EE & IT)
 * Technische Universitaet Muenchen
 * Arcisstr. 21
 * 80333 Muenchen - Germany
 * 
 * _____________________________________________________________________________
 *
 *   IMTAphy is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   IMTAphy is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with IMTAphy.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifndef WNS_SERVICE_PHY_CHANNELSTATEINFO_HPP
#define WNS_SERVICE_PHY_CHANNELSTATEINFO_HPP

#include <WNS/RefCountable.hpp>
#include <WNS/service/phy/imta/IMTAphyObserver.hpp>
#include <WNS/PowerRatio.hpp>
#include <map>


namespace wns { namespace service { namespace phy { namespace imta {

                typedef unsigned int PRB;
                typedef std::list<PRB> PRBList;
                typedef std::map<PRB, wns::Ratio> PRBMapdB;
                typedef std::map<PRB, wns::Power> PRBMapdBm;
	 

                class ChannelStateInfo :
            public wns::RefCountable
                {
                public:
                    PRBMapdB  sinrs;
                    PRBMapdBm rxPowers;
                };

                typedef wns::SmartPtr<ChannelStateInfo> ChannelStateInfoPtr;
       
            }
        }
    }
}

#endif
