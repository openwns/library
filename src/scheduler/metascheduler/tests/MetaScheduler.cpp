/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2009
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 5, D-52074 Aachen, Germany
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

#include <WNS/scheduler/metascheduler/MetaScheduler.hpp>
#include <WNS/scheduler/metascheduler/GreedyMetaScheduler.hpp>
#include <WNS/pyconfig/Parser.hpp>

#include <WNS/CppUnit.hpp>
#include <WNS/logger/Logger.hpp>

namespace wns { namespace scheduler { namespace metascheduler { namespace tests {

	class MetaSchedulerTest :
		public wns::TestFixture
	{
		CPPUNIT_TEST_SUITE(MetaSchedulerTest);
        CPPUNIT_TEST(testGreedyTwoBS);
        //CPPUNIT_TEST(testGreedyThreeBS);
		CPPUNIT_TEST_SUITE_END();
	public:
		MetaSchedulerTest();
		~MetaSchedulerTest();
		void prepare();
		void cleanup();
        void testGreedyTwoBS();
        //void testGreedyThreeBS();

    private:
        wns::logger::Logger logger_;
        wns::pyconfig::Parser parser_;
	};

CPPUNIT_TEST_SUITE_REGISTRATION(MetaSchedulerTest);
}}}} 

using namespace wns::scheduler::metascheduler::tests;

MetaSchedulerTest::MetaSchedulerTest()
{
}

MetaSchedulerTest::~MetaSchedulerTest()
{
}

void MetaSchedulerTest::prepare()
{
	parser_.loadString("class MS(object):\n"
                        "\tclass IC(object):\n"
                            "\t\tc = \"0dBm\"\n"
                            "\t\ti = \"0dBm\"\n"
                            "\t\tpl = \"0dB\"\n"
                        "\tinitialICacheValues = IC()\n"
                    "ms = MS()\n");
}

void MetaSchedulerTest::testGreedyTwoBS()
{
    int numBS = 2;
    int numUTs = 3;

    UtilityMatrix um;
    std::vector<int> uts(numBS);
    uts[0] = numUTs;
    uts[1] = numUTs;

    um.createMatrix(numBS, uts);

    /* Write utility matrix
    1 2 3
    4 5 6
    7 8 9
    */
    std::vector<int> index(2);
    index[0] = 0;
    index[1] = 0;
    um.setValue(index, 1);
    index[1] = 1;
    um.setValue(index, 2);
    index[1] = 2;
    um.setValue(index, 3);
    index[0] = 1;
    index[1] = 0;
    um.setValue(index, 4);
    index[1] = 1;
    um.setValue(index, 5);
    index[1] = 2;
    um.setValue(index, 6);
    index[0] = 2;
    index[1] = 0;
    um.setValue(index, 7);
    index[1] = 1;
    um.setValue(index, 8);
    index[1] = 2;
    um.setValue(index, 9);
    
    std::vector<std::vector<int> > vBestCombinations(numBS);
    for(int i = 0; i < numBS; i++)
        vBestCombinations[i].resize(numUTs);

    GreedyMetaScheduler gms = GreedyMetaScheduler(parser_.get("ms"));    

    gms.optimize(um, vBestCombinations);

    /* BS0 UT2 with BS1 UT2 */
    CPPUNIT_ASSERT(vBestCombinations[0][0] == 2);
    CPPUNIT_ASSERT(vBestCombinations[1][0] == 2);

    /* BS0 UT1 with BS1 UT1 */
    CPPUNIT_ASSERT(vBestCombinations[0][1] == 1);
    CPPUNIT_ASSERT(vBestCombinations[1][1] == 1);

    /* BS0 UT0 with BS1 UT0 */
    CPPUNIT_ASSERT(vBestCombinations[0][2] == 0);
    CPPUNIT_ASSERT(vBestCombinations[1][2] == 0);
}

/*void MetaSchedulerTest::testGreedyThreeBS()
{
    
}*/

void MetaSchedulerTest::cleanup()
{
}


