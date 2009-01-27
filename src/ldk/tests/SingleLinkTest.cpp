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

#include <WNS/CppUnit.hpp>
#include <WNS/ldk/SingleLink.hpp>
#include <WNS/ldk/tools/FakeFU.hpp>

namespace wns { namespace ldk { namespace tests {

	class SingleLinkTest :
		public wns::TestFixture
	{
		CPPUNIT_TEST_SUITE( SingleLinkTest );

                CPPUNIT_TEST( testConstructor );
		CPPUNIT_TEST( testSize );

                CPPUNIT_TEST( testAddAndEmpty );
                CPPUNIT_TEST( testAddNull );
		CPPUNIT_TEST( testClearAndEmpty );
                CPPUNIT_TEST( testGetAndEmpty );
                CPPUNIT_TEST( testSetAndEmpty );
                CPPUNIT_TEST( testSetMoreThanOneEmpty );

                CPPUNIT_TEST( testAddAndFull );
                CPPUNIT_TEST( testClearAndFull );
                CPPUNIT_TEST( testGetAndFull );
                CPPUNIT_TEST( testSetAndFull );
                CPPUNIT_TEST( testSetMoreThanOneFull );

		CPPUNIT_TEST_SUITE_END();

		void
		prepare()
		{
		}

		void
		cleanup()
		{
		}

	public:
		void
		testConstructor()
		{
			SingleLink testee;
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(0), testee.size() );
		}

		void
		testSize()
		{
			SingleLink testee;
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(0), testee.size() );
			tools::FakeFU fu;
			testee.add(&fu);
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(1), testee.size() );
		}

		void
		testAddNull()
		{
			SingleLink testee;
			WNS_ASSERT_ASSURE_NOT_NULL_EXCEPTION(testee.add(NULL));
		}

		void
		testAddAndEmpty()
		{
			SingleLink testee;
			tools::FakeFU fu;
			testee.add(&fu);
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(1), testee.size() );
		}

		void
		testClearAndEmpty()
		{
			SingleLink testee;
			testee.clear();
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(0), testee.size() );
		}

		void
		testGetAndEmpty()
		{
			SingleLink testee;
			CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), testee.get().size() );
		}

		void
		testSetAndEmpty()
		{
			SingleLink testee;
			Link::ExchangeContainer lec;
			tools::FakeFU fu;
			lec.push_back(&fu);
			testee.set(lec);
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(1), testee.size() );
		}

		void
		testAddAndFull()
		{
			SingleLink testee;
			tools::FakeFU fu;
			tools::FakeFU fu2;
			testee.add(&fu);
			WNS_ASSERT_ASSURE_EXCEPTION( testee.add(&fu2) );
			CPPUNIT_ASSERT( &fu == testee.get()[0] );
		}

		void
		testClearAndFull()
		{
			SingleLink testee;
			tools::FakeFU fu;
			testee.add(&fu);
			testee.clear();
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(0), testee.size() );
		}

		void
		testGetAndFull()
		{
			SingleLink testee;
			tools::FakeFU fu;
			testee.add(&fu);
			CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(1), testee.get().size() );
		}

		void
		testSetAndFull()
		{
			SingleLink testee;
			tools::FakeFU fu;
			testee.add(&fu);
			Link::ExchangeContainer lec;
			tools::FakeFU fu2;
			lec.push_back(&fu2);
			testee.set(lec);
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(1), testee.size() );
		}

		void
		testSetMoreThanOneEmpty()
		{
			SingleLink testee;
			Link::ExchangeContainer lec;
			tools::FakeFU fu;
			tools::FakeFU fu2;
			lec.push_back(&fu);
			lec.push_back(&fu2);
			WNS_ASSERT_ASSURE_EXCEPTION( testee.set(lec) );
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(0), testee.size() );
		}

		void
		testSetMoreThanOneFull()
		{
			SingleLink testee;
			tools::FakeFU fu3;
			testee.add(&fu3);
			Link::ExchangeContainer lec;
			tools::FakeFU fu;
			tools::FakeFU fu2;
			lec.push_back(&fu);
			lec.push_back(&fu2);
			WNS_ASSERT_ASSURE_EXCEPTION( testee.set(lec) );
			CPPUNIT_ASSERT_EQUAL( static_cast<uint32_t>(1), testee.size() );
			CPPUNIT_ASSERT( &fu3 == testee.get()[0] );
		}

	};


	CPPUNIT_TEST_SUITE_REGISTRATION( SingleLinkTest );

} // tests
} // ldk
} // wns




