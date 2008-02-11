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

#include <WNS/probe/bus/MasterProbeBus.hpp>
#include <WNS/probe/bus/tests/ProbeBusStub.hpp>

#include <WNS/pyconfig/Parser.hpp>

#include <WNS/TestFixture.hpp>
#include <sstream>

namespace wns { namespace probe { namespace bus { namespace tests {

    /**
     * @brief Tests for the MasterProbeBus
     * @author Daniel Bültmann <me@daniel-bueltmann.de>
     */
    class MasterProbeBusTest : public wns::TestFixture  {
        CPPUNIT_TEST_SUITE( MasterProbeBusTest );
        CPPUNIT_TEST( testSingleListener );
        CPPUNIT_TEST( testMultipleListeners );
        CPPUNIT_TEST( testRegistry );
        CPPUNIT_TEST_SUITE_END();

    public:
        void prepare();
        void cleanup();

        void testSingleListener();
        void testMultipleListeners();
        void testRegistry();

    private:
        ProbeBus* theMasterProbeBus_;
    };
}}}}

using namespace wns::probe::bus::tests;

CPPUNIT_TEST_SUITE_REGISTRATION( MasterProbeBusTest );

void
MasterProbeBusTest::prepare()
{
    wns::pyconfig::Parser empty;
    theMasterProbeBus_ = new MasterProbeBus(empty);
}

void
MasterProbeBusTest::cleanup()
{
    delete theMasterProbeBus_;
}

void
MasterProbeBusTest::testSingleListener()
{
    ProbeBusStub listener;

    listener.startReceiving(theMasterProbeBus_);

    wns::probe::bus::Context tmp;
    this->theMasterProbeBus_->forwardMeasurement(1.0, 2.0, tmp);

    CPPUNIT_ASSERT(listener.receivedCounter == 1);
    CPPUNIT_ASSERT(listener.receivedTimestamps[0] == 1.0);
    CPPUNIT_ASSERT(listener.receivedValues[0] == 2.0);
}

void
MasterProbeBusTest::testMultipleListeners()
{
    ProbeBusStub listener1;
    listener1.startReceiving(theMasterProbeBus_);

    ProbeBusStub listener2;
    listener2.startReceiving(theMasterProbeBus_);

    wns::probe::bus::Context tmp;
    this->theMasterProbeBus_->forwardMeasurement(2.0, 5.0, tmp);

    CPPUNIT_ASSERT(listener1.receivedCounter == 1);
    CPPUNIT_ASSERT(listener1.receivedTimestamps[0] == 2.0);
    CPPUNIT_ASSERT(listener1.receivedValues[0] == 5.0);

    CPPUNIT_ASSERT(listener2.receivedCounter == 1);
    CPPUNIT_ASSERT(listener2.receivedTimestamps[0] == 2.0);
    CPPUNIT_ASSERT(listener2.receivedValues[0] == 5.0);
}

void
MasterProbeBusTest::testRegistry()
{
    wns::probe::bus::Context reg;
    reg.insertInt("Peter", 3);
    reg.insertInt("July", 7);

    ProbeBusStub listener1;
    listener1.startReceiving(theMasterProbeBus_);
    listener1.setFilter("Peter", 3);

    ProbeBusStub listener2;
    listener2.startReceiving(theMasterProbeBus_);
    listener2.setFilter("July", 15);

    this->theMasterProbeBus_->forwardMeasurement(3.0, 17.0, reg);

    CPPUNIT_ASSERT(listener1.receivedCounter == 1);

    CPPUNIT_ASSERT(listener1.receivedTimestamps[0] == 3.0);

    CPPUNIT_ASSERT(listener1.receivedValues[0] == 17.0);

    CPPUNIT_ASSERT(listener2.receivedCounter == 0);
}
