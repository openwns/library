/******************************************************************************
 * WNS (Wireless Network Simulator)                                           *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2004-2006                                                    *
 * Chair of Communication Networks (ComNets)                                  *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
 ******************************************************************************/

#ifndef WNS_PROBE_BUS_PYSELECTIVEPROBEBUS_HPP
#define WNS_PROBE_BUS_PYSELECTIVEPROBEBUS_HPP

#include <WNS/probe/bus/ProbeBus.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/pyconfig/Object.hpp>

namespace wns { namespace probe { namespace bus {

    /**
     * @brief Filter Measurements by calling back a PythonObject
     *
     * Use this ProbeBus for rapid deployment if individual filter rules.
     * The wns::pyconfig::View passed to the constructor must contain a callable named
     * accepts. This callable must accept two parameters. The first is the ProbeID
     * which is mapped to a Python string and the second is a Dict of Python strings
     * to Python ints. Based on this information the Python method must return wether
     * the PythonProbeBus accepts the measurement or not.
     *
     * PythonProbeBus does not do anything in its onMeasurement method. It
     * is simply a gate that determines wether measurements are forwarded to its Observers
     * or not.
     *
     * If you only want to evaluate measurements for certain IDs plug the
     * PythonProbeBus between the MasterProbeBus and a StatEvalProbeBus and provide
     * the filtering criteria from within your configuration file.
     *
     * @author Daniel Bültmann <dbn@comnets.rwth-aachen.de>
     */
    class PythonProbeBus:
        public wns::probe::bus::ProbeBus
    {
    public:
        PythonProbeBus(const wns::pyconfig::View& pyco);

        virtual ~PythonProbeBus();

        virtual void
        onMeasurement(const wns::simulator::Time&,
                      const double&,
                      const IContext&);

        virtual bool
        accepts(const wns::simulator::Time&, const IContext&);

        virtual void
        output();

    private:

        void
        showdown(const std::string& reason) const;

        wns::pyconfig::View pyco;

        wns::pyconfig::Object pyAcceptsMethod;

        wns::pyconfig::Object pyOnMeasurementMethod;

        wns::pyconfig::Object pyOutputMethod;

        bool reportErrors;
    };
} // bus
} // probe
} // wns

#endif //WNS_PROBE_BUS_PYSELECTIVEPROBEBUS_HPP
