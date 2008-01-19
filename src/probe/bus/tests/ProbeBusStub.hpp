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

#ifndef WNS_PROBE_BUS_TEST_PROBEBUSSTUB_HPP
#define WNS_PROBE_BUS_TEST_PROBEBUSSTUB_HPP

#include <WNS/probe/bus/ProbeBus.hpp>

namespace wns { namespace probe { namespace bus { namespace tests {

    /**
     * @brief ProbeBusStub. ProbeBus for testing
     * @author Daniel Bültmann <dbn@comnets.rwth-aachen.de>
     */
    class ProbeBusStub:
        public wns::probe::bus::ProbeBus
    {
    public:
        ProbeBusStub();

        virtual ~ProbeBusStub();

        virtual void
        onMeasurement(const wns::simulator::Time&,
                      const double&,
                      const wns::probe::bus::IContext&);

        virtual void
        output();

        virtual bool
        accepts(const wns::simulator::Time&, const IContext&);

        virtual void
        setFilter(std::string, int32_t);

        int receivedCounter;

        std::vector<double> receivedTimestamps;

        std::vector<double> receivedValues;

        std::string providerName;

        int32_t filter;

    };
} // tests
} // bus
} // probe
} // wns

#endif //WNS_PROBE_TEST_PROBEBUSSTUB_HPP
