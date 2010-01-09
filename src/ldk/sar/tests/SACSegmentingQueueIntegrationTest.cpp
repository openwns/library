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

#include <WNS/ldk/sar/SegAndConcat.hpp>
#include <WNS/scheduler/queue/SegmentingQueue.hpp>
#include <WNS/TestFixture.hpp>
#include <WNS/ldk/FunctionalUnit.hpp>
#include <WNS/ldk/fun/Main.hpp>
#include <WNS/ldk/tests/LayerStub.hpp>
#include <WNS/ldk/tools/Stub.hpp>
#include <WNS/scheduler/tests/RegistryProxyStub.hpp>
#include <WNS/ldk/helper/FakePDU.hpp>
#include <WNS/pyconfig/Parser.hpp>

#include <cppunit/extensions/HelperMacros.h>

#define NEWPDU wns::ldk::helper::FakePDUPtr(new wns::ldk::helper::FakePDU())

namespace wns { namespace ldk { namespace sar { namespace tests {
class SACSegmentingQueueIntegrationTest:
    public wns::TestFixture
{
    CPPUNIT_TEST_SUITE( SACSegmentingQueueIntegrationTest );
    CPPUNIT_TEST( testSACSegmentingDisabled );
    CPPUNIT_TEST( testIncomingOneSegment );
    CPPUNIT_TEST( testIncomingTwoSegments );
    CPPUNIT_TEST( testIncomingFourSegments );
    CPPUNIT_TEST_SUITE_END();

    wns::ldk::ILayer* layer_;
    wns::ldk::fun::FUN* fuNet_;

    wns::ldk::tools::Stub* upper_;
    SegAndConcat* testee_;
    wns::scheduler::queue::SegmentingQueue* segmentingQueue_;
    wns::ldk::tools::Stub* lower_;
    wns::scheduler::tests::RegistryProxyStub* registryProxy_;

public:

    void
    prepare();

    void
    latePrepare();

    void
    cleanup();

    void
    testSACSegmentingDisabled();

    void
    testIncomingOneSegment();

    void
    testIncomingTwoSegments();

    void
    testIncomingFourSegments();

    std::string alternateConfiguration;
};
} // tests
} // sar
} // ldk
} // wns

using namespace wns::ldk::sar::tests;

CPPUNIT_TEST_SUITE_REGISTRATION( SACSegmentingQueueIntegrationTest );

void SACSegmentingQueueIntegrationTest::prepare()
{
}

void SACSegmentingQueueIntegrationTest::latePrepare()
{
    wns::pyconfig::Parser emptyConfig;

    layer_ = new wns::ldk::tests::LayerStub();
    fuNet_ = new wns::ldk::fun::Main(layer_);

    upper_ = new wns::ldk::tools::Stub(fuNet_, emptyConfig);
    lower_ = new wns::ldk::tools::Stub(fuNet_, emptyConfig);

    if (alternateConfiguration == "")
    {
        wns::pyconfig::Parser config;
        config.loadString(
            "from openwns.SAR import SegAndConcat\n"
            "testee = SegAndConcat(segmentSize=24, headerSize=16, commandName=\"testee\")\n"
            "testee.logger.level = 3\n"
            "testee.isSegmenting = False\n"
        );

        wns::pyconfig::View configView(config, "testee");
        testee_ = new SegAndConcat(fuNet_, configView);
    }
    else
    {
        wns::pyconfig::Parser config;
        config.loadString(alternateConfiguration);

        wns::pyconfig::View configView(config, "testee");
        testee_ = new SegAndConcat(fuNet_, configView);
    }

    fuNet_->addFunctionalUnit("upper", upper_);
    fuNet_->addFunctionalUnit("testee", testee_);
    fuNet_->addFunctionalUnit("lower", lower_);

    upper_->connect(testee_)->connect(lower_);

    testee_->onFUNCreated();

    {
        wns::pyconfig::Parser config;
        config.loadString(
           "from openwns.Scheduler import SegmentingQueue\n"
           "testee = SegmentingQueue(\"testee\", \"testee\")\n"
            "testee.fixedHeaderSize = 16\n"
            "testee.extensionHeaderSize = 8\n"
        );
        registryProxy_ = new wns::scheduler::tests::RegistryProxyStub();

        wns::pyconfig::View configView(config, "testee");
        segmentingQueue_ = new wns::scheduler::queue::SegmentingQueue(NULL, configView);
        segmentingQueue_->setFUN(fuNet_);
        segmentingQueue_->setColleagues(registryProxy_);
    }
}

void
SACSegmentingQueueIntegrationTest::cleanup()
{
    delete segmentingQueue_;
    delete registryProxy_;
    delete layer_;
    delete fuNet_;
}

void
SACSegmentingQueueIntegrationTest::testSACSegmentingDisabled()
{
    alternateConfiguration = "from openwns.SAR import SegAndConcat\n"
        "testee = SegAndConcat(segmentSize=24, headerSize=16, commandName=\"testee\")\n"
        "testee.logger.level = 3\n"
        "testee.isSegmenting = False\n";

    latePrepare();

    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), lower_->sent.size());

    wns::ldk::helper::FakePDUPtr innerPDU = NEWPDU;
    innerPDU->setLengthInBits(24);
    wns::ldk::CompoundPtr compound(new wns::ldk::Compound(fuNet_->createCommandPool(), innerPDU));

    upper_->sendData(compound);

    CPPUNIT_ASSERT_EQUAL((size_t) 1, lower_->sent.size());

    CPPUNIT_ASSERT_EQUAL(compound, lower_->sent[0]);
}

void
SACSegmentingQueueIntegrationTest::testIncomingOneSegment()
{
    latePrepare();

    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), lower_->sent.size());

    wns::ldk::helper::FakePDUPtr innerPDU = NEWPDU;
    innerPDU->setLengthInBits(24);
    wns::ldk::CompoundPtr compound(new wns::ldk::Compound(fuNet_->createCommandPool(), innerPDU));

    registryProxy_->setCIDforPDU(compound, 1);
    segmentingQueue_->put(compound);

    CPPUNIT_ASSERT(segmentingQueue_->queueHasPDUs(1));

    wns::ldk::CompoundPtr c = segmentingQueue_->getHeadOfLinePDUSegment(1, 40);

    CPPUNIT_ASSERT_EQUAL((Bit) 40, c->getLengthInBits());

    lower_->onData(c);

    CPPUNIT_ASSERT_EQUAL((size_t) 1, upper_->received.size());
}

void
SACSegmentingQueueIntegrationTest::testIncomingTwoSegments()
{
    latePrepare();

    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), lower_->sent.size());

    wns::ldk::helper::FakePDUPtr innerPDU = NEWPDU;
    innerPDU->setLengthInBits(24);
    wns::ldk::CompoundPtr compound(new wns::ldk::Compound(fuNet_->createCommandPool(), innerPDU));

    registryProxy_->setCIDforPDU(compound, 1);
    segmentingQueue_->put(compound);

    CPPUNIT_ASSERT(segmentingQueue_->queueHasPDUs(1));

    wns::ldk::CompoundPtr c = segmentingQueue_->getHeadOfLinePDUSegment(1, 28);

    CPPUNIT_ASSERT_EQUAL((Bit) 28, c->getLengthInBits());

    lower_->onData(c);

    CPPUNIT_ASSERT_EQUAL((size_t) 0, upper_->received.size());

    c = segmentingQueue_->getHeadOfLinePDUSegment(1, 28);

    CPPUNIT_ASSERT_EQUAL((Bit) 28, c->getLengthInBits());

    lower_->onData(c);

    CPPUNIT_ASSERT_EQUAL((size_t) 1, upper_->received.size());
}

void
SACSegmentingQueueIntegrationTest::testIncomingFourSegments()
{
    latePrepare();

    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), lower_->sent.size());

    wns::ldk::helper::FakePDUPtr innerPDU = NEWPDU;
    innerPDU->setLengthInBits(24);
    wns::ldk::CompoundPtr compound(new wns::ldk::Compound(fuNet_->createCommandPool(), innerPDU));

    registryProxy_->setCIDforPDU(compound, 1);
    segmentingQueue_->put(compound);

    CPPUNIT_ASSERT(segmentingQueue_->queueHasPDUs(1));

    wns::ldk::CompoundPtr c = segmentingQueue_->getHeadOfLinePDUSegment(1, 22);

    CPPUNIT_ASSERT_EQUAL((Bit) 22, c->getLengthInBits());

    lower_->onData(c);

    CPPUNIT_ASSERT_EQUAL((size_t) 0, upper_->received.size());

    c = segmentingQueue_->getHeadOfLinePDUSegment(1, 22);

    CPPUNIT_ASSERT_EQUAL((Bit) 22, c->getLengthInBits());

    lower_->onData(c);

    CPPUNIT_ASSERT_EQUAL((size_t) 0, upper_->received.size());

    c = segmentingQueue_->getHeadOfLinePDUSegment(1, 22);

    CPPUNIT_ASSERT_EQUAL((Bit) 22, c->getLengthInBits());

    lower_->onData(c);

    CPPUNIT_ASSERT_EQUAL((size_t) 0, upper_->received.size());

    c = segmentingQueue_->getHeadOfLinePDUSegment(1, 22);

    CPPUNIT_ASSERT_EQUAL((Bit) 22, c->getLengthInBits());

    lower_->onData(c);

    CPPUNIT_ASSERT_EQUAL((size_t) 1, upper_->received.size());
}
