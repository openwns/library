/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2009
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

#ifndef WNS_SCHEDULER_STRATEGY_STATICPRIORITY_PERSISTENTVOIP_RESOURCEGRID_HPP
#define WNS_SCHEDULER_STRATEGY_STATICPRIORITY_PERSISTENTVOIP_RESOURCEGRID_HPP

namespace wns { namespace scheduler { namespace strategy { namespace staticpriority { namespace persistentvoip {


class TransmissionBlock
{
    public:
        int start;
        int length;
        bool neverUsed;
        ConnectionSet previousCIDs;    
        std::map<ConnectionSet, wns::simulator::Time> lastUse;
        
        TransmissionBlock(int s, int l):
            start(s),
            length(l),
            neverUsed(true)
        {
        };

        bool
        operator<(const TransmissionBlock& other) const
        {
            return this->start < other.start;
        };    
}; 

}}}}}

#endif
