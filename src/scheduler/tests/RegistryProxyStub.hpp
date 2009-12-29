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

#ifndef WNS_SCHEDULER_TESTS_REGISTRYPROXYSTUB_HPP
#define WNS_SCHEDULER_TESTS_REGISTRYPROXYSTUB_HPP

#include <WNS/scheduler/RegistryProxyInterface.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>

#include <WNS/Interval.hpp>
#include <WNS/container/RangeMap.hpp>
#include <WNS/service/phy/phymode/PhyModeMapperInterface.hpp>
#include <map>
#include <string>

namespace wns { namespace scheduler { namespace tests {

            /** @brief this class is used for tests only */
            class RegistryProxyStub
                    : public RegistryProxyInterface {
            public:
                RegistryProxyStub();
                ~RegistryProxyStub();

                // The Interface:
                UserID getUserForCID(ConnectionID cid);
                wns::service::dll::UnicastAddress getPeerAddressForCID(wns::scheduler::ConnectionID cid);
                ConnectionVector getConnectionsForUser(const UserID user);
                ConnectionID getCIDforPDU(const wns::ldk::CompoundPtr& compound);
                void setFriends(const wns::ldk::CommandTypeSpecifierInterface* _classifier);
                void setFUN(const wns::ldk::fun::FUN* fun);
                std::string getNameForUser(const UserID user);
                wns::service::phy::phymode::PhyModeMapperInterface* getPhyModeMapper() const;
                wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> getBestPhyMode(const wns::Ratio& sinr);
                UserID getMyUserID();
                simTimeType getOverhead(int numBursts);
                wns::CandI estimateTxSINRAt(const UserID user);
                wns::CandI estimateRxSINROf(const UserID user);
                wns::Power estimateInterferenceAt(const UserID user);
                wns::Power estimateInterferenceStdDeviation(const UserID user);
                Bits getQueueSizeLimitPerConnection();
                void setQueueSizeLimitPerConnection(Bits bits);
                wns::service::dll::StationType getStationType(const UserID user);
                UserSet filterReachable(UserSet users); // soon obsolete
                UserSet filterReachable(UserSet users, const int frameNr);
                wns::scheduler::ConnectionSet filterReachable(wns::scheduler::ConnectionSet connections, const int frameNr);
                wns::scheduler::PowerMap calcULResources(const wns::scheduler::UserSet&, uint32_t) const;
                wns::scheduler::UserSet getActiveULUsers() const;
                int getTotalNumberOfUsers(const wns::scheduler::UserID user) const;

                virtual ChannelQualitiesOnAllSubBandsPtr getChannelQualities4UserOnUplink(UserID user, int frameNr);
                virtual ChannelQualitiesOnAllSubBandsPtr getChannelQualities4UserOnDownlink(UserID user, int frameNr);

                virtual wns::scheduler::PowerCapabilities
                getPowerCapabilities(const UserID user) const;

                virtual wns::scheduler::PowerCapabilities
                getPowerCapabilities() const;

                int
                getNumberOfPriorities();

                virtual void
                registerCID(wns::scheduler::ConnectionID cid, wns::scheduler::UserID userID/*nextHop!*/) {};

                virtual void
                deregisterCID(wns::scheduler::ConnectionID cid, const wns::scheduler::UserID userID) {};

                virtual void
                deregisterUser(const wns::scheduler::UserID userID) {};

                wns::scheduler::ConnectionList&
                getCIDListForPriority(int priority);

                wns::scheduler::ConnectionSet
                getConnectionsForPriority(int /*priority*/) {};

                std::string
                compoundInfo(const wns::ldk::CompoundPtr& compound);

                const wns::service::phy::phymode::PhyModeInterfacePtr
                getPhyMode(ConnectionID /*cid*/);

                int
                getPriorityForConnection(wns::scheduler::ConnectionID /*cid*/);

                bool
                getDL() const;

                virtual bool
                getCQIAvailable() const;

                // The functions to modify the stub's state and define the return values
                void setCIDforPDU(const wns::ldk::CompoundPtr& compound, ConnectionID cid);
                void associateCIDandUser(ConnectionID cid, UserID user);

            private:
                std::map<wns::ldk::CompoundPtr, ConnectionID> compound2CIDmap;
                std::map<UserID, ConnectionVector> User2CID;
                std::map<ConnectionID, UserID> CID2User;
                wns::ldk::CommandTypeSpecifierInterface* classifier;
                wns::service::phy::phymode::PhyModeInterfacePtr phyMode;
                wns::service::phy::phymode::PhyModeMapperInterface *phyModeMapper;
                UserID myUserID;
                Bits queueSizeLimit;
            };
        } // tests
    } // scheduler
} // wns
#endif // WNS_SCHEDULER_TESTS_REGISTRYPROXYSTUB_HPP


