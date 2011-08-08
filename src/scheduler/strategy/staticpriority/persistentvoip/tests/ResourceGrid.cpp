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

#include <WNS/scheduler/strategy/staticpriority/persistentvoip/ResourceGrid.hpp>

#include <WNS/CppUnit.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/pyconfig/Parser.hpp>
#include <WNS/pyconfig/View.hpp>

namespace wns { namespace scheduler { namespace strategy { namespace staticpriority { namespace persistentvoip { namespace tests {

	class ResourceGridTest :
		public wns::TestFixture
	{
		CPPUNIT_TEST_SUITE(ResourceGridTest);
        CPPUNIT_TEST(testDimension);
		CPPUNIT_TEST_SUITE_END();
	public:
		ResourceGridTest();
		~ResourceGridTest();
		void prepare();
		void cleanup();
        void testDimension();

    private:
        wns::logger::Logger logger_;
        ResourceGrid* rg_;
	};

CPPUNIT_TEST_SUITE_REGISTRATION(ResourceGridTest);
}}}}}} 

using namespace wns::scheduler::strategy::staticpriority::persistentvoip::tests;

ResourceGridTest::ResourceGridTest() : 
	wns::TestFixture(),
    logger_("TEST", "TEST")
{
}

ResourceGridTest::~ResourceGridTest()
{
}

void ResourceGridTest::prepare()
{
	wns::pyconfig::Parser rgConfig;
	std::stringstream ss;
	ss << "from openwns.Scheduler import PersistentVoIP\n"
	   << "rg = PersistentVoIP.ResourceGrid(None)\n";
	rgConfig.loadString(ss.str());

    rg_ = new ResourceGrid(rgConfig.getView("rg"), logger_, 5, 10);
}

void ResourceGridTest::testDimension()
{
    unsigned int nf = rg_->getNumberOfFrames();
    CPPUNIT_ASSERT_EQUAL(nf, (unsigned int)5);
    CPPUNIT_ASSERT_THROW(rg_->getFrame(nf), wns::Assure::Exception);
    CPPUNIT_ASSERT_EQUAL(rg_->getFrame(3)->getFrameIndex(), (unsigned int)3);
}


void ResourceGridTest::cleanup()
{
    delete rg_;
}


