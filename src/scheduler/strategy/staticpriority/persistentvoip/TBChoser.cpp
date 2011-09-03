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

#include <WNS/scheduler/strategy/staticpriority/persistentvoip/TBChoser.hpp>
#include <sstream>
#include <limits>

using namespace wns::scheduler::strategy::staticpriority::persistentvoip;


STATIC_FACTORY_REGISTER(First, ITBChoser, "First");
STATIC_FACTORY_REGISTER(BestFit, ITBChoser, "BestFit");
STATIC_FACTORY_REGISTER(WorstFit, ITBChoser, "WorstFit");
STATIC_FACTORY_REGISTER(Random, ITBChoser, "Random");

Frame::SearchResult
TBChoser::choseTB(const Frame::SearchResultSet& tbs)
{
    assure(!tbs.empty(), "Cannot chose from empty set");

    Frame::SearchResult sr = doChoseTB(tbs);
    assure(sr.success, "Invalid TB");
    assure(sr.length >= sr.tbLength, "TB too small");

    return sr;

}

Frame::SearchResult
First::doChoseTB(const Frame::SearchResultSet& tbs)
{
    return *(tbs.begin()); 
}

Frame::SearchResult
BestFit::doChoseTB(const Frame::SearchResultSet& tbs)
{
    Frame::SearchResultSet::iterator it;
    Frame::SearchResult best;
    unsigned int bestFit = std::numeric_limits<unsigned int>::max();
    for(it = tbs.begin(); it != tbs.end(); it++)
    {
        unsigned int waste = it->length - it->tbLength; /*TODO*/
        if(waste < bestFit)
        {
            best = *it;
            bestFit = waste;
        }
    }
    assure(bestFit < std::numeric_limits<unsigned int>::max(), "Failed to find best TB");

    return best;
}

Frame::SearchResult
WorstFit::doChoseTB(const Frame::SearchResultSet& tbs)
{
    Frame::SearchResultSet::iterator it;
    Frame::SearchResult worst;
    unsigned int worstFit = 0;
    for(it = tbs.begin(); it != tbs.end(); it++)
    {
        unsigned int waste = it->length - it->tbLength; /*TODO*/
        if(waste >= worstFit)
        {
            worst = *it;
            worstFit = waste;
        }
    }

    return worst;
}

Frame::SearchResult
Random::doChoseTB(const Frame::SearchResultSet& tbs)
{
    Frame::SearchResultSet::iterator it;

    unsigned int r = rnd_() * tbs.size();

    it = tbs.begin();
    for(int i = 0; i < r; i++)
        it++;

    return *it;
}


