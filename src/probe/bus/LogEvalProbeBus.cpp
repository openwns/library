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

#include <WNS/probe/bus/LogEvalProbeBus.hpp>
#include <WNS/simulator/ISimulator.hpp>
#include <fstream>
#include <iomanip>

using namespace wns::probe::bus;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    LogEvalProbeBus,
    wns::probe::bus::ProbeBus,
    "LogEvalProbeBus",
    wns::PyConfigViewCreator);

LogEvalProbeBus::LogEvalProbeBus(const wns::pyconfig::View& pyco):
    filename(pyco.get<std::string>("outputFilename")),
    firstWrite(true),
    timePrecision(pyco.get<int>("timePrecision")),
    valuePrecision(pyco.get<int>("valuePrecision")),
    format(formatFixed),
    //format((config.get<std::string>("format")=="scientific") ? LogEvalProbeBus::scientific : LogEvalProbeBus::fixed),
	name(pyco.get<std::string>("name")),
	desc(pyco.get<std::string>("description")),
	prefix("#")
{
    wns::pyconfig::View pycoRoot = wns::simulator::getConfiguration();
    outputPath = pycoRoot.get<std::string>("outputDir");
}

LogEvalProbeBus::~LogEvalProbeBus()
{
}

const std::string&
LogEvalProbeBus::getName() const
{
    return name;
}

const std::string&
LogEvalProbeBus::getDesc() const
{
    return desc;
}

bool
LogEvalProbeBus::accepts(const wns::simulator::Time&, const IContext&)
{
    return true;
}

void
LogEvalProbeBus::onMeasurement(const wns::simulator::Time& _time,
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
LogEvalProbeBus::output()
{
    std::ofstream out((outputPath + "/" + filename).c_str(),
                      (firstWrite ? std::ios::out : (std::ios::out |  std::ios::app)));

    if (firstWrite) {
		firstWrite = false;
		out<<prefix<<"  PROBE RESULTS (THIS IS A MAGIC LINE)" << std::endl;
		out<<prefix<<" ---------------------------------------------------------------------------" << std::endl;
		out<<prefix<<" Evaluation: LogEval" << std::endl;
		out<<prefix<<" ---------------------------------------------------------------------------" << std::endl;
		out<<prefix<<"  Name: " << getName() << std::endl;
		out<<prefix<<"  Description: " << getDesc() << std::endl;
		out<<prefix<<" ---------------------------------------------------------------------------" << std::endl;
		// the next items cannot be known here in the header,
		// but the fields are required in WNSUnit.py
		// Maybe we can overwrite the 10 digits later (with file seek)?
		out<<prefix<<"  Trials:   0000000000" << std::endl;
		out<<prefix<<"  Mean:     0000000000" << std::endl;
		out<<prefix<<"  Variance: 0000000000" << std::endl;
		out<<prefix<<"  Minimum:  0000000000" << std::endl;
		out<<prefix<<"  Maximum:  0000000000" << std::endl;
//		out<<prefix<<"" << std::endl;
		out<<prefix<<" ---------------------------------------------------------------------------" << std::endl;
	}

    out << (format == formatFixed ? std::setiosflags(std::ios::fixed) : std::setiosflags(std::ios::scientific))
        << std::resetiosflags(std::ios::right)
        << std::setiosflags(std::ios::left);

    assure(out, "I/O Error: Can't dump LogEvalProbeBus log file");
    while (!logQueue.empty())
        {
            LogEntry entry = logQueue.front();
            logQueue.pop_front();
            out << std::setprecision(timePrecision) << entry.time
                << " " << std::setprecision(valuePrecision) << entry.value << std::endl;
            assure(out, "I/O Error: Can't dump LogEvalProbeBus log file");
        }
}

