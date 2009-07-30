/******************************************************************************
 * WNS (Wireless Network Simulator)                                           *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2004-2008                                                    *
 * Chair of Communication Networks (ComNets)                                  *
 * Kopernikusstr. 1, D-52074 Aachen, Germany                                  *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
 ******************************************************************************/

#ifndef WNS_LDK_DELIVERERRECEPTACLEREGISTRY_HPP
#define WNS_LDK_DELIVERERRECEPTACLEREGISTRY_HPP

#include <WNS/ldk/IDelivererReceptacle.hpp>

#include <WNS/container/Registry.hpp>

#include <string>

namespace wns { namespace ldk {

        class DelivererReceptacleRegistry
        {
        public:
            DelivererReceptacleRegistry()
                : theRegistry_()
            {}

            virtual
            ~DelivererReceptacleRegistry()
            {}

            IDelivererReceptacle*
            getFromDelivererReceptacleRegistry(std::string portname)
            {
                return theRegistry_.find(portname);
            }

        protected:
            void
            addToDelivererReceptacleRegistry(std::string portname, IDelivererReceptacle* cr)
            {
                theRegistry_.insert(portname, cr);
            }

        private:
            container::Registry<std::string, IDelivererReceptacle*> theRegistry_;
        };
    } // ldk
} // wns

#endif // WNS_LDK_DELIVERERRECEPTACLEREGISTRY_HPP
