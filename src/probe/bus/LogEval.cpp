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

#include <WNS/probe/bus/LogEval.hpp>

//#include <iostream>
#include <fstream>
#include <iomanip>

using namespace wns::probe::bus;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    LogEval,
    wns::probe::bus::ProbeBus,
    "LogEval",
    wns::PyConfigViewCreator);

LogEval::LogEval(const wns::pyconfig::View& pyco):
    outputPath(pyco.get<std::string>("outputDir")),
    filename(pyco.get<std::string>("outputFilename")),
	firstWrite(true),
	timePrecision(7),
	valuePrecision(6),
	//format((pyco.get<std::string>("format")=="scientific") ? formatScientific : formatFixed)
	format(formatFixed)
{
}

LogEval::~LogEval()
{
}

bool
LogEval::accepts(const wns::simulator::Time&, const IContext&)
{
	return true;
}

void
LogEval::onMeasurement(const wns::simulator::Time& _time,
			  const double& _value,
			  const IContext&)
{
	LogEntry entry;
	entry.value = _value;
	entry.time  = _time;

    // Store value for later output
	logQueue.push_back(entry);
}

void
LogEval::output()
{
	std::ofstream out((outputPath + "/" + filename).c_str(),
					  (firstWrite ? std::ios::out : (std::ios::out |  std::ios::app)));
	if (firstWrite) firstWrite = false;
    std::string errorString = "I/O Error: Can't dump LogEval log file";
	out << (format == formatFixed ? std::setiosflags(std::ios::fixed) :
			std::setiosflags(std::ios::scientific))
		<< std::resetiosflags(std::ios::right)
		<< std::setiosflags(std::ios::left);
	assure(out, errorString);
    while (!logQueue.empty())
    {
		LogEntry entry = logQueue.front();
		logQueue.pop_front();
		out << std::setprecision(timePrecision) << entry.time
			<< " " << std::setprecision(valuePrecision) << entry.value << std::endl;
		assure(out, errorString);
    }

} // output()

