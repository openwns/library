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
        CPPUNIT_TEST(testSched);
		CPPUNIT_TEST_SUITE_END();
	public:
		ResourceGridTest();
		~ResourceGridTest();
		void prepare();
		void cleanup();
        void testDimension();
        void testSched();

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

void ResourceGridTest::testSched()
{
    Frame* f0 = rg_->getFrame(0);

    /* Start is out of range */
    CPPUNIT_ASSERT_THROW(f0->reserve(0, 10, 4, false), wns::Assure::Exception);
    /* End is  out of range */
    CPPUNIT_ASSERT_THROW(f0->reserve(0, 7, 4, true), wns::Assure::Exception);

    Frame::SearchResultSet srs;

    srs = f0->findTransmissionBlocks(3);

    /* Everything is free so there is one successfull result */
    CPPUNIT_ASSERT_EQUAL(srs.size(), (size_t)1);
    CPPUNIT_ASSERT_EQUAL(srs.begin()->success, true);

    /* Starting at zero */
    CPPUNIT_ASSERT_EQUAL(srs.begin()->start, (unsigned int)0);

    /* 10 RBs long */
    CPPUNIT_ASSERT_EQUAL(srs.begin()->length, (unsigned int)10);

    /* Reserve RBs 5 and 6 for CID 0*/
    f0->reserve(0, 5, 2, true);

    srs = f0->findTransmissionBlocks(3);
    /* Now there should be two potential TBs */
    CPPUNIT_ASSERT_EQUAL(srs.size(), (size_t)2);

    Frame::SearchResultSet::iterator it = srs.begin();

    /* First potential TB: 0 to 4; length 5 */
    CPPUNIT_ASSERT_EQUAL(it->start, (unsigned int)0);
    CPPUNIT_ASSERT_EQUAL(it->length, (unsigned int)5);

    it++;

    /* Second potential TB: 7 to 9; length 3 */
    CPPUNIT_ASSERT_EQUAL(it->start, (unsigned int)7);
    CPPUNIT_ASSERT_EQUAL(it->length, (unsigned int)3);

    /* There is no free TB of length 6 */
    srs = f0->findTransmissionBlocks(6);
    CPPUNIT_ASSERT_EQUAL(srs.size(), (size_t)0);

    /* Clear unpersistent reservations for frame 0 */
    rg_->onNewFrame(0);

    /* We scheduled CID 0 persistently so nothing has changed */
    srs = f0->findTransmissionBlocks(6);
    CPPUNIT_ASSERT_EQUAL(srs.size(), (size_t)0);

    /* We can get 5 RBs */
    srs = f0->findTransmissionBlocks(5);
    CPPUNIT_ASSERT_EQUAL(srs.size(), (size_t)1);

    /* Now we reserve 0 to 2 for CID 1 unpersistently */
    f0->reserve(1, 0, 3, false);

    /* Do not allow to reserve occupied blocks (try to reserve 2 to 3)*/
    CPPUNIT_ASSERT_THROW(f0->reserve(2, 2, 2, false), wns::Assure::Exception);
    
    /* We cannot get 5 RBs anymore */
    srs = f0->findTransmissionBlocks(5);
    CPPUNIT_ASSERT_EQUAL(srs.size(), (size_t)0);

    /* Clear unpersistent reservations for frame 0 */
    rg_->onNewFrame(0);

    /* We can get 5 RBs again */
    srs = f0->findTransmissionBlocks(5);
    CPPUNIT_ASSERT_EQUAL(srs.size(), (size_t)1);

    /* Try to reserve for CID 0 already having reservation */

    /* Unpersistently */
    CPPUNIT_ASSERT_THROW(f0->reserve(0, 0, 1, false), wns::Assure::Exception);
    /* Persistently */
    CPPUNIT_ASSERT_THROW(f0->reserve(0, 0, 1, true), wns::Assure::Exception);
    /* Other frame */
    Frame* f1 = rg_->getFrame(1);
    CPPUNIT_ASSERT_NO_THROW(f1->reserve(0, 0, 1, true));


}

void ResourceGridTest::cleanup()
{
    delete rg_;
}


