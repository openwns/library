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

#include <WNS/ldk/FlowSeparator.hpp>
#include <WNS/ldk/Layer.hpp>

#include <WNS/StaticFactory.hpp>
#include <WNS/module/Base.hpp>

using namespace wns::ldk;
using namespace wns::ldk::flowseparator;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	FlowSeparator,
	FunctionalUnit,
	"wns.FlowSeparator",
	FUNConfigCreator
	);

FlowSeparator::FlowSeparator(
	fun::FUN* fuNet,
	const pyconfig::View& _config,
	std::auto_ptr<KeyBuilder> _keyBuilder,
	std::auto_ptr<flowseparator::NotFoundStrategy> _notFound) :

	CommandTypeSpecifier<>(fuNet),
	HasReceptor<>(),
	HasConnector<>(),
	HasDeliverer<>(),
	NotCloneable(),
	instances(),
	instanceBusy(NULL),
	config(_config),
	keyBuilder(_keyBuilder),
	notFound(_notFound),
	dis(),
	logger(_config.getView("logger"))
{
} // FlowSeparator


FlowSeparator::FlowSeparator(
	fun::FUN* fuNet,
	const pyconfig::View& _config) :

	CommandTypeSpecifier<>(fuNet),
	HasReceptor<>(),
	HasConnector<>(),
	HasDeliverer<>(),
	NotCloneable(),
	instances(),
	instanceBusy(NULL),
	config(_config),
	keyBuilder(),
	notFound(),
	dis(),
	logger(_config.get("logger"))
{
	pyconfig::View notFoundConfig(config, "notFound");
	std::string notFoundName = notFoundConfig.get<std::string>("__plugin__");
	this->notFound = std::auto_ptr<flowseparator::NotFoundStrategy>(
		flowseparator::NotFoundStrategyFactory::creator(notFoundName)->create(fuNet, notFoundConfig));

	pyconfig::View keyBuilderConfig(config, "keyBuilder");
	std::string keyBuilderName = keyBuilderConfig.get<std::string>("__plugin__");
	this->keyBuilder = std::auto_ptr<KeyBuilder>(
		KeyBuilderFactory::creator(keyBuilderName)->create(fuNet, keyBuilderConfig));
} // FlowSeparator


FlowSeparator::~FlowSeparator()
{
	assure(instanceBusy == NULL, "One FU in FlowSeparator is still busy. Can't delete FlowSeparator!");

	while(instances.empty() == false)
	{
		delete instances.begin()->second;
		instances.erase(instances.begin());
	}

	notFound.reset();
	keyBuilder.reset();
} // ~FlowSeparator


void
FlowSeparator::onFUNCreated()
{
	/*
	 * The prototype is registered at the FUN itself, so its onFUNCreated method
	 * gets called by the FUN directly.
	 * <br>
	 * The keyBuilder itself may need to make friends, since the keys will usually
	 * need the content of some Commands at instantiation time.
	 * Resolving these dependencies at keyBuilder startup time instead during Key
	 * instantiation pushes run-time complexity to configuration time.
	 */
	keyBuilder->onFUNCreated();
	notFound->onFUNCreated();
} // onFUNCreated


void
FlowSeparator::doSendData(const CompoundPtr& compound)
{
	FunctionalUnit* functionalUnit = tryGetInstanceAndInsertPermanent(compound, Direction::OUTGOING());

	functionalUnit->sendData(compound);
} // doSendData


void
FlowSeparator::doOnData(const CompoundPtr& compound)
{
	FunctionalUnit* functionalUnit = tryGetInstanceAndInsertPermanent(compound, Direction::INCOMING());

	instanceBusy = functionalUnit;
	functionalUnit->onData(compound);
	instanceBusy = NULL;
} // doOnData


std::size_t
FlowSeparator::size() const
{
	return instances.size();
} // size


FunctionalUnit*
FlowSeparator::getInstance(const ConstKeyPtr& key) const
{
	InstanceMap::const_iterator it = instances.find(key);
	if(it == instances.end())
	{
		return NULL;
	}

	return it->second;
} // getInstance

FunctionalUnit*
FlowSeparator::getInstance(const CompoundPtr& compound, int direction) const
{
	return(this->getInstance(this->getKey(compound, direction)));
} // getInstance

ConstKeyPtr
FlowSeparator::getKey(const CompoundPtr& compound, int direction) const
{
	ConstKeyPtr key = (*keyBuilder)(compound, direction);
	return key;
}


void
FlowSeparator::addInstance(const ConstKeyPtr& key)
{
	FunctionalUnit* functionalUnit = notFound->ifNotFound(key);
	addInstance(key, functionalUnit);
}

void
FlowSeparator::addInstance(const ConstKeyPtr& key, FunctionalUnit* functionalUnit)
{
	assure(NULL == getInstance(key),
	       "trying to add an instance for an already known key.");

	MESSAGE_BEGIN(NORMAL, logger, m, this->getFUN()->getName());
	m <<": add Instance/Flow      Key: "<< key->str()
	  << ";       FU: " << functionalUnit->getName();
	MESSAGE_END();

	this->integrate(key, functionalUnit);
} // addInstance


void
FlowSeparator::removeInstance(const ConstKeyPtr& key)
{
	assure(NULL != getInstance(key),
	       "trying to remove an instance using an unknown key.");

	MESSAGE_BEGIN(NORMAL, logger, m, this->getFUN()->getName());
	m <<": remove Instance/Flow      Key: " << key->str()
	  << ";       FU: " << getInstance(key)->getName();
	MESSAGE_END();

	this->disintegrate(key);
} // removeInstance


bool
FlowSeparator::doIsAccepting(const CompoundPtr& compound) const
{
	try
	{
		FunctionalUnit* candidate = this->_getInstance(compound, Direction::OUTGOING());
		// either no one may be busy, or the instance in question
		if (instanceBusy == NULL || instanceBusy == candidate)
		{
			return candidate->isAccepting(compound);
		}
		else
		{
			return false;
		}
	}
	catch(const InstanceNotFound& ifn)
	{
 		MESSAGE_SINGLE(
			VERBOSE,
			logger,
			"this->getFUN()->getName(): " <<
			"creating new instance for key (" <<
			ifn.key->str() <<
			") temporarily.");
		FunctionalUnit* candidate = this->notFound->ifNotFound(ifn.key);
		candidate->getConnector()->set(this->getConnector()->get());
		candidate->getReceptor()->set(this->getReceptor()->get());
		candidate->getDeliverer()->set(this->getDeliverer()->get());
		bool accepting = candidate->isAccepting(compound);
		delete candidate;
		return accepting;
	}
	// throw on
	catch(...)
	{
		throw;
	}
} // doIsAccepting


void
FlowSeparator::doWakeup()
{
	InstanceMap::iterator begin = instances.begin();
	InstanceMap::iterator somewhere = instances.begin();
	InstanceMap::iterator end = instances.end();

	// set jump-in point somewhere
	size_t offset = size_t(dis() * instances.size());
	for( size_t i = 0; i < offset; i++)
		somewhere++;

	// and walk from somewhere to the end
	for(InstanceMap::iterator i = somewhere;
	    i != end; ++i)
	{
		instanceBusy = (*i).second;
		(*i).second->wakeup();
		instanceBusy = NULL;
	}

	// walk from the beginning to the jump-in point somewhere
	for(InstanceMap::iterator i = begin;
	    i != somewhere; ++i)
	{
		instanceBusy = (*i).second;
		(*i).second->wakeup();
		instanceBusy = NULL;
	}
} // doWakeup


FunctionalUnit*
FlowSeparator::_getInstance(const CompoundPtr& compound, int direction) const
{
	ConstKeyPtr key = (*keyBuilder)(compound, direction);

	InstanceMap::const_iterator it = instances.find(key);

	if(it == instances.end())
	{
		throw InstanceNotFound(key);
	}

	MESSAGE_BEGIN(VERBOSE, logger, m,"this->getFUN()->getName()");
	m << ": "
	  << "reusing instance for key "
	  << key->str();
	MESSAGE_END();

	return it->second;
} // getInstance


FunctionalUnit*
FlowSeparator::tryGetInstanceAndInsertPermanent(const CompoundPtr& compound, int direction)
{
	FunctionalUnit* fu = NULL;
	try
	{
		fu = this->_getInstance(compound, direction);
	}
	catch(const InstanceNotFound& ifn)
	{
 		MESSAGE_SINGLE(
			VERBOSE,
			logger,
			"this->getFUN()->getName(): " <<
			"adding new instance for key (" <<
			ifn.key->str() <<
			") permanently to FlowSeparator");
		fu = this->notFound->ifNotFound(ifn.key);
 		this->addInstance(ifn.key, fu);
	}
	// throw on
	catch(...)
	{
		throw;
	}
	return fu;
}

void
FlowSeparator::integrate(const ConstKeyPtr& key, FunctionalUnit* functionalUnit)
{
	/**
	 * @todo If something changes in the layout of the FUN, it will not be
	 * propagated to the FUs inside this FlowSep!
	 */
	functionalUnit->getConnector()->set(this->getConnector()->get());
	functionalUnit->getReceptor()->set(this->getReceptor()->get());
	functionalUnit->getDeliverer()->set(this->getDeliverer()->get());

	instances[key] = functionalUnit;
} // instance.integrate


void
FlowSeparator::disintegrate(const ConstKeyPtr& key)
{
	InstanceMap::iterator it = instances.find(key);
	assure(it != instances.end(),
	       "trying to disintegrate FU for an unknown key.");

	assure( instanceBusy != it->second,
		"FlowSeparator::disintegrate: Can't disintegrate busy Instance/Flow!");

	delete it->second;
	instances.erase(it);
} // instance.disintegrate


