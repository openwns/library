/******************************************************************************
 * WNS (Wireless Network Simulator)                                           *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2004-2006                                                    *
 * Chair of Communication Networks (ComNets)                                  *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
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
            StartStopReceivingCommand(TimeWindowProbeBus* _who, ProbeBus* _other, bool _starting) :
                who(_who),
                other(_other),
                starting(_starting)
                {
                }

            virtual void operator()()
                {
                    if (starting)
                    {
                        who->wns::probe::bus::ProbeBus::startReceiving(other);
                    }
                    else
                    {
                        who->wns::probe::bus::ProbeBus::stopReceiving(other);
                    }
                }
            virtual
            ~StartStopReceivingCommand()
            {}

        private:
            TimeWindowProbeBus* who;

            ProbeBus* other;

            bool starting;
        };

        wns::events::scheduler::Interface* evsched;

        wns::simulator::Time start;

        wns::simulator::Time end;
    };
} // bus
} // probe
} // wns

#endif // WNS_PROBE_BUS_TIMEWINDOWPROBEBUS_HPP
