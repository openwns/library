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

#ifndef WNS_PROBE_BUS_PROBEBUSREGISTRY_HPP
#define WNS_PROBE_BUS_PROBEBUSREGISTRY_HPP

#include <WNS/probe/bus/ProbeBus.hpp>

#include <WNS/container/Registry.hpp>
#include <WNS/pyconfig/View.hpp>

namespace wns { namespace probe { namespace bus {

    typedef wns::container::Registry<std::string, ProbeBus*, wns::container::registry::DeleteOnErase> ProbeBusRegistryContainer;

    /**
     * @brief The ProbeBusRegistry resolves a ProbeBus name to a ProbeBus
     * Prototype which may then be used as the entry point for measurements to
     * the ProbeBus system.
     *
     * @author Daniel Bültmann <me@daniel-bueltmann.de>
     */
    class ProbeBusRegistry
    {
    public:
        ProbeBusRegistry(const wns::pyconfig::View&);

        virtual ~ProbeBusRegistry();

        /**
         * @brief Retrieves an instance of a ProbeBus. The ProbeBus returned is
         * an instance of the Prototype given in the configuration.
         *
         * If the given name is asked for the first time the ProbeBusRegistry
         * creates a ProbeBus from the Prototyp. Subsequent requests for the
         * same ProbeBus then alwys get the same instance. In this way
         * measurement sources and measurement sinks are decoupled.
         */
        ProbeBus*
        getProbeBus(const std::string&);

        /**
         * @brief All known ProbeBusses are triggered to write their results to
         * persistent storage.
         */
        void
        forwardOutput();

    private:
        wns::pyconfig::View protoConf_;

        ProbeBusRegistryContainer registry_;
    };
} // bus
} // probe
} // wns

#endif //WNS_PROBE_BUS_PROBEBUSREGISTRY_HPP
