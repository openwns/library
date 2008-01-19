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

#include <Python.h>
#include <WNS/probe/bus/PythonProbeBus.hpp>

#include <sstream>

using namespace wns::probe::bus;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    PythonProbeBus,
    wns::probe::bus::ProbeBus,
    "PythonProbeBus",
    wns::PyConfigViewCreator);

PythonProbeBus::PythonProbeBus(const wns::pyconfig::View& _pyco):
    pyco(_pyco),
    reportErrors(pyco.get<bool>("reportErrors"))
{
    assure(pyco.knows("accepts"), "Cannot retrieve method accept from PyConfig");
    assure(pyco.knows("onMeasurement"), "Cannot retrieve method on Measurement from PyConfig");
    pyAcceptsMethod = pyco.getObject("accepts");
    pyOnMeasurementMethod = pyco.getObject("onMeasurement");
    pyOutputMethod = pyco.getObject("output");
}

PythonProbeBus::~PythonProbeBus()
{
}

void
PythonProbeBus::onMeasurement(const wns::simulator::Time& timestamp,
                              const double& value,
                              const IContext& reg)
{
    PyObject* pyTimestamp = Py_BuildValue("d", timestamp);
    PyObject* pyValue = Py_BuildValue("d", value);

    assure(pyValue != NULL, "Could not create Python Object");

	const Context* c = dynamic_cast<const Context*>(&reg);
	assure(c != NULL, "PythonProbeBus can only collaborate with 'Context' implementation of IContext");

    PyObject* result = PyObject_CallFunctionObjArgs(pyOnMeasurementMethod.obj_,
                                                    pyTimestamp,
                                                    pyValue,
                                                    c->pyDict.obj_,
                                                    NULL);
    if (result == NULL)
        this->showdown("Error when calling Python accepts method.");

    Py_DECREF(result);
    Py_DECREF(pyTimestamp);
    Py_DECREF(pyValue);

    // Post Condition: Python must be clean before we leave
    if (PyErr_Occurred())
    {
        this->showdown("Error Python Context ist dirty");
    }
}

bool
PythonProbeBus::accepts(const wns::simulator::Time& timestamp,
                        const IContext& reg)
{
    PyObject* pyTimestamp = Py_BuildValue("d", timestamp);

	const Context* c = dynamic_cast<const Context*>(&reg);
	assure(c != NULL, "PythonProbeBus can only collaborate with 'Context' implementation of IContext");

    PyObject* result = PyObject_CallFunctionObjArgs(pyAcceptsMethod.obj_,
                                                    pyTimestamp,
                                                    c->pyDict.obj_,
                                                    NULL);
    if (result == NULL)
    {
        this->showdown("Error when calling Python accepts method.");
    }

    bool returnValue = (result==Py_True);

    Py_DECREF(result);
    Py_DECREF(pyTimestamp);

    // Post Condition: Python must be clean before we leave
    if (PyErr_Occurred())
    {
        this->showdown("Error Python Context ist dirty");
    }

    return returnValue;
}

void
PythonProbeBus::output()
{
    PyObject* result = PyObject_CallFunctionObjArgs(pyOutputMethod.obj_,
                                                    NULL);
    if (result == NULL)
        this->showdown("Error when calling Python output method.");

    Py_DECREF(result);

    // Post Condition: Python must be clean before we leave
    if (PyErr_Occurred())
    {
        this->showdown("Error Python Context ist dirty");
    }
}

void
PythonProbeBus::showdown(const std::string& reason) const
{
    if (reportErrors)
    {
        std::cerr << "\n" << reason << "\n";
        std::cerr << "\nPython says:\n";
        PyErr_Print();
        std::cerr << "\n";

    }

    PyErr_Clear();

    throw Exception(reason);
} // showdown
