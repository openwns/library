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

#include <WNS/ldk/sar/reassembly/ReorderingWindow.hpp>
#include <WNS/ldk/sar/reassembly/ReassemblyBuffer.hpp>

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

namespace wns { namespace ldk { namespace arq {

    namespace tests
    {
        class SelectiveRepeatIODTest;
    }

    /**
     * @brief Command used by the SelectiveRepeatIOD arq implementation.
     *
     */
    class SelectiveRepeatIODCommand :
        public wns::scheduler::queue::ISegmentationCommand
    {
    public:
        typedef long SequenceNumber;

        /*
         * I - Information Frame
         * ACK - received Packet (ACK)
         */
        typedef enum {I, ACK} FrameType;
        enum ACKPolicy {
            NoACK, ImmACK, BACK
        };

        // enum FrameType {
        //     Data, Ack
        // };

        SelectiveRepeatIODCommand():
            localTransmissionCounter(0)
        {
            peer.isBegin_ = false;
            peer.isEnd_ = false;
            peer.headerSize_ = 0;
            peer.dataSize_ = 0;
            peer.paddingSize_ = 0;
            peer.sdus_ = 0;
            peer.type = I;
            peer.sn_ = 0;
            local.lastSentTime = 0.0;
            local.firstSentTime = 0.0;
            magic.ackSentTime = 0.0;
        }

        void setNS(SequenceNumber sn)
        {
            peer.sn_ = sn;
        }

        SequenceNumber getNS() const
        {
            return peer.sn_;
        }

        bool isACK() const
        {
            return peer.type == ACK;
        }

        size_t localTransmissionCounter;

        struct
        {
            simTimeType lastSentTime;
            simTimeType firstSentTime;
        }
        local;
        struct
        {
            FrameType type;
            wns::ldk::ClassificationID id;
            bool isBegin_;
            bool isEnd_;
            SequenceNumber sn_;
            Bit headerSize_;
            Bit dataSize_;
            Bit paddingSize_;
            unsigned int sdus_;
            std::list<wns::ldk::CompoundPtr> pdus_;
            enum ACKPolicy ACKpolicy;
        }
        peer;
        struct Magic {
            int bsld;
            simTimeType ackSentTime;
        };
        Magic magic;

        /**
         * @brief Is the first byte of the datafield the beginning of
         * an SDU
         */
        virtual void
        setBeginFlag() { peer.isBegin_ = true; }

        virtual bool
        getBeginFlag() { return peer.isBegin_; }

        virtual void
        clearBeginFlag() { peer.isBegin_ = false; }

        /**
         * @brief Is the last byte of the datafield the end of
         * an SDU
         */
        virtual void
        setEndFlag() { peer.isEnd_ = true; }

        virtual bool
        getEndFlag() { return peer.isEnd_; }

        virtual void
        clearEndFlag() { peer.isEnd_ = false; }

        /**
         * @brief Set the Sequence number of this RLC PDU
         */
        virtual void
        setSequenceNumber(SequenceNumber sn) { peer.sn_ = sn;}

        /**
         * @brief The Sequence number of this RLC PDU
         */
        virtual SequenceNumber
        getSequenceNumber() { return peer.sn_; }

        virtual void
        increaseHeaderSize(Bit size) { peer.headerSize_ += size; }

        virtual void
        increaseDataSize(Bit size) { peer.dataSize_ += size; }

        virtual void
        increasePaddingSize(Bit size) { peer.paddingSize_ += size; }

        virtual Bit
        headerSize() { return peer.headerSize_; }

        virtual Bit
        dataSize() { return peer.dataSize_; }

        virtual Bit
        paddingSize() { return peer.paddingSize_; }

        virtual Bit
        totalSize() { return peer.headerSize_ + peer.dataSize_ + peer.paddingSize_; }

        /**
         * @brief Append an SDU segment to this PDU.
         */
        virtual void
        addSDU(wns::ldk::CompoundPtr c) { peer.pdus_.push_back(c); peer.sdus_++;}

        virtual unsigned int
        getNumSDUs() {return peer.sdus_;}

    };


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
        public ARQ,
        public wns::ldk::fu::Plain<SelectiveRepeatIOD, SelectiveRepeatIODCommand>,
        public Delayed<SelectiveRepeatIOD>,
        virtual public SuspendableInterface,
        public SuspendSupport,
        virtual public DelayedDeliveryInterface,
        public events::CanTimeout
    {
        friend class tests::SelectiveRepeatIODTest;
        typedef std::list<CompoundPtr> CompoundContainer;

    public:
        // FUNConfigCreator interface realisation
        SelectiveRepeatIOD(fun::FUN* fuNet, const wns::pyconfig::View& config);
        ~SelectiveRepeatIOD();

        // CanTimeout interface realisation
        virtual void onTimeout();

        // Delayed interface realisation
        virtual bool hasCapacity() const;
        virtual void processOutgoing(const CompoundPtr& sdu);
        // virtual const CompoundPtr hasSomethingToSend() const; // implemented by ARQ
        // virtual CompoundPtr getSomethingToSend(); // implemented by ARQ
        virtual void processIncoming(const CompoundPtr& compound);

        // ARQ interface realization
        virtual const wns::ldk::CompoundPtr hasACK() const;
        virtual const wns::ldk::CompoundPtr hasData() const;
        virtual wns::ldk::CompoundPtr getACK();
        virtual wns::ldk::CompoundPtr getData();

        // Overload of CommandTypeSpecifier Interface
        void calculateSizes(const CommandPool* commandPool, Bit& commandPoolSize, Bit& sduSize) const;

    protected:
        // Internal handlers for I- and ACK-Frames
        void onIFrame(const CompoundPtr& compound);
        void onACKFrame(const CompoundPtr& compound);

        // sort into given Compound List
        void keepSorted(const CompoundPtr& compound, CompoundContainer& conatiner);

        // remove ACKed PDU from list
        void removeACKed(const CompoundPtr& ackCompound, CompoundContainer& container);

        // prepare list of frames to retransmit
        void prepareRetransmission();

        // retransmissionState
        bool retransmissionState() const;

        virtual bool onSuspend() const;

        virtual void doDelayDelivery();
        virtual void doDeliver();

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
         * @brief Remember to send the activeCompound.
         */
        bool sendNow;

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
    public:
        void frameStarts() {};

    };

}
}
}

#endif // NOT defined WNS_LDK_ARQ_SELECTIVEREPEATIOD_HPP


