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

#ifndef WNS_PROBE_BUS_PROBEBUSREGISTRY_HPP
#define WNS_PROBE_BUS_PROBEBUSREGISTRY_HPP

#include <WNS/probe/bus/ProbeBus.hpp>

#include <WNS/container/Registry.hpp>
#include <WNS/pyconfig/View.hpp>

namespace wns { namespace probe { namespace bus {

    typedef wns::container::Registry<std::string, ProbeBus*, wns::container::registry::DeleteOnErase> ProbeBusRegistryContainer;

    class ProbeBusRegistry
    {
    public:
        ProbeBusRegistry(const wns::pyconfig::View&);

        virtual ~ProbeBusRegistry();

        ProbeBus*
        getProbeBus(const std::string&);

        void
        forwardOutput();

    private:
        wns::pyconfig::View protoConf;

        ProbeBusRegistryContainer registry;
    };
} // bus
} // probe
} // wns

#endif //WNS_PROBE_BUS_PROBEBUSREGISTRY_HPP
