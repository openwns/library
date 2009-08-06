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

#ifndef WNS_SCHEDULER_SCHEDULINGMAP_HPP
#define WNS_SCHEDULER_SCHEDULINGMAP_HPP

#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/MapInfoEntry.hpp>
#include <WNS/service/phy/phymode/PhyModeInterface.hpp>
#include <WNS/ldk/Compound.hpp>
#include <WNS/simulator/Time.hpp>
#include <WNS/SmartPtr.hpp>
#include <vector>
#include <list>

namespace wns { namespace scheduler {
        namespace strategy {
            class RequestForResource;
        }
        /** @brief class to describe the contents of a SchedulingSubChannel */
        class SchedulingCompound
        {
        public:
            SchedulingCompound();
            SchedulingCompound(int _subChannel,
                               int _beam,
                               simTimeType _startTime,
                               simTimeType _endTime,
                               wns::scheduler::ConnectionID _connectionID,
                               wns::scheduler::UserID _userID,
                               wns::ldk::CompoundPtr _compoundPtr,
                               wns::service::phy::phymode::PhyModeInterfacePtr _phyModePtr,
                               wns::Power _txPower,
                               wns::service::phy::ofdma::PatternPtr _pattern
                );
            ~SchedulingCompound();
            simTimeType getCompoundDuration() { return endTime-startTime; };
            std::string toString() const;
        public:
            int subChannel;
            /** @brief for MIMO; in [0..(maxBeams-1)] */
            int beam;
            simTimeType startTime;
            simTimeType endTime;
            wns::scheduler::ConnectionID connectionID;
            wns::scheduler::UserID userID;
            wns::ldk::CompoundPtr compoundPtr;
            /** @brief usually all compounds in a subChannel must have the same phyMode */
            wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr;
            /** @brief usually all compounds in a subChannel must have the same power level */
            wns::Power txPower;
            /** @brief Antenna pattern for beamforming; else empty.
                Yet unclear if this is constant over all subchannels or not.
                Probably it should be removed from the compound. */
            wns::service::phy::ofdma::PatternPtr pattern;
            /** @brief signal and noise+interference power assumed at receiver.
                Just informational. Can be used in Receiver to calculate
                the difference between real and estimated SINR. */
            //wns::CandI estimatedCandI; // not supported yet
        }; // SchedulingCompound

        typedef std::list<SchedulingCompound> ScheduledCompoundsList;

        /** @brief class to describe one PhysicalResourceBlock.
            There are 1..M of this object in the SchedulingMap for each subChannel.
            With MIMO there are CxM of these altogether. */
        class PhysicalResourceBlock // PRB
        {
        public:
            PhysicalResourceBlock();
            PhysicalResourceBlock(int _subChannelIndex, int _beam, simTimeType _slotLength);
            ~PhysicalResourceBlock();
            std::string toString() const;
            /** @brief total used time in this PhysicalResourceBlock */
            simTimeType getUsedTime() const;
            /** @brief total free time on this PhysicalResourceBlock */
            simTimeType getFreeTime() const;
            void setNextPosition(simTimeType _nextPosition);

            /** @brief get "offset for new compounds" == used time for already scheduled compounds. Zero for empty subChannel */
            simTimeType getNextPosition() const;

            /** @brief true if compound can be put into the PhysicalResourceBlock */
            bool pduFitsIntoPhysicalResourceBlock(strategy::RequestForResource& request,
                                                  MapInfoEntryPtr mapInfoEntry) const;

            /** @brief returns number of bits that fit into the PhysicalResourceBlock.
                Depends on PhyMode provided by mapInfoEntry. */
            int getFreeBitsOnPhysicalResourceBlock(MapInfoEntryPtr mapInfoEntry) const;

            /** @brief put scheduled compound (one after another) into the SchedulingSubChannel */
            bool addCompound(simTimeType compoundDuration,
                             wns::scheduler::ConnectionID connectionID,
                             wns::scheduler::UserID userID,
                             wns::ldk::CompoundPtr compoundPtr,
                             wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr,
                             wns::Power txPower,
                             wns::service::phy::ofdma::PatternPtr pattern
                );

            /** @brief put scheduled compound (one after another) into the PhysicalResourceBlock
                @param MapInfoEntryPtr contains result from doAdaptiveResourceScheduling(), but without compound contained
                @param compoundPtr is the PDU to be put into the resource
                @return true if successful, false if not enough space.
            */
            bool addCompound(strategy::RequestForResource& request,
                             MapInfoEntryPtr mapInfoEntry, // <- must not contain compounds yet
                             wns::ldk::CompoundPtr compoundPtr
                );

            /** @brief true if there is nothing scheduled in this block.
                This question is NOT enough to allow it to be used. see subChannelIsUsable. */
            bool isEmpty() const;

            /** @brief Delete all compounds. But keep all other info (PhyMode, usedTime).
                This is called by the UL master scheduler,
                because there are no "real" compounds (just fakes). */
            void deleteCompounds();

        public:
            /** @brief my own subChannelIndex as seen from outside (container) */
            int subChannelIndex;
            /** @brief my own beamIndex as seen from outside (container).
                For MIMO; in [0..(maxBeams-1)] */
            int beamIndex;
            /** @brief fixed frame/slot length given from outside */
            simTimeType slotLength;
            /** @brief remaining time on this subChannel for new compounds */
            simTimeType freeTime;
            /** @brief offset for new compounds == used time for already scheduled compounds */
            simTimeType nextPosition;
            /** @brief list of all compound together with their attributes */
            ScheduledCompoundsList scheduledCompounds;
            /** @brief phyMode used in this subChannel (all compounds should have the same) */
            wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr;
            //usedTxPowerOnOneChannel usedTxPower; // std::vector<TxPower4PDU> is very bad
            /** @brief transmit power used in this subChannel (e.g. when APC is used) */
            wns::Power txPower;
            /** @brief Antenna pattern for beamforming; else empty.
                Yet unclear if this is constant over all subchannels or not. */
            wns::service::phy::ofdma::PatternPtr antennaPattern;
        }; // PhysicalResourceBlock

        /** @brief collection of all subChannels and all (MIMO) beams */
        typedef std::vector<PhysicalResourceBlock> PhysicalResourceBlockVector;

        /** @brief class to describe one SchedulingSubChannel.
            There is one of this object in the SchedulingMap for each subChannel.
            SISO/MIMO see the same.
            Only that for MIMO it contains 1..M PhysicalResourceBlocks */
        class SchedulingSubChannel
                : virtual public wns::RefCountable // for SmartPtr
        {
        public:
            SchedulingSubChannel();
            SchedulingSubChannel(int _subChannelIndex, int _numberOfBeams, simTimeType _slotLength);
            ~SchedulingSubChannel();
            std::string toString() const;
            /** @brief total used time in this subchannel */
            simTimeType getUsedTime() const;
            /** @brief total free time on this subchannel */
            simTimeType getFreeTime() const;
            void setNextPosition(simTimeType _nextPosition);

            /** @brief get "offset for new compounds" == used time for already scheduled compounds. Zero for empty subChannel */
            simTimeType getNextPosition() const;

            /** @brief true if compound can be put into the SchedulingSubChannel */
            bool pduFitsIntoSubChannel(strategy::RequestForResource& request,
                                       MapInfoEntryPtr mapInfoEntry) const;

            /** @brief returns number of bits that fit into the SchedulingSubChannel.
                Depends on PhyMode provided by mapInfoEntry. */
            int getFreeBitsOnSubChannel(MapInfoEntryPtr mapInfoEntry) const;

            /** @brief true if there is nothing scheduled in this block.
                This question is NOT enough to allow it to be used. see subChannelIsUsable. */
            bool isEmpty() const;

            /** @brief Delete all compounds. But keep all other info (PhyMode, usedTime).
                This is called by the UL master scheduler,
                because there are no "real" compounds (just fakes). */
            void deleteCompounds();

        public:
            /** @brief my own subChannelIndex as seen from outside (container) */
            int subChannelIndex;
            /** @brief size of resources in spatial direction.
                This can be beamforming beams (available for WiMAC)
                or MIMO paths. */
            int numberOfBeams;
            /** @brief fixed frame/slot length given from outside */
            simTimeType slotLength;
            /** @brief isUsable = flag to exclude certain subchannels from DSA */
            bool subChannelIsUsable;
            /** @brief collection of all PhysicalResourceBlocks (one per MIMO beam; only one for SISO) */
            PhysicalResourceBlockVector physicalResources; // [0..M-1] for MIMO
        }; // SchedulingSubChannel

        /** @brief can be used to send via an container compound to emulate one complete resource unit (chunk) */
        typedef SmartPtr<SchedulingSubChannel> SchedulingSubChannelPtr;

        /** @brief collection of all subChannels */
        typedef std::vector<SchedulingSubChannel> SubChannelVector;
        //typedef std::vector<SchedulingSubChannelPtr> SubChannelVector; // TODO?

        /** @brief this class contains the results over all subChannels */
        class SchedulingMap :
            public wns::IOutputStreamable,
            virtual public wns::RefCountable // for SmartPtr
        {
        public:
            SchedulingMap() {};

            /** @brief construct a new empty SchedulingMap which contains a number of SchedulingSubChannel's */
            SchedulingMap(simTimeType _slotLength, int _numberOfSubChannels, int _numberOfBeams, int _frameNr);

            ~SchedulingMap();

            /** @brief true if compound can be put into the SchedulingSubChannel */
            bool pduFitsIntoSubChannel(strategy::RequestForResource& request,
                                       MapInfoEntryPtr mapInfoEntry) const;

            /** @brief returns number of bits that fit into the SchedulingSubChannel.
                Depends on PhyMode provided by mapInfoEntry. */
            int getFreeBitsOnSubChannel(MapInfoEntryPtr mapInfoEntry) const;

            /** @brief put scheduled compound (one after another) into the SchedulingMap
                @return true if successful, false if not enough space.
            */
            bool addCompound(int subChannelIndex,
                             int beam,
                             simTimeType compoundDuration,
                             wns::scheduler::ConnectionID connectionID,
                             wns::scheduler::UserID userID,
                             wns::ldk::CompoundPtr compoundPtr,
                             wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr,
                             wns::Power txPower,
                             wns::service::phy::ofdma::PatternPtr pattern
                );

            /** @brief put scheduled compound (one after another) into the SchedulingMap
                @param MapInfoEntryPtr contains result from doAdaptiveResourceScheduling(), but without compound contained
                @param compoundPtr is the PDU to be put into the resource
                @return true if successful, false if not enough space.
            */
            bool addCompound(strategy::RequestForResource& request,
                             MapInfoEntryPtr mapInfoEntry, // <- must not contain compounds yet
                             wns::ldk::CompoundPtr compoundPtr
                );

            simTimeType getNextPosition(int subChannel, int beam) const;

            /** @brief collection of all subChannels */
            SubChannelVector subChannels;

            /** @brief statistics for the percentage of resources used.
                (correcly counts partially filled subChannels).
                Result is in interval [0..1]. Multiply with 100.0 for Percent.
                Expensive operation, iterates over all resource blocks.
                Please do this only once (for probing); or in dbg mode (in MESSAGE). */
            double
            getResourceUsage();

            /** @brief total used time (up to numberOfSubChannels*slotLength) */
            simTimeType
            getUsedTime() const;

            /** @brief total leftover time (initially numberOfSubChannels*slotLength) */
            simTimeType
            getFreeTime() const;

            /** @brief total leftover power */
            wns::Power
            getRemainingPower(wns::Power totalPower) const;

            simTimeType getSlotLength()  const { return slotLength; }
            int getNumberOfSubChannels() const { return numberOfSubChannels; }
            int getNumberOfBeams()       const { return numberOfBeams; }
            int getNumberOfCompounds()   const { return numberOfCompounds; }

            /** @brief mask out certain subChannels (e.g. for resource partitioning) */
            void maskOutSubChannels(const UsableSubChannelVector& usableSubChannels);

            /** @brief make MapInfoCollection structure from myself */
            void convertToMapInfoCollection(MapInfoCollectionPtr collection /*return value*/);

            /** @brief Delete all compounds from the map. But keep all other info (PhyMode, usedTime).
                This is called by the UL master scheduler,
                because there are no "real" compounds (just fakes). */
            void deleteCompounds();

            /** @brief true if there is nothing scheduled in the whole schedulingMap. */
            bool isEmpty() const;

            /** @brief output structure (structured text) */
            std::string
            toString();

            /** @brief output structure (conforming to IOutputStreamable) */
            virtual std::string
            doToString() const;

            void writeToFile(std::ofstream& f) const;

            /** @brief output structure (table file) */
            void writeFile(std::string fileName) const;

        private:
            /** @brief index of the frame this map is for (system dependent) */
            int frameNr;
            /** @brief size of resources in time-direction */
            simTimeType slotLength;
            /** @brief size of resources in frequency-direction */
            int numberOfSubChannels;
            /** @brief size of resources in spatial direction.
                This can be beamforming beams (available for WiMAC)
                or MIMO paths (not yet available). */
            int numberOfBeams;
            /** @brief just counting compounds as they are inserted */
            int numberOfCompounds;
            /** @brief result of getResourceUsage() stored for convenience and efficiency */
            double resourceUsage;
            /** @brief decreased each time a subChannel is used */
            //wns::Power totalRemainingPower
        }; // SchedulingMap

        /** @brief created in the strategies; no need for memory tracking later */
        typedef SmartPtr<SchedulingMap> SchedulingMapPtr;

        /** @brief stream operator for class */
        inline std::ostream&
        operator<< (std::ostream& s, const SchedulingCompound& object) {
            s << object.toString();
            return s;
        }
        /** @brief stream operator for class */
        inline std::ostream&
        operator<< (std::ostream& s, const PhysicalResourceBlock& object) {
            s << object.toString();
            return s;
        }
        /** @brief stream operator for class */
        inline std::ostream&
        operator<< (std::ostream& s, const SchedulingSubChannel& object) {
            s << object.toString();
            return s;
        }
        /** @brief stream operator for class */
        inline std::ostream&
        operator<< (std::ostream& s, SchedulingMap& object) {
            s << object.toString();
            return s;
        }
        //}}}}
    }}
#endif //WNS_SCHEDULER_SCHEDULINGMAP_HPP
