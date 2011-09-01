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

#include <WNS/scheduler/tests/LAProxyStub.hpp>
#include <WNS/scheduler/tests/PhyModeMapperStub.hpp>
#include <WNS/pyconfig/Parser.hpp>

using namespace wns::scheduler;
using namespace wns::scheduler::tests;

LAProxyStub::LAProxyStub() :
	  phyModeMapper_(NULL)
{
	wns::pyconfig::Parser parser;
	parser.loadString("import openwns.PhyMode\n"
			  "from openwns.interval import Interval\n"
			  "phyModeMap = openwns.PhyMode.PhyModeMapperDropin()\n"
			  "phyModeMap.setMinimumSINR(3.0)\n"
			  "phyModeMap.addPhyMode(Interval(-200.0,   3.0, \"(]\"), openwns.PhyMode.PhyModeDropin1())\n"
			  "phyModeMap.addPhyMode(Interval(   3.0,  10.0, \"(]\"), openwns.PhyMode.PhyModeDropin2())\n"
			  "phyModeMap.addPhyMode(Interval(  10.0, 200.0, \"(]\"), openwns.PhyMode.PhyModeDropin3())\n"
			  "\n"
		);
	phyModeMapper_ = new wns::scheduler::tests::PhyModeMapper(parser.get("phyModeMap"));
}

LAProxyStub::~LAProxyStub()
{
    delete phyModeMapper_;
}

wns::service::phy::phymode::PhyModeMapperInterface*
LAProxyStub::getPhyModeMapper() const
{
	return phyModeMapper_;
}

wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface>
LAProxyStub::getBestPhyMode(const wns::Ratio& sinr)
{
	return getPhyModeMapper()->getBestPhyMode(sinr);
}

ChannelQualityOnOneSubChannel
LAProxyStub::estimateTxSINRAt(const UserID user, int slot = 0)
{
    return txCQ_;
}

void
LAProxyStub::estimateTxSINRAt(ChannelQualityOnOneSubChannel cq)
{
    txCQ_ = cq;
}

ChannelQualityOnOneSubChannel
LAProxyStub::estimateRxSINROf(const UserID user, int slot = 0)
{
    return rxCQ_;
}

void
LAProxyStub::estimateRxSINROf(ChannelQualityOnOneSubChannel cq)
{
    rxCQ_ = cq;
}

