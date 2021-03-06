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

#include <WNS/service/phy/phymode/SNR2MIInterface.hpp>

#include <WNS/StaticFactoryBroker.hpp>
#include <WNS/Singleton.hpp>
#include <WNS/StaticFactory.hpp>

using namespace wns::service::phy::phymode;

SNR2MIInterface*
SNR2MIInterface::getSNR2MImapper(const wns::pyconfig::View& config)
{
	// Define shorthand for the staticfactorybroker typename
	typedef wns::StaticFactoryBroker<SNR2MIInterface, SNR2MICreator>  BrokerType;

	// Obtain the broker from its singleton
	BrokerType* broker = wns::SingletonHolder<BrokerType>::getInstance();

	// read name and obtain the right object from the broker
	std::string name = config.get<std::string>("nameInSNR2MIFactory");
	return broker->procure(name);
}
