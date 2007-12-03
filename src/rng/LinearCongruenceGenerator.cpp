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

#include <WNS/rng/LinearCongruenceGenerator.hpp>

using namespace wns::rng;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	LinearCongruenceGenerator,
	wns::rng::RNGen,
	"wns.rng.LinearCongruenceGenerator",
	wns::PyConfigViewCreator);

LinearCongruenceGenerator::LinearCongruenceGenerator(const wns::pyconfig::View& configuration) :
    seed_(configuration.get<uint32_t>("seed")),
    state_(seed_)
{
	reset();
}

LinearCongruenceGenerator::~LinearCongruenceGenerator()
{
}

void
LinearCongruenceGenerator::reset()
{
    state_ = seed_;
}

uint32_t
LinearCongruenceGenerator::asLong32()
{
    // minimal standard due to Park and Miller (1986)
    // implementation uses the algorithm of Schrage (1979)
    // to avoid a possible overflow when calculating a * Xn-1

    int32_t k;

    k = state_ / 127773;
    state_ = (16807 * (state_ - (k * 127773))) - (2836 * k);

    if (state_ < 0)
    {
		state_ += 2147483647;
    }
    return state_;
}

void
LinearCongruenceGenerator::seed(uint32_t newSeed)
{
    seed_ = newSeed;
    // a seed of 0 is not allowed!
    if (0 == seed_)
    {
        ++seed_;
    }
    reset();
}

bool
LinearCongruenceGenerator::hasLong32()
{
    return false;
}

/*
  Local Variables:
  mode: c++
  folded-file: t
  End:
*/
