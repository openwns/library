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

#ifndef WNS_SCHEDULER_TESTS_LAPROXYSTUB_HPP
#define WNS_SCHEDULER_TESTS_LAPROXYSTUB_HPP

#include <WNS/scheduler/ILinkAdaptationProxy.hpp>

namespace wns { namespace scheduler { namespace tests {

        /** @brief this class is used for tests only */
    class LAProxyStub : 
        public ILinkAdaptationProxy 
    {
    public:
        LAProxyStub();
        ~LAProxyStub();

        wns::service::phy::phymode::PhyModeMapperInterface* 
        getPhyModeMapper() const;

        wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> 
        getBestPhyMode(const wns::Ratio& sinr);

        ChannelQualityOnOneSubChannel 
        estimateTxSINRAt(const UserID user, int slot);

        ChannelQualityOnOneSubChannel 
        estimateRxSINROf(const UserID user, int slot);

        void
        estimateTxSINRAt(ChannelQualityOnOneSubChannel cq);

        void
        estimateRxSINROf(ChannelQualityOnOneSubChannel cq);


    private:
        wns::service::phy::phymode::PhyModeMapperInterface* phyModeMapper_;
        ChannelQualityOnOneSubChannel txCQ_;
        ChannelQualityOnOneSubChannel rxCQ_;
    };
} // tests
} // scheduler
} // wns
#endif // WNS_SCHEDULER_TESTS_LAPROXYSTUB_HPP


