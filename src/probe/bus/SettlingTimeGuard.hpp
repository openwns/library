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

#ifndef WNS_PROBE_BUS_SETTLINGTIMEGUARD_HPP
#define WNS_PROBE_BUS_SETTLINGTIMEGUARD_HPP

#include <WNS/probe/bus/ProbeBus.hpp>

namespace wns { namespace probe { namespace bus {

    class SettlingTimeGuard :
        public wns::probe::bus::ProbeBus
    {
    public:

        SettlingTimeGuard(const wns::pyconfig::View&);

        virtual ~SettlingTimeGuard();

        virtual void
        onMeasurement(const wns::simulator::Time&,
                      const double&,
                      const IContext&);

        virtual bool
        accepts(const wns::simulator::Time&, const IContext&);

        virtual void
        output();

	private:
		wns::simulator::Time settlingTime_;
    };
} // bus
} // probe
} // wns

#endif // WNS_PROBE_BUS_SETTLINGTIMEGUARD_HPP
