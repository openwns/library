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

#ifndef WNS_PROBE_BUS_MASTERPROBEBUS_HPP
#define WNS_PROBE_BUS_MASTERPROBEBUS_HPP

#include <WNS/probe/bus/ProbeBus.hpp>
#include <WNS/pyconfig/View.hpp>

namespace wns { namespace probe { namespace bus {
    /**
     * @brief The MasterProbeBus publishes all Measurements available.
     *
     * If you want to receive messages implement the ProbeBus Interface and
     * use the startObserving method on the MasterProbeBus to receive measurements.
     * You may also use existing general purpose implementations already availabel.
     *
     * @author Daniel Bültmann <dbn@comnets.rwth-aachen.de>
     */
    class MasterProbeBus:
        virtual public ProbeBus
    {
    public:
        MasterProbeBus(const wns::pyconfig::View&);

        virtual ~MasterProbeBus() {}

        virtual bool
        accepts(const wns::simulator::Time&, const IContext&);

        virtual void
        onMeasurement(const wns::simulator::Time&,
                      const double&,
                      const IContext&);

        virtual void
        output();

    };
} // bus
} // probe
} // wns

#endif // WNS_PROBE_BUS_MASTERPROBEBUS_HPP
