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

#include <WNS/scheduler/strategy/staticpriority/persistentvoip/LinkAdaptation.hpp>

using namespace wns::scheduler::strategy::staticpriority::persistentvoip;


STATIC_FACTORY_REGISTER(AtStart, ILinkAdaptation, "AtStart");
STATIC_FACTORY_REGISTER(All, ILinkAdaptation, "All");

LinkAdaptation::LinkAdaptation() :
    registry_(NULL),
    slotDuration_(0.0)
{
}

Frame::SearchResultSet
LinkAdaptation::setTBSizes(const Frame::SearchResultSet& tbs, ConnectionID cid, Bit pduSize)
{
    assure(!tbs.empty(), "Cannot chose from empty set");
    assure(registry_ != NULL, "Need RegistryProxy");
    assure(slotDuration_ > 0, "Need positive slot duration");

    return doSetTBSizes(tbs, cid, pduSize);

}

void
LinkAdaptation::setRegistryProxy(RegistryProxyInterface* reg)
{
    assure(reg != NULL, "Cannot set RegistryProxy to NULL");

    registry_ = reg;
}

void
LinkAdaptation::setSlotDuration(wns::simulator::Time sd)
{
    assure(sd > 0.0, "Need positive slot duration");

    slotDuration_ = sd;
}

unsigned int
LinkAdaptation::getTBSize(Bit pduSize, 
    wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr)
{
    Bit bitPerRB = phyModePtr->getBitCapacityFractional(slotDuration_);
    return ceil(double(pduSize) / double(bitPerRB));   
}

Frame::SearchResultSet
AtStart::doSetTBSizes(const Frame::SearchResultSet& tbs, ConnectionID cid, Bit pduSize)
{
    UserID user = registry_->getUserForCID(cid);

    Frame::SearchResultSet result;
    Frame::SearchResultSet::iterator it;

    for(it = tbs.begin(); it != tbs.end(); it++)
    {
        unsigned int testLength = 0;
        unsigned int neededLength = 0;
        wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr;

        do
        {
            testLength++;
            std::set<unsigned int> rbs;
            for(int i = 0; i < it->length; i++)
                rbs.insert(it->start + i);
            
            wns::Ratio effSINR;
            /*TODO Downlink, Power*/
            effSINR = registry_->getEffectiveUplinkSINR(user, rbs, wns::Power::from_dBm(4.0));
    
            /* Which MCS can we use on RBs testLength RBs? */
            phyModePtr = registry_->getBestPhyMode(effSINR);

            /* How many RBs do we need if we use this MCS? */
            neededLength = getTBSize(pduSize, phyModePtr);
        }
        while(testLength <= it->length && neededLength > testLength);
        if(testLength <= it->length)
        {
            Frame::SearchResult sr = *it;
            sr.tbLength = neededLength;
            sr.tbStart = sr.start;
            sr.phyMode = phyModePtr;
            result.insert(sr);
        }
        
    }
    return result; 
}

Frame::SearchResultSet
All::doSetTBSizes(const Frame::SearchResultSet& tbs, ConnectionID cid, Bit pduSize)
{
    return tbs;
}

