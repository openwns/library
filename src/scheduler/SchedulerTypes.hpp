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

#ifndef WNS_SCHEDULER_SCHEDULERTYPES_HPP
#define WNS_SCHEDULER_SCHEDULERTYPES_HPP


#include <WNS/ldk/Command.hpp>
#include <WNS/ldk/Compound.hpp>
#include <WNS/ldk/FunctionalUnit.hpp>
#include <WNS/ldk/Classifier.hpp>
#include <WNS/node/Node.hpp>
#include <WNS/PowerRatio.hpp>
#include <WNS/CandI.hpp>

#include <WNS/service/phy/ofdma/Pattern.hpp>
#include <WNS/service/phy/phymode/PhyModeInterface.hpp>


#include <WNS/Enum.hpp>

#include <map>
#include <vector>
#include <set>
#include <list>
#include <string>
#include <sstream>

namespace wns { namespace scheduler {

	typedef wns::node::Interface* UserID;
	typedef wns::ldk::ClassificationID ConnectionID;
	typedef std::vector<ConnectionID> ConnectionVector;
	typedef std::list<ConnectionID> ConnectionList;
	typedef std::set<ConnectionID> ConnectionSet;

	typedef int Symbols;
	typedef int Bits;
	typedef int PHYmode; // TODO: use better type: chall PhyModeInterface
	typedef float DataRate;

	const int noPhyMode =  0;
	const int noCID = -1;

	typedef std::map<wns::scheduler::UserID, wns::CandI> Group;
	typedef std::set<wns::scheduler::UserID> UserSet;

	inline std::string
	printGroup(const wns::scheduler::Group& group) {
		std::stringstream s;
		s << "Group(";
		for (Group::const_iterator iter = group.begin();
		     iter != group.end(); ++iter)
		{
			s << iter->first->getName() << ",";
		}
		s << ")";
		return s.str();
	}

	inline std::string
	printConnectionVector(const wns::scheduler::ConnectionVector& connectionVector) {
		std::stringstream s;
		s << "ConnectionVector(";
		for (ConnectionVector::const_iterator iter = connectionVector.begin();
		     iter != connectionVector.end(); ++iter)
		{
		  s << (*iter) << ",";
		}
		s << ")";
		return s.str();
	}

    /*
    template <class ContainerTemplateType>;
	inline std::string
	printContainer<ContainerTemplateType>(ContainerTemplateType& container) {
		std::stringstream s;
		s << "Container(";
		s << ")";
		return s.str();
	}
    */

	/** @brief define stream operator for class Group */
	inline std::ostream&
	operator<< (std::ostream& s, const wns::scheduler::Group& group) {
		s << "Group(";
		for (Group::const_iterator iter = group.begin();
		     iter != group.end(); ++iter)
		{
			s << iter->first->getName() << ",";
		}
		s << ")";
		return s;
	}

	/** @brief Data struct used to exchange info about user's power
	 * capabilities */
	struct PowerCapabilities
	{
		PowerCapabilities() :
			maxPerSubband(),
			nominalPerSubband(),
			maxOverall()
		{}

		explicit
		PowerCapabilities(const wns::pyconfig::View& config) :
			maxPerSubband(config.get<wns::Power>("maxPerSubband")),
			nominalPerSubband(config.get<wns::Power>("nominalPerSubband")),
			maxOverall(config.get<wns::Power>("maxOverall"))
		{}

		wns::Power maxPerSubband;
		wns::Power nominalPerSubband;
		wns::Power maxOverall;
	};

	/** @brief Info about joint power and resource allocation, currently
	 * used by PCRR scheduling strategy */
	struct PowerAllocation {
		wns::Power txPowerPerSubband;
		UserID user;
		uint32_t maxNumSubbands;
		double resourceShare;

		PowerAllocation() :
			txPowerPerSubband(),
			user(),
			maxNumSubbands(),
			resourceShare()
		{}

		std::string
		toString() const
		{
			std::stringstream ss;
			ss << user->getName() << ": " << maxNumSubbands << " simultaneously with " << txPowerPerSubband << " each." << std::endl;
			return ss.str();
		}
	};

	typedef std::map<UserID, PowerAllocation> PowerMap;

	struct Grouping {
		std::string getDebugOutput() {
			std::stringstream ss;
			for (unsigned int i = 0; i < groups.size(); ++i) {
				ss << "Group" << i << "(";
				for (Group::iterator iter = groups[i].begin();
				     iter != groups[i].end();
				     ++iter)
					ss << (*iter).first->getName() << " @ "
					   << (*iter).second.C / (*iter).second.I << ", )\n";
			}
			return ss.str();
		}

		wns::Ratio shareOfPowerPerStreams(uint groupNumber){

			return shareOfPowerPerStreams( groups.at(groupNumber) );

		}

		wns::Ratio shareOfPowerPerStreams(Group group){
			// transmitting multiple streams the available Tx power
			// is shared among all streams
			assure( !group.empty() , "group does not exists");

			uint concurrentStreams = group.size();
			return wns::Ratio().from_factor(1.0 / concurrentStreams);
		}

		wns::Ratio eirpReductionOfPower(uint groupNumber){

			return eirpReductionOfPower( groups.at(groupNumber) );

		}

		wns::Ratio eirpReductionOfPower(Group group){
			// in EIRP limited spectrum, the maximum adaptive
			// antenna gain of the sum of all patterns have to be
			// normalised to one
			assure( !group.empty() , "group does not exists");

			wns::service::phy::ofdma::SumPattern sumPattern;
			assure(sumPattern.getSize() == 0, "new sum pattern is not empty");

			for (Group::iterator itr = group.begin();
			     itr != group.end();
			     itr++) {
				wns::service::phy::ofdma::PatternPtr patternOfUser = patterns[itr->first];
				// In case of no beamforming, adding the invalid
				// pattern would fail, therefore we avoid it
				/** @todo pab, 20070327: for mixed operation of
				 *  users with and without beams we would have
				 *  to add a real omni-pattern here.
				 */
				if (patternOfUser != wns::service::phy::ofdma::PatternPtr())
					sumPattern.add(patternOfUser);
			}
			// again, for no beamforming we return the standard factor
			if (sumPattern.getSize() == 0)
				return wns::Ratio().from_factor( 1 );

			return wns::Ratio().from_factor( 1 / sumPattern.getMaxGain().get_factor() );
		}

		std::map<UserID, wns::service::phy::ofdma::PatternPtr> patterns;
		std::vector<Group> groups;
		std::map<UserID, int> userGroupNumber; // what group is user in
	};

	/** @brief data structure which is used to store channel quality  */
	class ChannelQualityOnOneSubband
	{
	public:
		ChannelQualityOnOneSubband():
			Pathloss(),
			Interference(),
			subBandIndex(0)
			{
			}

		ChannelQualityOnOneSubband(wns::Ratio pathloss,  wns::Power interference, int indexOfSubband):
			Pathloss(pathloss),
			Interference(interference),
			subBandIndex(indexOfSubband)
			{
			}

		wns::Ratio Pathloss;
		wns::Power Interference;    // (I + N)
		int subBandIndex; // libwns scheduler index, not real(OFDMA) index
	};

        typedef std::vector<ChannelQualityOnOneSubband> ChannelsQualitiesOnAllSubBand; // index is real(OFDMA) subchannel number

	//class which is used to record the txPower usage
	class TxPower4PDU
	{
	public:
		TxPower4PDU():
			beginTime(0.0),
			endTime(0.0),
			txPower()
			{}

		TxPower4PDU(double beginT, double endT, wns::Power usedPower):
			beginTime(beginT),
			endTime(endT),
			txPower(usedPower)
			{}

		double beginTime;
		double endTime;
		wns::Power txPower;
	};

	typedef std::vector<TxPower4PDU> usedTxPowerOnOneChannel;
	typedef std::vector<usedTxPowerOnOneChannel> usedTxPowerOnAllChannels;

	struct ConnectionsCharacteristics {

		ConnectionsCharacteristics():
			phyModePtr(),
			ofdmaPatternPtr(),
			txPower(),
			estimatedCandI(),
			sinr(),
			timeNeed(0.0)
			{}

		wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr;
		wns::service::phy::ofdma::PatternPtr ofdmaPatternPtr;
		wns::Power txPower;
		wns::CandI estimatedCandI;
		wns::Ratio sinr;
		simTimeType timeNeed;
	};

	typedef std::map< ConnectionID, ConnectionsCharacteristics > ConnectionAttributes;

	/** @brief This is used e.g. for building ResourceRequests */
	struct QueueStatus {
		QueueStatus()
		{
			numOfBits = 0;
			numOfCompounds = 0;
		}

		unsigned int numOfBits;
		unsigned int numOfCompounds;
	};

	/** @brief Holds QueueStatus for all cids. ResourceRequests contain such a container. */
	typedef wns::container::Registry<ConnectionID, QueueStatus> QueueStatusContainer;

}} // namespace wns::scheduler
#endif // WNS_SCHEDULER_SCHEDULERTYPES_HPP


