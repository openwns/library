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

#include <WNS/ldk/Group.hpp>
#include <WNS/ldk/utils.hpp>
#include <WNS/ldk/Layer.hpp>

#include <string>

using namespace wns::ldk;

STATIC_FACTORY_REGISTER_WITH_CREATOR(Group,
									 FunctionalUnit,
									 "wns.Group",
									 FUNConfigCreator);

Group::Group(
	fun::FUN* fuNet,
	const pyconfig::View& _config) :

	CommandTypeSpecifier<>(fuNet),
	config(_config),
	logger("WNS", "Group"),
	topRedirector(this),
	bottomRedirector(this)
{
	sub = new fun::Sub(getFUN());

	pyconfig::View funView(config, "fun");
	configureFUN(sub, funView);

	topFU = sub->getFunctionalUnit(config.get<std::string>("top"));
	bottomFU = sub->getFunctionalUnit(config.get<std::string>("bottom"));

	topRedirector.upConnect(topFU);
	topFU->getReceptor()->add(&topRedirector);

	bottomFU->downConnect(&bottomRedirector);
}


Group::Group(const Group& other) :
    CompoundHandlerInterface<FunctionalUnit>(other),
	CommandTypeSpecifierInterface(other),
	HasConnectorInterface(other),
	HasReceptorInterface(other),
	HasDelivererInterface(other),
	CloneableInterface(other),
	IOutputStreamable(other),
	PythonicOutput(other),
	FunctionalUnit(other),
	HasReceptor<>(other),
	HasConnector<>(other),
	HasDeliverer<>(other),
	CommandTypeSpecifier<>(other),
	Cloneable<Group>(other),

	config(other.config),
	logger("WNS", "Group"),

	topRedirector(this),
	bottomRedirector(this)
{
	sub = new fun::Sub(getFUN());

	pyconfig::View funView(config, "fun");
	configureFUN(sub, funView);
	sub->onFUNCreated();

	topFU = sub->getFunctionalUnit(config.get<std::string>("top"));
	bottomFU = sub->getFunctionalUnit(config.get<std::string>("bottom"));

	topRedirector.upConnect(topFU);
	topFU->getReceptor()->add(&topRedirector);

	bottomFU->downConnect(&bottomRedirector);
}


Group::~Group()
{
	delete sub;
}


void
Group::onFUNCreated()
{
	sub->onFUNCreated();
} // onFUNCreated


bool
Group::doIsAccepting(const CompoundPtr& compound) const
{
	return topFU->isAccepting(compound);
} // isAccepting


void
Group::doSendData(const CompoundPtr& compound)
{
	topFU->sendData(compound);
} // doSendData


void
Group::doOnData(const CompoundPtr& compound)
{
	bottomFU->onData(compound);
} // doOnData


void
Group::doWakeup()
{
	bottomFU->wakeup();
} // wakeup


fun::Sub*
Group::getSubFUN() const
{
	return sub;
} // getSubFUN


void
Group::setName(std::string _name)
{
	FunctionalUnit::setName(_name);
	getSubFUN()->setNameParentFU(_name);
}



