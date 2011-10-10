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

#include <WNS/scheduler/strategy/staticpriority/persistentvoip/TBChoser.hpp>

#include <WNS/CppUnit.hpp>

namespace wns { namespace scheduler { namespace strategy { namespace staticpriority { namespace persistentvoip { namespace tests {

	class TBChoserTest :
		public wns::TestFixture
	{
		CPPUNIT_TEST_SUITE(TBChoserTest);
        CPPUNIT_TEST(testFirst);
        CPPUNIT_TEST(testBest);
        CPPUNIT_TEST(testWorst);
        CPPUNIT_TEST(testSmallest);
        CPPUNIT_TEST(testRandom);
        CPPUNIT_TEST(testEmpty);
		CPPUNIT_TEST_SUITE_END();
	public:
		TBChoserTest();
		~TBChoserTest();
		void prepare();
		void cleanup();
        void testFirst();
        void testBest();
        void testWorst();
        void testRandom();
        void testSmallest();
        void testEmpty();

    private:
        ITBChoser* tbc_;
        Frame::SearchResultSet srs1;
	};

CPPUNIT_TEST_SUITE_REGISTRATION(TBChoserTest);
}}}}}} 

using namespace wns::scheduler::strategy::staticpriority::persistentvoip::tests;

TBChoserTest::TBChoserTest() : 
	wns::TestFixture()
{
}

TBChoserTest::~TBChoserTest()
{
}

void TBChoserTest::prepare()
{
    Frame::SearchResult sr;

    /* Index     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 */
    /* Occupied  T T 0 0 1 1 1 T 0 1 1 1 1 1 1 T T T 0 0 */
    sr.success = true;

    sr.tbLength = 2;
    sr.start = 0;
    sr.length = 4;
    srs1.insert(sr);
    sr.tbLength = 1;
    sr.start = 7;
    sr.length = 2;
    srs1.insert(sr);
    sr.tbLength = 3;
    sr.start = 15;
    sr.length = 5;
    srs1.insert(sr);
}

void TBChoserTest::testFirst()
{
    tbc_ = new First();
    Frame::SearchResult sr;
    sr = tbc_->choseTB(srs1);
    CPPUNIT_ASSERT_EQUAL(sr.start, (unsigned int)0);
    CPPUNIT_ASSERT_EQUAL(sr.length, (unsigned int)4);
    delete tbc_;
}

void TBChoserTest::testBest()
{
    tbc_ = new BestFit();
    Frame::SearchResult sr;
    sr = tbc_->choseTB(srs1);
    CPPUNIT_ASSERT_EQUAL(sr.start, (unsigned int)7);
    CPPUNIT_ASSERT_EQUAL(sr.length, (unsigned int)2);
    delete tbc_;
}

void TBChoserTest::testWorst()
{
    tbc_ = new WorstFit();
    Frame::SearchResult sr;
    sr = tbc_->choseTB(srs1);
    CPPUNIT_ASSERT_EQUAL(sr.start, (unsigned int)15);
    CPPUNIT_ASSERT_EQUAL(sr.length, (unsigned int)5);
    delete tbc_;
}

void TBChoserTest::testRandom()
{
    tbc_ = new Random();
    Frame::SearchResult sr;

    unsigned int trials = 1000000;
    double sum = 0;
    for(int i = 0; i < trials; i++)
    {
        sr = tbc_->choseTB(srs1);
        sum += sr.start;
    }

    double mean = sum / double(trials);
    double calcMean;
    calcMean = (0.0 + 7.0 + 15.0) / 3.0;

    /* This is a statistic test so there is always a chance it could fail */
    /* In this case reduce tolerance or increase trials */
    CPPUNIT_ASSERT_DOUBLES_EQUAL(mean, calcMean, 0.01);
    delete tbc_;
}

void TBChoserTest::testSmallest()
{
    tbc_ = new Smallest();
    Frame::SearchResult sr;

    sr = tbc_->choseTB(srs1);
    CPPUNIT_ASSERT_EQUAL(sr.start, (unsigned int)7);
    CPPUNIT_ASSERT_EQUAL(sr.length, (unsigned int)2);
    delete tbc_;
}


void TBChoserTest::testEmpty()
{
    Frame::SearchResult sr;
    Frame::SearchResultSet srs;
    sr.success = true;
    sr.tbLength = 5;
    sr.length = 10;
    sr.start = 0;
    srs.insert(sr);

    tbc_ = new First();
    sr = tbc_->choseTB(srs);
    CPPUNIT_ASSERT_EQUAL(sr.start, (unsigned int)0);
    CPPUNIT_ASSERT_EQUAL(sr.length, (unsigned int)10);
    delete tbc_;
    tbc_ = new BestFit();
    sr = tbc_->choseTB(srs);
    CPPUNIT_ASSERT_EQUAL(sr.start, (unsigned int)0);
    CPPUNIT_ASSERT_EQUAL(sr.length, (unsigned int)10);
    delete tbc_;
    tbc_ = new WorstFit();
    sr = tbc_->choseTB(srs);
    CPPUNIT_ASSERT_EQUAL(sr.start, (unsigned int)0);
    CPPUNIT_ASSERT_EQUAL(sr.length, (unsigned int)10);
    delete tbc_;
    tbc_ = new Random();
    sr = tbc_->choseTB(srs);
    CPPUNIT_ASSERT_EQUAL(sr.start, (unsigned int)0);
    CPPUNIT_ASSERT_EQUAL(sr.length, (unsigned int)10);
    delete tbc_;
}


void TBChoserTest::cleanup()
{
}


