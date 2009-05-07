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

#ifndef WNS_SCHEDULER_MAPINFOENTRY_H
#define WNS_SCHEDULER_MAPINFOENTRY_H

#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/service/phy/phymode/PhyModeInterface.hpp>
#include <WNS/service/phy/ofdma/Pattern.hpp>
#include <WNS/CandI.hpp>
#include <WNS/ldk/Compound.hpp>
#include <WNS/SmartPtr.hpp>
#include <WNS/PowerRatio.hpp>
#include <WNS/IOutputStreamable.hpp>

#include <list>

namespace wns { namespace node {
	class Interface;
}}

namespace wns { namespace scheduler {

    typedef std::list<wns::ldk::CompoundPtr> CompoundList;
    const int undefinedTime = -1.0;

    /** @brief this is the data structure to specify the contents of one subChannel.
	There may be more than one PDU in it -> list<CompoundPtr> */
    class MapInfoEntry
      : virtual public wns::RefCountable,
	public wns::IOutputStreamable
    {
    public:
      MapInfoEntry();
      ~MapInfoEntry();
      /** @brief required for IOutputStreamable */
      virtual std::string doToString() const;
      std::string toString() const;
      /** @brief relative time; 0.0=slot/frame start */
      simTimeType start;
      /** @brief relative time of packet end */
      simTimeType end;
      /** @brief length of the compound in timeunits */
      //simTimeType compoundDuration;
      /** @brief there is no such thing as a connectionID or cid in a mapInfo */
      /** @brief destination node user (userID) */
      wns::node::Interface* user;
      /** @brief frameNr is the index within the superframe. Used for advance scheduling */
      int frameNr;
      /** @brief can be abstract subband from scheduling result or real world SubChannelNr after channelMapping */
      int subBand;
      /** @brief spatial channel (either beamforming or MIMO) */
      int beam;
      /** @brief for Adaptive Power Control (APC) */
      wns::Power txPower;
      /** @brief all compound must use the same phyMode */
      wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr;
      /** @brief Antenna pattern for beamforming; else empty */
      wns::service::phy::ofdma::PatternPtr pattern;
      /** @brief signal and noise+interference power assumed at receiver */
      wns::CandI estimatedCandI;
      /** @brief list because more than one compound can be in it */
      wns::scheduler::CompoundList compounds;
    }; // MapInfoEntry

    /** @brief created in the strategies; no need for memory tracking later */
    typedef SmartPtr<MapInfoEntry> MapInfoEntryPtr;
    /** @brief collection of subChannel informations (usually all subchannels) */
    typedef std::list<MapInfoEntryPtr> MapInfoCollectionList; // <- cannot make SmartPtr of that
    /** @brief this class contains a list of MapInfoEntryPtr and can itself be packed into a SmartPtr */
    class MapInfoCollection
      : virtual public wns::RefCountable,
	public MapInfoCollectionList // std::list<MapInfoEntryPtr>
    {
    public:
      MapInfoCollection() {};
      ~MapInfoCollection() {};
      /** @brief concatenate the other list at the end of the current list.
	  Does not change otherList.
	  Does not require or use ordering (which is dangerous if "operator<" goes by SmartPtr).
      */
      virtual void join(const MapInfoCollection& otherList);
    };
    /** @brief created in the strategies; no need for memory tracking later */
    typedef SmartPtr<MapInfoCollection> MapInfoCollectionPtr;

    /** @brief print the complete collection */
    inline std::string
    printMapInfoCollection(const MapInfoCollectionPtr bursts)
    {
      std::stringstream s;
      int b = 1;
      s << "MapInfoCollection:"<<std::endl;
      for ( MapInfoCollection::const_iterator iter = bursts->begin(); iter != bursts->end(); ++iter )
	{
	  s << "###  BurstNr:      " << b++;
	  s << " (Ptr=" << iter->getPtr() << ")";
	  s << (*iter)->toString();
	}
      return s.str();
    } // printMapInfoCollection()
    /** @brief print the complete collection */
    inline std::string
    printMapInfoCollection(const MapInfoCollection& bursts)
    {
      std::stringstream s;
      int b = 1;
      s << "MapInfoCollection:"<<std::endl;
      for ( MapInfoCollection::const_iterator iter = bursts.begin(); iter != bursts.end(); ++iter )
	{
	  s << "###  BurstNr:      " << b++;
	  s << " (Ptr=" << iter->getPtr() << ")";
	  s << (*iter)->toString();
	}
      return s.str();
    } // printMapInfoCollection()
    /** @brief stream operator for class */
    /*
    inline std::ostream&
    operator<< (std::ostream& s, const MapInfoCollectionPtr bursts) {
      s << printMapInfoCollection(bursts);
      return s;
    }
    // ^ using this leads to the following strange error:
    // .build/dbg/libwns--main--1.0/addOn/src/scheduler/queue/tests/SimpleQueueTest.cpp:172:   instantiated from here
    // /usr/include/cppunit/TestAssert.h:49: error: ambiguous overload for 'operator<<' in 'ost << x'
    // include/WNS/scheduler/MapInfoEntry.hpp:89: note:                 std::ostream& wns::scheduler::operator<<(std::ostream&, wns::scheduler::MapInfoCollectionPtr)
    */
}}

#endif


