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
            class RequestForResource; /** @see SchedulerState.hpp */
        }

            struct HARQInfo {
                HARQInfo() : NDI(true), enabled(false), processID(0), rv(0), retryCounter(0) {}
                /**
                 * @brief New Data Indication flag
                 */
                bool NDI;
                bool enabled;
                int processID;
                int rv;
                int retryCounter;
                boost::function<void ()> ackCallback;
                boost::function<void ()> nackCallback;
            };


        /** @brief class to describe the contents of a SchedulingSubChannel */
        class SchedulingCompound
        {
        public:
            SchedulingCompound();
            SchedulingCompound(int _subChannel,
                               int _timeSlot,
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
            //SchedulingCompound(const SchedulingCompound&);

            ~SchedulingCompound();
            simTimeType getCompoundDuration() { return endTime-startTime; };
            std::string toString() const;
        public:
            /** @brief index of subChannel (FDMA component) */
            int subChannel;
            /** @brief index of time slot (TDMA component) */
            int timeSlot;
            /** @brief for MIMO; in [0..(maxBeams-1)] */
            int beam;
            simTimeType startTime;
            simTimeType endTime;
            wns::scheduler::ConnectionID connectionID;
            /** @brief usually all compounds in a PRB must have the same user */
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

        typedef SmartPtr<SchedulingCompound> SchedulingCompoundPtr;
        typedef std::list<SchedulingCompound> ScheduledCompoundsList;

        /** @brief class to describe one PhysicalResourceBlock.
            There are 1..M of this object in the SchedulingMap for each subChannel.
            With MIMO there are CxM of these altogether. */
        class PhysicalResourceBlock // PRB
        {
        public:
            PhysicalResourceBlock();
            PhysicalResourceBlock(int _subChannelIndex, int _timeSlotIndex, int _beam, simTimeType _slotLength);
            //PhysicalResourceBlock(const PhysicalResourceBlock&);

            ~PhysicalResourceBlock();
            /** @brief total used time in this PhysicalResourceBlock */
            simTimeType getUsedTime() const;
            /** @brief total free time on this PhysicalResourceBlock */
            simTimeType getFreeTime() const;
            /** @brief mark t=[0..nextPosition] as used by compounds. The rest is free. */
            //void setNextPosition(simTimeType _nextPosition);

            /** @brief get "offset for new compounds" == used time for already scheduled compounds. Zero for empty subChannel */
            simTimeType getNextPosition() const;

            /** @brief true if compound can be put into the PhysicalResourceBlock */
            bool pduFitsInto(strategy::RequestForResource& request,
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

            /** @brief doToString(): human-readable format */
            std::string toString() const;
            /** @brief output structure (machine readable table for Matlab,Gnuplot,etc) */
            std::string dumpContents(const std::string& prefix) const;

            /** @brief number of compunds inside this resource */
            int countScheduledCompounds() const;
            /** @brief true if there is nothing reserved(scheduled) in this block (DL).
                For the uplink the master map entries have isEmpty==false.
                This question is NOT enough to allow it to be used. see subChannelIsUsable. */
            bool isEmpty() const;
            /** @brief get userID this resource has been reserved for or NULL if empty */
            wns::scheduler::UserID getUserID() const;
            /** @brief get txPower assigned to this resource.
                Assumes that all contents are for ONE user only and have all equal power. */
            wns::Power getTxPower() const;

            /** @brief Delete all compounds. But keep all other info (PhyMode, usedTime).
                This is called by the UL master scheduler,
                because there are no "real" compounds (just fakes).
                Only useful for UL scheduling. */
            void deleteCompounds();
            /** @brief extent the usage of each resource to 100% so that UL master map doesn't waste resources.
                Only useful for UL scheduling. */
            void grantFullResources();
            /** @brief perform modifications to use this as MasterMap (called in uplink slave scheduler).
                Only useful for UL scheduling. */
            void processMasterMap();
            /** @brief checks if there are UL resources available for given user */
            bool hasResourcesForUser(wns::scheduler::UserID user) const;
            /** @brief determine the length in bits stored in this resource */
            int getNetBlockSizeInBits() const;

        public:
            /** @brief my own subChannelIndex as seen from outside (container) */
            int subChannelIndex;
            /** @brief index of time slot (TDMA component) */
            int timeSlotIndex;
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
            /** @brief usually all compounds in a PRB must have the same user */
            wns::scheduler::UserID userID;
            /** @brief phyMode used in this subChannel (all compounds should have the same) */
            wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr;
            /** @brief transmit power used in this subChannel (e.g. when APC is used) */
            wns::Power txPower;
            //wns::CandI estimatedCandI; // not supported yet
            /** @brief Antenna pattern for beamforming; else empty.
                Yet unclear if this is constant over all subchannels or not. */
            wns::service::phy::ofdma::PatternPtr antennaPattern;
        }; // PhysicalResourceBlock

        /** @brief collection of and all spatial resources = MIMO streams or SDMA beams */
        typedef std::vector<PhysicalResourceBlock> PhysicalResourceBlockVector;

        /** @brief class to describe one SchedulingTimeSlot.
            There is one of this object in the subChannel for each timeSlot.
         */
        class SchedulingTimeSlot
                : virtual public wns::RefCountable // for SmartPtr
        {
        public:
            SchedulingTimeSlot();
            SchedulingTimeSlot(int _subChannel, int _timeSlot, int _numberOfBeams, simTimeType _slotLength);
            //SchedulingTimeSlot(const SchedulingTimeSlot&);

            ~SchedulingTimeSlot();
            /** @brief total used time in this SchedulingTimeSlot */
            simTimeType getUsedTime() const;
            /** @brief total free time on this SchedulingTimeSlot */
            simTimeType getFreeTime() const;
            /** @brief number of compounds inside this resource */
            int countScheduledCompounds() const;
            /** @brief true if there is nothing reserved(scheduled) in this block.
                For the uplink the master map entries have isEmpty==false.
                This question is NOT enough to allow it to be used. see subChannelIsUsable. */
            bool isEmpty() const;
            /** @brief get userID this resource has been reserved for or NULL if empty.
                Assumes that all contents are for ONE user only. */
            wns::scheduler::UserID getUserID() const;
            /** @brief get txPower assigned to this resource.
                Assumes that all contents are for ONE user only and have all equal power. */
            wns::Power getTxPower() const;
            /** @brief true if compound can be put into the ResourceBlock */
            bool pduFitsInto(strategy::RequestForResource& request,
                             MapInfoEntryPtr mapInfoEntry) const;
            /** @brief output structure (machine readable table for Matlab,Gnuplot,etc) */
            std::string dumpContents(const std::string& prefix) const;
            /** @brief doToString(): human-readable format */
            std::string toString() const;
            /** @brief Delete all compounds. But keep all other info (PhyMode, usedTime).
                This is called by the UL master scheduler,
                because there are no "real" compounds (just fakes). */
            void deleteCompounds();
            /** @brief extent the usage of each resource to 100% so that UL master map doesn't waste resources. */
            void grantFullResources();
            /** @brief perform modifications to use this as MasterMap (called in uplink slave scheduler) */
            void processMasterMap();
            /** @brief checks if there are UL resources available for given user */
            bool hasResourcesForUser(wns::scheduler::UserID user) const;
            /** @brief determine the length in bits stored in this resource */
            int getNetBlockSizeInBits() const;
        public:
            /** @brief collection of all PhysicalResourceBlocks (one per MIMO beam; only one for SISO) */
            PhysicalResourceBlockVector physicalResources; // [0..M-1] for MIMO
            /** @brief my own subChannelIndex as seen from outside (container) */
            int subChannelIndex;
            /** @brief number of resource blocks in time-direction */
            int timeSlotIndex;
            /** @brief size of resources in spatial direction.
                This can be beamforming beams (available for WiMAC)
                or MIMO paths. */
            int numberOfBeams;
            /** @brief fixed frame/slot length given from outside */
            simTimeType slotLength;
            /** @brief fixed frame/slot length given from outside */
            simTimeType timeSlotStartTime;
            /** @brief isUsable = flag to exclude certain resources from DSA */
            bool timeSlotIsUsable;

            HARQInfo harq;

         }; // SchedulingTimeSlot

        /** @brief can be used to send via an container compound to emulate one complete resource unit (chunk) */
        typedef SmartPtr<SchedulingTimeSlot> SchedulingTimeSlotPtr;

        /** @brief collection of and all temporal resources = TDMA slots */
        //typedef std::vector<SchedulingTimeSlot> SchedulingTimeSlotVector;
        /** @brief collection of and all temporal resources = TDMA slots. SmartPtr inside. */
        typedef std::vector<SchedulingTimeSlotPtr> SchedulingTimeSlotPtrVector;

        /** @brief class to describe one SchedulingSubChannel.
            There is one of this object in the SchedulingMap for each subChannel.
            SISO/MIMO see the same.
            Only that for MIMO it contains 1..M PhysicalResourceBlocks */
        class SchedulingSubChannel
                : virtual public wns::RefCountable // for SmartPtr
        {
        public:
            SchedulingSubChannel();
            SchedulingSubChannel(int _subChannelIndex, int _numberOfTimeSlots, int _numberOfBeams, simTimeType _slotLength);
            ~SchedulingSubChannel();
            /** @brief doToString(): human-readable format */
            std::string toString() const;
            /** @brief output structure (machine readable table for Matlab,Gnuplot,etc) */
            std::string dumpContents(const std::string& prefix) const;
            /** @brief total used time in this subchannel */
            simTimeType getUsedTime() const;
            /** @brief total free time on this subchannel */
            simTimeType getFreeTime() const;
            //void setNextPosition(simTimeType _nextPosition);

            /** @brief get "offset for new compounds" == used time for already scheduled compounds. Zero for empty subChannel */
            //simTimeType getNextPosition() const;

            /** @brief true if compound can be put into the SchedulingSubChannel */
            bool pduFitsInto(strategy::RequestForResource& request,
                             MapInfoEntryPtr mapInfoEntry) const;

            /** @brief returns number of bits that fit into the SchedulingSubChannel.
                Depends on PhyMode provided by mapInfoEntry. */
            int getFreeBitsOnSubChannel(MapInfoEntryPtr mapInfoEntry) const;

            //wns::service::phy::phymode::PhyModeInterfacePtr
            //getPhyModeUsedInResource(int timeSlot, int beam) const;
            //wns::Power
            //getTxPowerUsedInResource(int timeSlot, int beam) const;

            /** @brief true if there is nothing reserved(scheduled) in this block.
                For the uplink the master map entries have isEmpty==false.
                This question is NOT enough to allow it to be used. see subChannelIsUsable. */
            bool isEmpty() const;

            /** @brief Delete all compounds. But keep all other info (PhyMode, usedTime).
                This is called by the UL master scheduler,
                because there are no "real" compounds (just fakes). */
            void deleteCompounds();
            /** @brief extent the usage of each resource to 100% so that UL master map doesn't waste resources. */
            void grantFullResources();
            /** @brief perform modifications to use this as MasterMap (calles in uplink slave scheduler) */
            void processMasterMap();
            /** @brief checks if there are UL resources available for given user */
            bool hasResourcesForUser(wns::scheduler::UserID user) const;

        public:
            /** @brief my own subChannelIndex as seen from outside (container) */
            int subChannelIndex;
            /** @brief number of resource blocks in time-direction */
            int numberOfTimeSlots;
            /** @brief size of resources in spatial direction.
                This can be beamforming beams (available for WiMAC)
                or MIMO paths. */
            int numberOfBeams;
            /** @brief fixed frame/slot length given from outside */
            simTimeType slotLength;
            /** @brief isUsable = flag to exclude certain subchannels from DSA */
            bool subChannelIsUsable;
            /** @brief collection of all PhysicalResourceBlocks (one per MIMO beam; only one for SISO) */
            //PhysicalResourceBlockVector physicalResources; // [0..M-1] for MIMO
            /** @brief collection of and all temporal resources = TDMA slots. SmartPtr inside. */
            SchedulingTimeSlotPtrVector temporalResources;
        }; // SchedulingSubChannel

        /** @brief can be used to send via an container compound to emulate one complete resource unit (chunk) */
        typedef SmartPtr<SchedulingSubChannel> SchedulingSubChannelPtr;

        /** @brief collection of all subChannels */
        typedef std::vector<SchedulingSubChannel> SubChannelVector;
        /** @brief collection of all subChannels. SmartPtr inside. */
        //typedef std::vector<SchedulingSubChannelPtr> SubChannelPtrVector; // TODO?

        /** @brief this class contains the results over all subChannels */
        class SchedulingMap :
            public wns::IOutputStreamable,
            virtual public wns::RefCountable // for SmartPtr
        {
        public:
            SchedulingMap() {};

            /** @brief construct a new empty SchedulingMap which contains a number of SchedulingSubChannel's */
            SchedulingMap(simTimeType _slotLength, int _numberOfSubChannels, int _numberOfTimeSlots, int _numberOfBeams, int _frameNr);

            ~SchedulingMap();

            /** @brief true if compound can be put into the SchedulingSubChannel */
            bool pduFitsInto(strategy::RequestForResource& request,
                             MapInfoEntryPtr mapInfoEntry) const;

            /** @brief returns number of bits that fit into the SchedulingSubChannel.
                Depends on PhyMode provided by mapInfoEntry. */ 
           int getFreeBitsOnSubChannel(MapInfoEntryPtr mapInfoEntry) const;

            /** @brief put scheduled compound (one after another) into the SchedulingMap
                @return true if successful, false if not enough space.
            */
            bool addCompound(int subChannelIndex,
                             int timeSlot,
                             int beam,
                             simTimeType compoundDuration,
                             wns::scheduler::ConnectionID connectionID,
                             wns::scheduler::UserID userID,
                             wns::ldk::CompoundPtr compoundPtr,
                             wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr,
                             wns::Power txPower,
                             wns::service::phy::ofdma::PatternPtr pattern,
                             bool useHARQ
                );

            /** @brief put scheduled compound (one after another) into the SchedulingMap
                @param MapInfoEntryPtr contains result from doAdaptiveResourceScheduling(), but without compound contained
                @param compoundPtr is the PDU to be put into the resource
                @return true if successful, false if not enough space.
            */
            bool addCompound(strategy::RequestForResource& request,
                             MapInfoEntryPtr mapInfoEntry, // <- must not contain compounds yet
                             wns::ldk::CompoundPtr compoundPtr,
                             bool useHARQ
                );

            /** @brief get "offset for new compounds" == used time for already scheduled compounds. Zero for empty subChannel */
            simTimeType getNextPosition(int subChannel, int timeSlot, int beam) const;

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

            /** @brief total leftover power.
                (limitation over frequency=subChannel, but not over time) */
            wns::Power
            getRemainingPower(wns::Power totalPower, int timeSlot) const;

            simTimeType getSlotLength()  const { return slotLength; }
            int getNumberOfSubChannels() const { return numberOfSubChannels; }
            int getNumberOfBeams()       const { return numberOfBeams; }
            int getNumberOfCompounds()   const { return numberOfCompounds; }

            wns::service::phy::phymode::PhyModeInterfacePtr
            getPhyModeUsedInResource(int subChannelIndex, int timeSlot, int beam) const;
            wns::Power
            getTxPowerUsedInResource(int subChannelIndex, int timeSlot, int beam) const;

            /** @brief mask out certain subChannels (e.g. for resource partitioning) */
            void maskOutSubChannels(const UsableSubChannelVector& usableSubChannels);

            /** @brief make MapInfoCollection structure from myself */
            void convertToMapInfoCollection(MapInfoCollectionPtr collection /*return value*/);

            /** @brief true if there is nothing reserved(scheduled) in the whole schedulingMap. */
            bool isEmpty() const;

            /** @brief Delete all compounds from the map. But keep all other info (PhyMode, usedTime).
                This is called by the UL master scheduler,
                because there are no "real" compounds (just fakes). */
            void deleteCompounds();
            /** @brief extent the usage of each resource to 100% so that UL master map doesn't waste resources. */
            void grantFullResources();
            /** @brief perform modifications to use this as MasterMap (called in uplink slave scheduler) */
            void processMasterMap();
            /** @brief checks if there are UL resources available for given user */
            bool hasResourcesForUser(wns::scheduler::UserID user) const;

            /** @brief output structure (machine readable table for Matlab,Gnuplot,etc) */
            std::string
            dumpContents(const std::string& prefix) const;

            /** @brief output structure (structured text) */
            std::string
            toString();

            /** @brief output structure (conforming to IOutputStreamable) */
            virtual std::string
            doToString() const;

            /** @brief output SchedulingMap table header into already opened file. */
            static void writeHeaderToFile(std::ofstream& f);

            /** @brief output SchedulingMap structure (table file) into already opened file. */
            void writeToFile(std::ofstream& f, const std::string& prefix) const;

            /** @brief output SchedulingMap structure (table file). open/append+close. */
            void writeFile(std::string fileName) const;

        public:
            /** @brief collection of all subChannels */
            SubChannelVector subChannels;
        private:
            /** @brief index of the frame this map is for (system dependent) */
            int frameNr;
            /** @brief size of resources in time-direction */
            simTimeType slotLength;
            /** @brief size of resources in frequency-direction */
            int numberOfSubChannels;
            /** @brief number of resource blocks in time-direction (TDMA component) */
            int numberOfTimeSlots;
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
