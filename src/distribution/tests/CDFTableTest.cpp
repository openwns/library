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

#include <WNS/distribution/tests/CDFTableTest.hpp>

#include <WNS/pyconfig/Parser.hpp>
#include <WNS/Average.hpp>
#include <WNS/distribution/tests/VarEstimator.hpp>
#include <WNS/TestFixture.hpp>

using namespace wns::distribution::test;

CPPUNIT_TEST_SUITE_REGISTRATION( CDFTableTest );


void
CDFTableTest::setUp()
{
}


void
CDFTableTest::tearDown()
{
}


void
CDFTableTest::testIt()
{
    pyconfig::Parser config;
    config.loadString(
        "cdfTable = ((40, 0.5),\n"
                "(1500, 1.0))\n"
    );

    CDFTable* dis =
        dynamic_cast<CDFTable*>(
        wns::distribution::DistributionFactory::creator("CDFTable")
        ->create(config));

    Average<double> average;
    for(int i = 0; i < 100000; ++i)
    {
        average.put((*dis)());
    }

    double calculatedAverage = dis->getMean();
    /**
     * @todo (rs) Sometimes "randomness" even exceeds any given bound.
     */
    WNS_ASSERT_MAX_REL_ERROR(calculatedAverage, average.get(), 0.01);

    delete dis;
}

void
CDFTableTest::testVar()
{
    pyconfig::Parser config;
    config.loadString(
        "cdfTable = ((40, 0.5),\n"
                "(1500, 1.0))\n"
    );

    CDFTable* dis =
        dynamic_cast<CDFTable*>(
        wns::distribution::DistributionFactory::creator("CDFTable")
        ->create(config));

    VarEstimator var;
    for(int i = 0; i < 100000; ++i)
    {
        var.put((*dis)());
    }

    double calcMean = (1500.0 - 40.0) / 2.0;
    double calcVar = 0.5 * (calcMean - 40.0) * (calcMean - 40.0) 
                     + 0.5 * (calcMean - 1500.0) * (calcMean - 1500.0);

    WNS_ASSERT_MAX_REL_ERROR(calcVar, var.get(), 0.01);

    delete dis;
}


void
CDFTableTest::testPyConfig()
{
	pyconfig::Parser config;
	config.loadString(
		"import wns.distribution.CDFTables\n"
		"packetSize = wns.distribution.CDFTables.IPPacketSizeDataTraffic()\n"
	);

	wns::pyconfig::View packetConfig(config, "packetSize");

	CDFTable* dis =
		dynamic_cast<CDFTable*>(
		wns::distribution::DistributionFactory::creator(packetConfig.get<std::string>("__plugin__"))
		->create(packetConfig));

	Average<double> average;
	for(int i = 0; i < 100000; ++i) {
		average.put((*dis)());
	}

	double calculatedAverage = dis->getMean();
	// Bytes: - Expected: 257.104 - Actual  : 256.494
	// Bits:  - Expected: 2056.84 - Actual  : 2051.95
	/**
	 * @todo (msg) Can we get closer than 1%?
	 * @todo (rs) Sometimes "randomness" even exceeds any given bound.
	 */
	WNS_ASSERT_MAX_REL_ERROR(calculatedAverage, average.get(), 0.01);

	delete dis;
}

/*
  Local Variables:
  mode: c++
  fill-column: 80
  c-basic-offset: 8
  c-comment-only-line-offset: 0
  c-tab-always-indent: t
  indent-tabs-mode: t
  tab-width: 8
  End:
*/
