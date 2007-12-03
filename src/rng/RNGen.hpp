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

#ifndef WNS_RNG_RNGEN_HPP
#define WNS_RNG_RNGEN_HPP

#include <WNS/PyConfigViewCreator.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/StaticFactory.hpp>

namespace wns { namespace rng {

union PrivateRNGSingleType              // used to access floats as unsigneds
{
    float s;
    uint32_t u;
};

union PrivateRNGDoubleType  	   	// used to access doubles as unsigneds
{
    double d;
    uint32_t u[2];
};

/*
 * The class RNGen
 */
/*! \brief Class \bRNGen: Abstract base class for random number generators */
class RNGen
{
public:
    RNGen() throw();
    virtual ~RNGen();

    //! Return a long-words word of random bits
    virtual uint32_t asLong32() = 0;
    virtual bool     hasLong32() = 0;
    //! Return a 31bit long word of random bits
    virtual uint32_t asLong();

    virtual void     reset() = 0;
    virtual void     seed(uint32_t aNewSeed);

    //! Return random bits converted to either a float or a double
    virtual float    asFloat();
    virtual double   asDouble();

private:

    static PrivateRNGSingleType singleMantissa;	//mantissa bit vector
    static PrivateRNGDoubleType doubleMantissa;	//mantissa bit vector

    // Random number generator has been initialized?
    static bool initializedFlag;

    // Big or little endian machine?
    static bool bigEndianMachineFlag;
};
	typedef PyConfigViewCreator<RNGen> RNGCreator;
	typedef StaticFactory<RNGCreator> RNGFactory;

} // rng
} // wns

#endif  // WNS_RNG_RNGEN_HPP

/*
  Local Variables:
  mode: c++
  folded-file: t
  End:
*/
