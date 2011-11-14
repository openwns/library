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


STATIC_FACTORY_REGISTER_WITH_CREATOR(First, ITBChoser, "First", wns::PyConfigViewCreator);
STATIC_FACTORY_REGISTER_WITH_CREATOR(BestFit, ITBChoser, "BestFit", wns::PyConfigViewCreator);
STATIC_FACTORY_REGISTER_WITH_CREATOR(WorstFit, ITBChoser, "WorstFit", wns::PyConfigViewCreator);
STATIC_FACTORY_REGISTER_WITH_CREATOR(Random, ITBChoser, "Random", wns::PyConfigViewCreator);
STATIC_FACTORY_REGISTER_WITH_CREATOR(Smallest, ITBChoser, "Smallest", wns::PyConfigViewCreator);
STATIC_FACTORY_REGISTER_WITH_CREATOR(Smallest, ITBChoser, "Previous", wns::PyConfigViewCreator);

Frame::SearchResult
TBChoser::choseTB(const Frame::SearchResultSet& tbs)
{
    assure(!tbs.empty(), "Cannot chose from empty set");

    Frame::SearchResult sr = doChoseTB(tbs);

    assure(sr.success, "Invalid TB");
    assure(sr.length >= sr.tbLength, "TB too small");

    return sr;

}

First::First(const wns::pyconfig::View& config)
{
}

Frame::SearchResult
First::doChoseTB(const Frame::SearchResultSet& tbs)
{
    return *(tbs.begin()); 
}

BestFit::BestFit(const wns::pyconfig::View& config)
{
}

Frame::SearchResult
BestFit::doChoseTB(const Frame::SearchResultSet& tbs)
{
    Frame::SearchResultSet::iterator it;
    Frame::SearchResult best;
    unsigned int bestFit = std::numeric_limits<unsigned int>::max();
    for(it = tbs.begin(); it != tbs.end(); it++)
    {
        unsigned int waste = it->length - it->tbLength;
        if(waste < bestFit)
        {
            best = *it;
            bestFit = waste;
        }
    }
    assure(bestFit < std::numeric_limits<unsigned int>::max(), "Failed to find best TB");

    return best;
}

WorstFit::WorstFit(const wns::pyconfig::View& config)
{
}

Frame::SearchResult
WorstFit::doChoseTB(const Frame::SearchResultSet& tbs)
{
    Frame::SearchResultSet::iterator it;
    Frame::SearchResult worst;
    unsigned int worstFit = 0;
    for(it = tbs.begin(); it != tbs.end(); it++)
    {
        unsigned int waste = it->length - it->tbLength; 
        if(waste >= worstFit)
        {
            worst = *it;
            worstFit = waste;
        }
    }

    return worst;
}

Random::Random(const wns::pyconfig::View& config)
{
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

Smallest::Smallest(const wns::pyconfig::View& config)
{
}

Frame::SearchResult
Smallest::doChoseTB(const Frame::SearchResultSet& tbs)
{
    Frame::SearchResultSet::iterator it;
    Frame::SearchResult result;
    unsigned int smallest = std::numeric_limits<unsigned int>::max();;
    for(it = tbs.begin(); it != tbs.end(); it++)
    {
        if(it->tbLength < smallest)
        {
            result = *it;
            smallest = it->tbLength;
        }
    }
    assure(smallest < std::numeric_limits<unsigned int>::max(), "Failed to find smallest TB");

    return result;    
}

Previous::Previous(const wns::pyconfig::View& config)
{
    std::string tbChoserName = config.get<std::string>("fallbackChoser.__plugin__");
    fallbackChoser_ = ITBChoser::Factory::creator(
        tbChoserName)->create(config.get("fallbackChoser"));
}

Previous::~Previous()
{
    delete fallbackChoser_;
}

Frame::SearchResult
Previous::doChoseTB(const Frame::SearchResultSet& tbs)
{
    Frame::SearchResult result;
    if(history_.find(tbs.begin()->cid) == history_.end())
    {
        result = fallbackChoser_->choseTB(tbs);
        assure(result.cid == tbs.begin()->cid, "CIDs do not match"); 

        history_[tbs.begin()->cid] = std::set<unsigned int>();
    }
    else
    {
        std::set<unsigned int> previous = history_.find(tbs.begin()->cid)->second;
        assure(!previous.empty(), "List of past RBs is empty.");

        unsigned int bestMatch = 0;
        Frame::SearchResultSet theBest;
        Frame::SearchResultSet::iterator it;
        for(it = tbs.begin(); it != tbs.end(); it++)
        {
            int match = 0;
            for(int i = 0; i < it->tbLength; i++)
            {
                if(previous.find(i + it->tbStart) != previous.end())
                {
                    match++;
                }
            }
            if(match > bestMatch)
            {
                bestMatch = match;
                theBest.clear();
                theBest.insert(*it);
            }
            else if(match > 0 && match == bestMatch)
            {
                theBest.insert(*it);
            }
        }
        if(bestMatch > 0)
        {
            assure(!theBest.empty(), "No best TB candidate set.");
            history_[tbs.begin()->cid].clear();
            result = fallbackChoser_->choseTB(theBest);
        }
        else
        {
            result = fallbackChoser_->choseTB(tbs);
        }
    }
    for(int i = 0; i < result.tbLength; i++)
    {
        history_[tbs.begin()->cid].insert(i + result.tbStart);
    }
    return result;
}

