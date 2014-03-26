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

#ifndef WNS_LDK_ARQ_SELECTIVEREPEATIOD_HPP
#define WNS_LDK_ARQ_SELECTIVEREPEATIOD_HPP

#include <WNS/pyconfig/View.hpp>

#include <WNS/ldk/arq/SelectiveRepeatIODCommand.hpp>
#include <WNS/ldk/arq/ARQTimeouts.hpp>

#include <WNS/ldk/sar/reassembly/SegmentationBuffer.hpp>

#include <WNS/scheduler/queue/ISegmentationCommand.hpp>
#include <WNS/ldk/arq/ARQ.hpp>
#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/SuspendableInterface.hpp>
#include <WNS/ldk/SuspendSupport.hpp>
#include <WNS/ldk/DelayedDeliveryInterface.hpp>
#include <WNS/ldk/fu/Plain.hpp>

#include <WNS/logger/Logger.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>
#include <WNS/events/CanTimeout.hpp>

#include <WNS/ldk/Classifier.hpp>

#include <list>
#include <cmath>

#include <WNS/ldk/CommandTypeSpecifier.hpp>
#include <WNS/ldk/HasReceptor.hpp>
#include <WNS/ldk/HasConnector.hpp>
#include <WNS/ldk/HasDeliverer.hpp>

using namespace std;
using namespace wns::ldk;

namespace wns { namespace ldk { namespace arq {

    namespace tests
    {
        class SelectiveRepeatIODTest;
    }

    /**
     * @brief SelectiveRepeatIOD implementation of the ARQ interface.
     *
     * This implementation was made according to the implementation of the
     * SelectiveRepeatIOD ARQ algorithm as described by Fred Halsall in "Data
     * Communications, Computer Networks and open systems".
     *
     * @note The mentioned
     * implementation relies on the in-order delivery of
     * ACKnowledgements. i.e. if an ACKnowledgement other than the next one
     * expected is received by the Sender, a packet loss is considered to be
     * the reason and the sender enters retransmission state. This makes
     * this ARQ scheme unusable in applications where the in-order-delivery
     * can not be guaranteed (e.g. in a TCP implementation). Consider using
     * N-channel Stop and Wait in that case.
     *
     * @note: the following modification has been made to make this
     * implementation work in an environment where the Sender can receive the
     * ACKs for multiple packets at the same point in time: If an
     * out-of-sequence ACK arrives, previous (unacked) packets are only
     * retransmitted if (a) they have not been retransmitted before or (b)
     * the previous retransmission took place more than 2*minRTT ago. The
     * minRTT is measured by the protocol itself.
     */
    class SelectiveRepeatIOD :
        virtual public FunctionalUnit,
        public Delayed<SelectiveRepeatIOD>,
        virtual public SuspendableInterface,
        public SuspendSupport,
        public HasReceptor<>,
        public HasConnector<>,
        public HasDeliverer<>,
        public wns::Cloneable<SelectiveRepeatIOD>,
        public CommandTypeSpecifier<SelectiveRepeatIODCommand>,
        public events::CanTimeout
    {
        friend class tests::SelectiveRepeatIODTest;

    public:
        // FUNConfigCreator interface realisation
        SelectiveRepeatIOD(fun::FUN* fuNet, const wns::pyconfig::View& config);
        SelectiveRepeatIOD(const SelectiveRepeatIOD &other);
        ~SelectiveRepeatIOD();

        virtual void onFUNCreated();

        // CanTimeout interface realisation
        virtual void onTimeout();

        // Delayed interface realisation
        virtual bool hasCapacity() const;
        virtual void processOutgoing(const CompoundPtr& sdu);
        virtual void processIncoming(const CompoundPtr& compound);

        // ARQ interface realization
        virtual const CompoundPtr hasACK() const;
        virtual CompoundPtr getACK();

        virtual const CompoundPtr hasSomethingToSend() const;
        virtual CompoundPtr getSomethingToSend();

        // Overload of CommandTypeSpecifier Interface
        void calculateSizes(const CommandPool* commandPool, Bit& commandPoolSize, Bit& sduSize) const;

    protected:
        CompoundPtr createSegment(const CompoundPtr& sdu,
                                  SequenceNumber sequenceNumber,
                                  const Bit segmentSize,
                                  GroupNumber timestamp);

        CompoundPtr createStartSegment(const CompoundPtr& sdu,
                                       SequenceNumber sequenceNumber,
                                       const Bit segmentSize,
                                       GroupNumber timestamp);

        CompoundPtr createEndSegment(const CompoundPtr& sdu,
                                     SequenceNumber sequenceNumber,
                                     const Bit segmentSize,
                                     GroupNumber timestamp);

        CompoundPtr createSegment(const CompoundPtr& sdu,
                                  SequenceNumber sequenceNumber,
                                  const Bit segmentSize,
                                  GroupNumber timestamp,
                                  bool isBegin,
                                  bool isEnd);

        CompoundPtr createUnsegmented(const CompoundPtr& sdu,
                                      SequenceNumber sequenceNumber,
                                      const Bit segmentSize,
                                      GroupNumber groupId);

        void sendPoll(const CompoundPtr& compound);

        void prepareRetransmission();

        // retransmissionState
        bool retransmissionState() const;

        virtual bool onSuspend() const;

        // virtual void doDelayDelivery();
        // virtual void doDeliver();

        /**
         * @brief Window size.
         */
        int windowSize;

        /**
         * @brief Divisor for Sequence Numbers.
         */
        int sequenceNumberSize;

        /**
         * @brief Number of Bits the command occupies in the Commandpool.
         */
        int commandSize;

        /**
         * @brief Sequence number of the last packet sent.
         */
        ARQCommand::SequenceNumber NS;

        /**
         * @brief Sequence number of the last packet received in order.
         */
        ARQCommand::SequenceNumber NR;

        /**
         * @brief Sequence number of the last ACK received in order.
         */
        ARQCommand::SequenceNumber LA;

        /**
         * @brief Sequence number of the next PU to be sent, excluding retransmissions.
         * initialized with 0
         */
        ARQCommand::SequenceNumber VTS;

        /**
         * @brief Sequence number of the next in-sequence PU expected to be received.
         * initialized with 0
         */
        ARQCommand::SequenceNumber VTR;

        /**
         * @brief Receive State Variable: Sequence number ofthe next in-sequence PU expected to be received.
         * initialized with 0
         */
        ARQCommand::SequenceNumber VR;

        /**
         * @brief PDU to be sent.
         */
        CompoundPtr activeCompound;

        /**
         * @brief The packets sent but not acknowledged yet.
         */
        CompoundContainer sentPDUs;

        /**
         * @brief List of packets to be retransmit
         */
        CompoundContainer toRetransmit;

        /**
         * @brief list of ACK PDU to be sent.
         */
        CompoundContainer ackPDUs;

        /**
         * @brief received compounds.
         */
        CompoundContainer receivedPDUs;

        /**
         * @brief received out-of-sequence ACKs.
         */
        CompoundContainer receivedACKs;

        /**
         * @brief Idle (no ACK) time after which sender enters
         * retransmission state.
         */
        double resendTimeout;

        /**
         * @brief min time to be waiting between two retransmissions of
         * the same packet.
         */
        double retransmissionInterval;

        /**
         * @brief probe to count the number of retransmissions needed.
         * :TODO: this should go into the base class when the probe
         * configuration works better.
         */
        wns::probe::bus::ContextCollectorPtr transmissionAttemptsProbeBus;
        wns::probe::bus::ContextCollectorPtr ackDelayProbeBus;
        wns::probe::bus::ContextCollectorPtr roundTripTimeProbeBus;

        bool delayingDelivery;

        logger::Logger logger;
        bool isSegmenting_;

    private:
        bool onReassembly(compoundReassembly_t&);
        void addToSenderQueue(CompoundContainer& compoundList,
                              SequenceNumber startSegment,
                              SequenceNumber endSegment);

        CompoundContainer senderPendingSegments_;

        CompoundContainer senderPendingStatusSegments_;

        std::string commandName_;

        Bit headerSize_;

        Bit segmentSize_;

        Bit sduLengthAddition_;

        SequenceNumber nextOutgoingSN_;

        std::string segmentDropRatioProbeName_;

        wns::probe::bus::ContextCollectorPtr minDelayCC_;
        wns::probe::bus::ContextCollectorPtr maxDelayCC_;
        wns::probe::bus::ContextCollectorPtr sizeCC_;
        wns::probe::bus::ContextCollectorPtr segmentDropRatioCC_;

        CommandReaderInterface* probeHeaderReader_;

        sar::reassembly::SegmentationBuffer segmentationBuffer_;
    };

}
}
}

#endif // NOT defined WNS_LDK_ARQ_SELECTIVEREPEATIOD_HPP


