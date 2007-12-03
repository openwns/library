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

#include <WNS/rng/RNGen.hpp>

using namespace std;
using namespace wns::rng;

PrivateRNGSingleType RNGen::singleMantissa;	//mantissa bit vector
PrivateRNGDoubleType RNGen::doubleMantissa;	//mantissa bit vector

RNGen::RNGen() throw()
{
    if (!initializedFlag)
    {
		assert (sizeof(double) == 2 * sizeof(uint32_t));

		//
		//	The following is a hack that I attribute to
		//	Andres Nowatzyk at CMU. The intent of the loop
		//	is to form the smallest number 0 <= x < 1.0,
		//	which is then used as a mask for two longwords.
		//	This gives us a fast way way to produce double
		//	precision numbers from longwords.
		//
		//	I know that this works for IEEE and VAX floating
		//	point representations.
		//
		//	A further complication is that gnu C will blow
		//	the following loop, unless compiled with -ffloat-store,
		//	because it uses extended representations for some of
		//	of the comparisons. Thus, we have the following hack.
		//	If we could specify #pragma optimize, we wouldn't need this.
		//

		PrivateRNGDoubleType local_double_type;
		PrivateRNGSingleType local_single_type;

		local_double_type.d = 1.5;
		if (local_double_type.u[1] == 0)
		{
			bigEndianMachineFlag= true;
		}
		else
		{
			bigEndianMachineFlag= false;
		}
#if 0
//#if _IEEE == 1

		if (bigEndianMachineFlag)
		{
			local_double_type.u[0] = 0x3fffffff;
			local_double_type.u[1] = 0xffffffff;
		}
		else
		{
			local_double_type.u[0] = 0xffffffff;
			local_double_type.u[1] = 0x3fffffff;
		}

		local_single_type.u = 0x3fffffff;
#else
		double y = 0.5;
		volatile double x = 1.0; // volatile needed when fp hardware used,
		// and has greater precision than memory doubles
		do {			    // find largest fp-number < 2.0
			local_double_type.d = x;
			x += y;
			y *= 0.5;
		} while (x != local_double_type.d && x < 2.0);

		float yy = 0.5;
		volatile float xx = 1.0; // volatile needed when fp hardware used,
		// and has greater precision than memory floats
		do {			    // find largest fp-number < 2.0
			local_single_type.s = xx;
			xx += yy;
			yy *= 0.5;
		} while (xx != local_single_type.s && xx < 2.0);
#endif
		// set doubleMantissato 1 for each doubleMantissabit
		doubleMantissa.d= 1.0;
		doubleMantissa.u[0]^= local_double_type.u[0];
		doubleMantissa.u[1]^= local_double_type.u[1];

		// set singleMantissato 1 for each singleMantissabit
		singleMantissa.s= 1.0;
		singleMantissa.u^= local_single_type.u;

		initializedFlag= true;
    }
}

RNGen::~RNGen()
{}

void RNGen::seed(uint32_t aNewSeed)
{
    while (aNewSeed--)
		asLong32();
}

uint32_t RNGen::asLong()
{
    return asLong32() & 0x7fffffff;
}

float RNGen::asFloat()
{
    PrivateRNGSingleType aSingleType;
    aSingleType.s = 1.0;
    aSingleType.u |= (asLong32() & singleMantissa.u);
    aSingleType.s -= 1.0;
    assert( aSingleType.s < 1.0 && aSingleType.s >= 0);
    return( aSingleType.s );
}

double RNGen::asDouble()
{
    PrivateRNGDoubleType val;

    val.d = 1.0;
    // Big endian machine
    if (bigEndianMachineFlag)
    {
		val.u[0] |= (asLong32() & doubleMantissa.u[0]);
		val.u[1] |= (asLong32() & doubleMantissa.u[1]);
    }
    // Little endian machine
    else
    {
		val.u[1] |= (asLong32() & doubleMantissa.u[1]);
		val.u[0] |= (asLong32() & doubleMantissa.u[0]);
    }
    val.d -= 1.0;
    assert(val.d < 1.0 && val.d >= 0);
    return val.d;
}

//! Random number generator has been initialized?
bool RNGen::initializedFlag= false;

//! Big or little endian machine?
bool RNGen::bigEndianMachineFlag= false;

/*
  Local Variables:
  mode: c++
  folded-file: t
  End:
*/
