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

#ifndef WNS_SCHEDULER_STRATEGY_STATICPRIORITY_PERSISTENTVOIP_HPP
#define WNS_SCHEDULER_STRATEGY_STATICPRIORITY_PERSISTENTVOIP_HPP

#include <WNS/scheduler/strategy/staticpriority/SubStrategy.hpp>
#include <WNS/scheduler/strategy/staticpriority/persistentvoip/ResourceGrid.hpp>
#include <WNS/scheduler/strategy/Strategy.hpp>
#include <WNS/scheduler/SchedulingMap.hpp>
#include <WNS/scheduler/queue/QueueInterface.hpp>
#include <WNS/scheduler/RegistryProxyInterface.hpp>
#include <WNS/StaticFactory.hpp>

namespace wns { namespace scheduler { namespace strategy { namespace staticpriority {


class PersistentVoIP
        : public SubStrategy
{
    public:
        PersistentVoIP(const wns::pyconfig::View& config);

        ~PersistentVoIP();

        virtual void
        initialize();

        virtual wns::scheduler::MapInfoCollectionPtr
        doStartSubScheduling(SchedulerStatePtr schedulerState,
                             wns::scheduler::SchedulingMapPtr schedulingMap);

    private:
        void
        updateState(const ConnectionSet activeConnections);

        void 
        onFirstScheduling(const SchedulerStatePtr& schedulerState);

        bool firstScheduling_;
        int numberOfSubchannels_;

        unsigned int numberOfFrames_;
        unsigned int currentFrame_;

        std::vector<ConnectionSet> expectedCIDs_;
        std::vector<ConnectionSet> pastPeriodCIDs_;
        ConnectionSet silentCIDs_;
        ConnectionSet allCIDs_;

        persistentvoip::ResourceGrid* resources_;

        wns::pyconfig::View resourceGridConfig_;
};

}}}}

#endif
