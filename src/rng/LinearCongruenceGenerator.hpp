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

#ifndef WNS_RNG_LINEARCONRUGENCEGENERATOR_HPP
#define WNS_RNG_LINEARCONRUGENCEGENERATOR_HPP

#include <WNS/rng/RNGen.hpp>

namespace wns { namespace rng {

    /**
     * @brief Linear Congruence Random Number Generator
     */
    class LinearCongruenceGenerator :
        public RNGen
    {
    public:
        explicit
        LinearCongruenceGenerator(const wns::pyconfig::View&);

        virtual
        ~LinearCongruenceGenerator();

        virtual uint32_t
        asLong32();

        virtual void
        reset();

        virtual bool
        hasLong32();

        virtual void
        seed(uint32_t newSeed);

    private:
        uint32_t seed_;
        int32_t state_;
    };

} // rng
} // wns

#endif  // NOT defined WNS_RNG_LINEARCONRUGENCEGENERATOR_HPP
