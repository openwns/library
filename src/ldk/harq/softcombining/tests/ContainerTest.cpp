/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
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

#include <WNS/TestFixture.hpp>
#include <WNS/ldk/harq/softcombining/Container.hpp>

namespace wns { namespace ldk { namespace harq { namespace softcombining { namespace tests {

    /**
	 * @brief test for wns::ldk::harq::softcombining::Container
	 * @author Daniel Bueltmann <me@daniel-bueltmann.de>
	 */
    class ContainerTest :
        public wns::TestFixture
    {
        CPPUNIT_TEST_SUITE( ContainerTest );
        CPPUNIT_TEST( testGetNumRVs );
        CPPUNIT_TEST( testGetEmptyCompoundList );
        CPPUNIT_TEST( testAppendAndGetCompoundList );
        CPPUNIT_TEST( testGetCompoundListForInvalidRV );
        CPPUNIT_TEST( testAppendCompoundListForInvalidRV );
        CPPUNIT_TEST( testClear );
        CPPUNIT_TEST_SUITE_END();

    public:

        void
        prepare();

        void
        cleanup();

        void
        testGetNumRVs();

        void
        testGetEmptyCompoundList();

        void
        testAppendAndGetCompoundList();

        void
        testGetCompoundListForInvalidRV();

        void
        testAppendCompoundListForInvalidRV();

        void
        testClear();
    };

    CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( ContainerTest, wns::testsuite::Default() );

} // tests
} // softcombining
} // harq
} // ldk
} // wns

using namespace wns::ldk::harq::softcombining::tests;

void
ContainerTest::prepare()
{
}

void
ContainerTest::cleanup()
{
}

void
ContainerTest::testGetNumRVs()
{
    Container c = Container(5);

    CPPUNIT_ASSERT_EQUAL(c.getNumRVs(), 5);
}

void
ContainerTest::testGetEmptyCompoundList()
{
    Container c = Container(9);

    CPPUNIT_ASSERT_EQUAL(c.getNumRVs(), 9);

    Container::CompoundList cl;

    cl = c.getCompoundsForRV(4);

    CPPUNIT_ASSERT_EQUAL(cl.size(), (size_t) 0);
}


void
ContainerTest::testAppendAndGetCompoundList()
{
    Container c = Container(3);

    CPPUNIT_ASSERT_EQUAL(c.getNumRVs(), 3);

    c.appendCompoundForRV(1, wns::ldk::CompoundPtr());

    c.appendCompoundForRV(2, wns::ldk::CompoundPtr());

    c.appendCompoundForRV(2, wns::ldk::CompoundPtr());

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(0).size(), (size_t) 0);

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(1).size(), (size_t) 1);

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(2).size(), (size_t) 2);
}

void
ContainerTest::testGetCompoundListForInvalidRV()
{
    Container c = Container(3);

    CPPUNIT_ASSERT_THROW(c.getCompoundsForRV(3), Container::InvalidRV);
}


void
ContainerTest::testAppendCompoundListForInvalidRV()
{
    Container c = Container(5);

    CPPUNIT_ASSERT_THROW(c.appendCompoundForRV(6, wns::ldk::CompoundPtr()), Container::InvalidRV);
}

void
ContainerTest::testClear()
{
    Container c = Container(3);

    CPPUNIT_ASSERT_EQUAL(c.getNumRVs(), 3);

    c.appendCompoundForRV(1, wns::ldk::CompoundPtr());

    c.appendCompoundForRV(2, wns::ldk::CompoundPtr());

    c.appendCompoundForRV(2, wns::ldk::CompoundPtr());

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(0).size(), (size_t) 0);

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(1).size(), (size_t) 1);

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(2).size(), (size_t) 2);

    c.clear();

    CPPUNIT_ASSERT_EQUAL(c.getNumRVs(), 3);

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(0).size(), (size_t) 0);

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(1).size(), (size_t) 0);

    CPPUNIT_ASSERT_EQUAL(c.getCompoundsForRV(2).size(), (size_t) 0);
}

