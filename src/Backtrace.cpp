
/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 16, D-52074 Aachen, Germany
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

#include <WNS/Backtrace.hpp>
#include <WNS/demangle.hpp>
#include <iomanip>
#include <cmath>

#ifdef _GNU_SOURCE
#include <execinfo.h>
#endif // _GNU_SOURCE

#include <sstream>

using namespace wns;

Backtrace::Backtrace() :
    backtrace_()
{
}

Backtrace::~Backtrace()
{
    clear();
}

void
Backtrace::snapshot()
{
    clear();
#ifdef _GNU_SOURCE
    const int arraySize = 1000;
    void* array[arraySize];
    size_t size;
    char** strings;

    size = backtrace (array, arraySize);
    strings = backtrace_symbols (array, size);

    for (size_t ii = 0; ii < size; ++ii)
    {
        backtrace_.push_back(strings[ii]);
    }

    free (strings);
#endif // _GNU_SOURCE
}

void
Backtrace::clear()
{
    backtrace_.clear();
}

wns::Backtrace::FunctionCalls
Backtrace::getFunctionCalls() const
{
    FunctionCalls functionCalls;

    for(Lines::const_iterator itr = backtrace_.begin();
        itr != backtrace_.end();
        ++itr)
    {
        functionCalls.push_back(FunctionCall(*itr));
    }

    return functionCalls;
}

std::string
Backtrace::doToString() const
{
    // get function calls and stream reverse
    FunctionCalls functionCalls = getFunctionCalls();
    size_t stackSize = functionCalls.size();
    size_t frame = stackSize;
    int width = static_cast<int>(std::ceil(std::log10(stackSize)));
    std::stringstream tmp;
    tmp << "Backtrace (most recent call last, stack size: " << stackSize << "):\n";
    for(FunctionCalls::const_reverse_iterator itr = functionCalls.rbegin();
        itr != functionCalls.rend();
        ++itr)
    {
        tmp << " " << std::setw(width) << frame << ")  " << itr->getName() << "\n";
        --frame;
    }
    return tmp.str();
}



Backtrace::FunctionCall::FunctionCall(std::string line) :
    origin_("unkown"),
    name_("unkown"),
    offset_("unkown"),
    returnAddress_("unkown")
{

    // the line typically looks like this:
    // ./openwns(_ZN3wns3WNS13SignalHandler11catchSignalEi+0xfc) [0x8068870]
    size_t openBrace = line.find("(");
    size_t plus = line.find("+");
    size_t closeBrace = line.find(")");
    size_t openSquareBrace = line.find("[");
    size_t closeSquareBrace = line.find("]");

    if (openBrace != std::string::npos)
    {
        // the stuff in front of "(" is the origin
        origin_ = line.substr(0, openBrace);

        if (closeBrace != std::string::npos && plus != std::string::npos)
        {
            // the stuff from "(" to "+" is the function name
            {
                std::string symbol = line.substr(openBrace+1, plus-openBrace-1);
                name_ = wns::demangle(symbol);
            }
            // the stuff from "+" to ")" is the offset
            offset_ = line.substr(plus+1, closeBrace-plus-1);
        }
    }

    if (openSquareBrace != std::string::npos && closeSquareBrace != std::string::npos)
    {
        // between "[" and "]" we find the return address
        returnAddress_ = line.substr(openSquareBrace+1, closeSquareBrace-plus-1);
    }
}

std::string
Backtrace::FunctionCall::getOrigin() const
{
    return origin_;
}

std::string
Backtrace::FunctionCall::getName() const
{
    return name_;
}

std::string
Backtrace::FunctionCall::getOffset() const
{
    return offset_;
}

std::string
Backtrace::FunctionCall::getReturnAddress() const
{
    return returnAddress_;
}
