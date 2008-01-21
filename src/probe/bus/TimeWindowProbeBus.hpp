/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 16, D-52074 Aachen, Germany
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

#ifndef WNS_PROBE_BUS_TIMEWINDOWPROBEBUS_HPP
#define WNS_PROBE_BUS_TIMEWINDOWPROBEBUS_HPP

#include <WNS/probe/bus/ProbeBus.hpp>

namespace wns { namespace probe { namespace bus {

    class TimeWindowProbeBus :
        public wns::probe::bus::ProbeBus
    {
    public:

        TimeWindowProbeBus(const wns::pyconfig::View&);

        virtual ~TimeWindowProbeBus();

        virtual void
        onMeasurement(const wns::simulator::Time&,
                      const double&,
                      const IContext&);

        virtual bool
        accepts(const wns::simulator::Time&, const IContext&);

        virtual void
        output();

        virtual void
        startReceiving(ProbeBus* other);
    private:

        class StartStopReceivingCommand
        {
        public:
            StartStopReceivingCommand(TimeWindowProbeBus* who,
                                      ProbeBus* other,
                                      bool starting) :
                who_(who),
                other_(other),
                starting_(starting)
                {
                }

            virtual void operator()()
                {
                    if (starting_)
                    {
                        who_->wns::probe::bus::ProbeBus::startReceiving(other_);
                    }
                    else
                    {
                        who_->wns::probe::bus::ProbeBus::stopReceiving(other_);
                    }
                }
            virtual
            ~StartStopReceivingCommand()
            {}

        private:
            TimeWindowProbeBus* who_;

            ProbeBus* other_;

            bool starting_;
        };

        wns::events::scheduler::Interface* evsched_;

        wns::simulator::Time start_;

        wns::simulator::Time end_;
    };
} // bus
} // probe
} // wns

#endif // WNS_PROBE_BUS_TIMEWINDOWPROBEBUS_HPP
