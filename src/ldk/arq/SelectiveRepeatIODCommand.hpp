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

#ifndef WNS_LDK_ARQ_SELECTIVEREPEATIODCOMMAND_HPP
#define WNS_LDK_ARQ_SELECTIVEREPEATIODCOMMAND_HPP

#include <WNS/pyconfig/View.hpp>

#include <WNS/ldk/Compound.hpp>
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

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

using namespace std;
using namespace wns::ldk;
using namespace ::boost;
using namespace ::boost::multi_index;

namespace wns { namespace ldk { namespace arq {

    typedef long SequenceNumber;
    typedef unsigned long long int BigSequenceNumber;

    typedef struct {
      clock_t clock;
      time_t time;

    } timestamp_s;

    typedef struct {
      timestamp_s timestamp;
      BigSequenceNumber SN;
    } missingPdu_s;

    // segments are grouped by timestamp
    // typedef timestamp_s GroupNumber;
    typedef double GroupNumber;

    //missingItems {
    // timestamp: {
    //   list of big sequence numbers
    // }
    //}
    typedef std::list<BigSequenceNumber> lPdu_t;
    typedef std::map<GroupNumber, lPdu_t> mapMissingPdu_t;

    struct outgoingPdu
    {
      GroupNumber timestamp;
      BigSequenceNumber sn;

      CompoundPtr pdu_;

      outgoingPdu(GroupNumber ts, BigSequenceNumber seq, const CompoundPtr &pdu)
          : timestamp(ts), sn(seq), pdu_(pdu)
      {}
    };

    struct pduSN {};
    struct pduID {};

    // allow looking up and deleting pdus from the outgoing buffer either by
    // sequence number or by timestamp
    typedef multi_index_container<
      outgoingPdu,
      indexed_by<
        ordered_non_unique<
          tag< pduID >,
          member< outgoingPdu, GroupNumber, &outgoingPdu::timestamp >
        >,
        hashed_unique<
          tag< pduSN >,
          member< outgoingPdu, BigSequenceNumber, &outgoingPdu::sn >
        >
      >
    > multiOutgoing_t;
    typedef multiOutgoing_t::index<pduSN>::type outgoing_by_sn;
    typedef multiOutgoing_t::index<pduID>::type outgoing_by_timestamp;

    typedef std::map<BigSequenceNumber, CompoundPtr> mapCompoundOutgoing_t;

    // typedef std::map<GroupNumber, mapCompoundOutgoing_t> mapOutgoing_t;

    typedef std::list<CompoundPtr> CompoundContainer;

    // list of completed segments, we use that instead of LSN objects
    // inside of the status PDU
    typedef std::set<GroupNumber> completedList_t;

    typedef map<SequenceNumber, CompoundPtr> compoundReassembly_t;

    typedef map<GroupNumber, compoundReassembly_t> compoundHashTable_t;
    bool const operator==(const timestamp_s n, const timestamp_s &o);
    bool const operator<(const timestamp_s n, const timestamp_s &o);

    /**
     * @brief Command used by the SelectiveRepeatIOD arq implementation.
     *
     */
    class SelectiveRepeatIODCommand :
        public wns::scheduler::queue::ISegmentationCommand
    {
    public:

        /*
         * I - Information Frame
         * ACK - received Packet (ACK)
         */
        typedef enum {I, ACK, STATUS} FrameType;
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
            peer.bigSn_ = 0;
            peer.isPoll_ = false;
            local.lastSentTime = 0.0;
            local.firstSentTime = 0.0;
            magic.ackSentTime = 0.0;
            magic.isSegmented = false;
            magic.startSN_ = 0;
            magic.endSN_ = 0;
        }

        void setNS(SequenceNumber sn)
        {
            peer.sn_ = sn;
        }

        SequenceNumber getNS() const
        {
            return peer.sn_;
        }

        void setGroupId(GroupNumber id)
        {
          magic.groupId_ = id;
        }

        GroupNumber groupId() const
        {
          return magic.groupId_;
        }

        void setSegmented()
        {
          magic.isSegmented = true;
        }

        bool isSegmented()
        {
          return magic.isSegmented;
        }

        bool isACK() const
        {
            return peer.type == ACK;
        }

        bool isStatus() const
        {
          return peer.type == STATUS;
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
            ClassificationID id;
            bool isBegin_;
            bool isEnd_;
            bool isPoll_;
            SequenceNumber sn_;
            BigSequenceNumber bigSn_;
            Bit headerSize_;
            Bit dataSize_;
            Bit paddingSize_;
            unsigned int sdus_;
            CompoundContainer pdus_;
            enum ACKPolicy ACKpolicy;
        }
        peer;
        struct Magic {
            int bsld;
            simTimeType ackSentTime;
            bool isSegmented;
            GroupNumber groupId_;
            // magic storing beginning and end segment in every segment
            // allows us to quickly see which segments are missing
            SequenceNumber startSN_;
            SequenceNumber endSN_;
            // add base sdu
            CompoundPtr baseSDU_;

            // for polling completed pdus and missing pdus
            lPdu_t missingPduList_;
            completedList_t completedPdus_;
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
         * @brief Has the poll flag set
         * an SDU
         */
        virtual void
        setPollFlag() { peer.isPoll_ = true; }

        virtual bool
        hasPollFlag() { return peer.isPoll_; }

        virtual void
        clearPollFlag() { peer.isPoll_ = false; }


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
        addSDU(CompoundPtr c) { peer.pdus_.push_back(c); peer.sdus_++;}

        virtual unsigned int
        getNumSDUs() {return peer.sdus_;}


        /**
         * store start and end sequence number
         */
        virtual SequenceNumber
        getStartSN() { return magic.startSN_; }

        virtual void
        setStartSN(SequenceNumber startSN) { magic.startSN_ = startSN; }

        virtual SequenceNumber
        getEndSN() { return magic.endSN_; }

        virtual void
        setEndSN(SequenceNumber endSN) { magic.endSN_ = endSN; }

        void bigSN(BigSequenceNumber SN) {
          peer.bigSn_ = SN;
        }

        BigSequenceNumber bigSN() {
          return peer.bigSn_;
        }

        /**
         * store the base pdu
         */
        void setBaseSDU(CompoundPtr c)
        {
          magic.baseSDU_ = c;
        }

        CompoundPtr baseSDU()
        {
          return magic.baseSDU_;
        }

        void addMissingPdus(const lPdu_t& missingPduList) {
          magic.missingPduList_ = missingPduList;
        }

        const lPdu_t* missingPdus() {
          return &magic.missingPduList_;
        }

        void addCompletedPdus(completedList_t completedList) {
          magic.completedPdus_ = completedList;
        }

        const completedList_t* completedPdus() {
          return &magic.completedPdus_;
        }
    };


}
}
}

#endif // NOT defined WNS_LDK_ARQ_SELECTIVEREPEATIODCOMMAND_HPP


