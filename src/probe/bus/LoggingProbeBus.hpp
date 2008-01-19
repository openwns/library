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

#ifndef WNS_PROBE_BUS_LOGGINGPROBEBUS_HPP
#define WNS_PROBE_BUS_LOGGINGPROBEBUS_HPP

#include <WNS/probe/bus/ProbeBus.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>

namespace wns { namespace probe { namespace bus {

    /**
     * @brief A logger for the ProbeBus
     *
     * Attach the LoggingProbeBus to another ProbeBus to see the Measurements that pass that ProbeBus.
     * The measurements are printed to std::out. Use this for debugging purposes.
     */
    class LoggingProbeBus:
        public ProbeBus
    {
    public:
        LoggingProbeBus(const wns::pyconfig::View&);

        virtual ~LoggingProbeBus();

        virtual void
        onMeasurement(const wns::simulator::Time&,
                      const double&, const IContext&);

        virtual bool
        accepts(const wns::simulator::Time&, const IContext&);

        virtual void
        output();

    private:
        wns::logger::Logger logger;
    };
} // bus
} // probe
} // wns
#endif //WNS_PROBE_BUS_LOGGINGPROBEBUS_HPP
