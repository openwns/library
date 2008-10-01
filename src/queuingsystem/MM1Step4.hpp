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

/**
 * @page wns.queuingsystem.mm1step4 Obtaining Results
 * @ref group_wns_queuingsystem_tutorial
 * @section embeddingEvaluation How to embed evaluation in your configuration
 *
 *
 * @code
 * class StatisticsProbeBus(openwns.probebus.ProbeBus):
 *
 *   nameInFactory = "PythonProbeBus"
 *
 *   def __init__(self, outputFilename):
 *       openwns.probebus.ProbeBus.__init__(self)
 *       self.reportErrors = True
 *       self.outputFilename = outputFilename
 *       self.sum = 0.0
 *       self.trials = 0
 *
 *   def accepts(self, time, context):
 *       return True
 *
 *   def onMeasurement(self, time, measurement, context):
 *       self.sum += measurement
 *       self.trials += 1
 *
 *   def output(self):
 *       f = open(self.outputFilename, "w")
 *       f.write("Number of trials: %s\n" % str(self.trials))
 *       f.write("Mean value : %s\n" % str(self.sum/self.trials))
 *       f.close()
 * @endcode
 */

