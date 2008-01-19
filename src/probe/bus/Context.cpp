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

#include <WNS/probe/bus/Context.hpp>
#include <WNS/Assure.hpp>


using namespace wns::probe::bus;


Context::Context():
    pyDict(NULL)
{
    assure(Py_IsInitialized(), "Python interpreter is not initialized!");
    pyDict = wns::pyconfig::Object(PyDict_New());
    assure(!pyDict.isNull(), "Creation of Python Dict Failed");
}

Context::~Context()
{
    assure(Py_IsInitialized(), "Python interpreter is not initialized!");
    assure(!pyDict.isNull(), "No Python Object!");
    PyDict_Clear(pyDict.obj_);
    pyDict.decref();
}

void
Context::insertInt(const std::string& key, int value)
{
    assure(Py_IsInitialized(), "Python interpreter is not initialized!");
    assure(!pyDict.isNull(), "Creation of Python Dict Failed");

	// Check if entry with "key" already exists
	if ( this->knows(key) == true )
	{
		throw context::DuplicateKey();
	}

    PyObject* pyValue = PyInt_FromLong(value);

#ifndef NDEBUG
    int result =
#endif
		PyDict_SetItemString(pyDict.obj_, key.c_str(), pyValue);

    assure(result==0, "Inserting of element into Dict failed!");
    Py_DECREF(pyValue);
}

void
Context::insertString(const std::string& key, const std::string& value)
{
    assure(Py_IsInitialized(), "Python interpreter is not initialized!");
    assure(!pyDict.isNull(), "Creation of Python Dict Failed");

	// Check if entry with "key" already exists
	if ( this->knows(key) == true )
	{
		throw context::DuplicateKey();
	}

    PyObject* pyValue = PyString_FromString(value.c_str());

#ifndef NDEBUG
    int result =
#endif
		PyDict_SetItemString(pyDict.obj_, key.c_str(), pyValue);

    assure(result==0, "Inserting of element into Dict failed!");
    Py_DECREF(pyValue);
}

bool
Context::knows(const std::string& key) const
{
    assure(Py_IsInitialized(), "Python interpreter is not initialized!");
    assure(!pyDict.isNull(), "Creation of Python Dict Failed");

    PyObject* pyKey = PyString_FromString(key.c_str());
    int result = PyDict_Contains(pyDict.obj_, pyKey);

    assure(result!=-1, "Error in PyDict_Contains occurred");

    Py_DECREF(pyKey);

    return result==1;
}

int
Context::getInt(const std::string& key) const
{
    assure(Py_IsInitialized(), "Python interpreter is not initialized!");
    assure(!pyDict.isNull(), "Creation of Python Dict Failed");

    PyObject* pyKey = PyString_FromString(key.c_str());

    // PyDict_GetItem returns a "borrowed" reference, which means we are not
    // responsible of DECREFING it.
    PyObject* pyValue = PyDict_GetItem(pyDict.obj_, pyKey);

    if (pyValue == NULL)
        throw context::NotFound();

	if (PyInt_Check(pyValue) != true)
	{
		context::TypeError up;
		up << "Type mismatch. Object with key=" << key << " is not of type int";
		throw up;
	}

    int32_t value = PyInt_AsLong(pyValue);

    Py_DECREF(pyKey);

    return value;
}

std::string
Context::getString(const std::string& key) const
{
    assure(Py_IsInitialized(), "Python interpreter is not initialized!");
    assure(!pyDict.isNull(), "Creation of Python Dict Failed");

    PyObject* pyKey = PyString_FromString(key.c_str());

    // PyDict_GetItem returns a "borrowed" reference, which means we are not
    // responsible of DECREFING it.
    PyObject* pyValue = PyDict_GetItem(pyDict.obj_, pyKey);

    if (pyValue == NULL)
        throw context::NotFound();

	if (PyString_Check(pyValue) != true)
	{
		context::TypeError up;
		up << "Type mismatch. Object with key=" << key << " is not of type string";
		throw up;
	}

    std::string value(PyString_AsString(pyValue));

    Py_DECREF(pyKey);

    return value;
}

std::string
Context::doToString() const
{
	assure(!pyDict.isNull(), "NULL Python Object!");

	PyObject *key, *value;
	int pos = 0;

	std::stringstream str;
	str << "{";
	while (PyDict_Next(pyDict.obj_, &pos, &key, &value))
	{
		str << PyString_AsString(key) << " : ";
		if (PyInt_Check(value) == true)
		{
			str << PyInt_AsLong(value);
		}
		else if (PyString_Check(value) == true)
		{
			str << "'" << PyString_AsString(value) << "'";
		}
		else
		{
			str << "Unknown Object Type";
		}
		str << ",";
	}
	str << "}";
	return str.str();
}
