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

#ifndef WNS_LDK_HARQ_HARQ_HPP
#define WNS_LDK_HARQ_HARQ_HPP

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>

namespace wns { namespace ldk { namespace harq {

    class HARQCommand :
        public Command
    {
    public:

        typedef enum{
            I,
            ACK,
            NACK,
            INVALID
        } FrameType;

        HARQCommand()
        {
            peer.type = INVALID;
            peer.NDI = false;
            peer.processId = -1;
        }

        struct {
        } local;

        struct {
            FrameType type;
            bool NDI;
            int processId;
        } peer;

        struct {} magic;

    };

    class HARQ :
        public fu::Plain<HARQ, HARQCommand>,
        public Delayed<HARQ>
    {
    public:
        HARQ(fun::FUN* fuNet, const wns::pyconfig::View& config);

        ~HARQ();

        virtual void
        onFUNCreated();

        // SDU and PCI size calculation
        void
        calculateSizes(const CommandPool* commandPool,
                       Bit& commandPoolSize,
                       Bit& dataSize) const;

    private:

        class HARQSenderProcess
        {
        public:

            HARQSenderProcess(int processID, HARQ* entity, wns::logger::Logger logger);

            bool
            hasCapacity() const;

            void
            enqueueTransmission(const wns::ldk::CompoundPtr& compound);

            void
            ackReceived();

            void
            nackReceived();

        private:
            int processID_;

            HARQ* entity_;

            wns::logger::Logger logger_;

            wns::ldk::CompoundPtr buffer_;

            bool waitingForFeedback_;

            int retransmissionCounter_;

        };

        class HARQReceiverProcess
        {
        public:

            HARQReceiverProcess(int processID, HARQ* entity, wns::logger::Logger logger);

            void
            receive(const wns::ldk::CompoundPtr&);

        private:
            int processID_;

            HARQ* entity_;

            wns::logger::Logger logger_;

            wns::ldk::CompoundPtr ack_;

            wns::ldk::CompoundPtr buffer_;

        };

        virtual void
        processIncoming(const wns::ldk::CompoundPtr& compound);

        virtual void
        processOutgoing(const wns::ldk::CompoundPtr&);

        virtual bool
        hasCapacity() const;

        virtual const wns::ldk::CompoundPtr
        hasSomethingToSend() const;

        virtual wns::ldk::CompoundPtr
        getSomethingToSend();

        void
        addToSendQueue(wns::ldk::CompoundPtr compound);

        wns::logger::Logger logger_;

        int numSenderProcesses_;

        int numReceiverProcesses_;

        std::vector<HARQSenderProcess> senderProcesses_;

        std::vector<HARQ::HARQReceiverProcess> receiverProcesses_;

        std::list<wns::ldk::CompoundPtr> sendQueue_;
    };

} // harq
} // ldk
} // wns

#endif // WNS_LDK_HARQ_HARQ_HPP
