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

#ifndef WNS_SERVICE_PHY_IMTA_DATATRANSMISSION_HPP
#define WNS_SERVICE_PHY_IMTA_DATATRANSMISSION_HPP

#include <WNS/node/Node.hpp>
#include <WNS/PowerRatio.hpp>
#include <WNS/osi/PDU.hpp>
#include <WNS/service/Service.hpp>
#include <WNS/service/phy/imta/ChannelStateInfo.hpp>
#include <vector>

namespace wns { namespace service { namespace phy { namespace imta {
	
   
 
                class DataTransmission :
            public virtual service::Service {
                public:
                    virtual void registerTransmission(wns::node::Interface* destination,
                                                      // later: 
                                                      // - precoding (vector/matrix),
                                                      // - phymode
                                                      wns::Power txPower,
                                                      wns::osi::PDUPtr transportBlock,
                                                      PRBList transmitPRBs,
                                                      unsigned int TTI) = 0; 
                };
	
            }}}}
#endif
