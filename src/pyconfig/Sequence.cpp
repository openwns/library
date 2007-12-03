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

#include <WNS/pyconfig/Sequence.hpp>
#include <sstream>

using namespace wns::pyconfig;

Sequence::Sequence(PyObject* _sequence) :
	sequence(_sequence),
	pathName("")

{
	assure(PySequence_Check(_sequence) == 1, "Object is no sequence!");
	Py_INCREF(sequence);
} // Sequence

Sequence::Sequence(PyObject* _sequence,
		   const std::string& _pathName) :
	sequence(_sequence),
	pathName(_pathName)

{
	assure(PySequence_Check(_sequence) == 1, "Object is no sequence!");
	Py_INCREF(sequence);
} // Sequence

Sequence::Sequence(const Sequence& other) :
	sequence(other.sequence),
	pathName(other.pathName)
{
	Py_INCREF(sequence);
} // copy constructor


Sequence::~Sequence()
{
	Py_DECREF(sequence);
} // ~Sequence


Sequence&
Sequence::operator=(const Sequence& other)
{
	sequence = other.sequence;
	Py_INCREF(sequence);

	return *this;
} // =


bool
Sequence::empty() const
{
	return !PySequence_Length(sequence);
} // empty


int
Sequence::size() const
{
	return PySequence_Length(sequence);
} // size


bool
Sequence::isSequenceAt(int n) const
{
	PyObject* obj = PySequence_GetItem(sequence, n);
	assure(obj, "This is the end, my friend.");
	if (PySequence_Check(obj)==1)
	{
		Py_DECREF(obj);
		return true;
	}
	else
	{
		Py_DECREF(obj);
		return false;
	}
}

Sequence
Sequence::getSequenceAt(int n) const
{
	PyObject* obj = PySequence_GetItem(sequence, n);
	assure(isSequenceAt(n), "No sequence at pos:"<<n);

	if (PySequence_Check(obj)!=1)
	{
		Py_DECREF(obj);
		Exception e;
		e << "Sequence: Item at pos:'" << n
		  << "' is no nested sequence.\n\n";
		throw e;
	}

	std::stringstream ss;
	ss << pathName << "[" << n << "]";
	Sequence s(obj, ss.str());
	Py_DECREF(obj);
	return s;
} // getSequenceAt


//
// IterPolicy
//

Sequence::IterPolicy::IterPolicy() :
		iter(),
		nextObject()
{
} // end() constructor


Sequence::IterPolicy::IterPolicy(PyObject* sequence) :
		iter(),
		nextObject()
{
	iter = PyObject_GetIter(sequence);
	assure(iter, "Sequence::iterator called without sequence.");

	next();
} // valid constructor


Sequence::IterPolicy::IterPolicy(const IterPolicy& other) :
		iter(other.iter),
		nextObject(other.nextObject)
{
	Py_XINCREF(iter);
	Py_XINCREF(nextObject);
} // copy constructor


Sequence::IterPolicy::~IterPolicy()
{
	Py_XDECREF(iter);
	Py_XDECREF(nextObject);
} // ~IterPolicy


Sequence::IterPolicy&
Sequence::IterPolicy::operator=(const IterPolicy& other)
{
	iter = other.iter;
	nextObject = other.nextObject;

	Py_XINCREF(iter);
	Py_XINCREF(nextObject);

	return *this;
} // =


void
Sequence::IterPolicy::next()
{
	Py_XDECREF(nextObject);
	nextObject = PyIter_Next(iter);
} // next


PyObject*
Sequence::IterPolicy::obj() const
{
	return nextObject;
} // obj


Sequence
Sequence::fromString(const std::string& s)
{
	assure(!PyErr_Occurred(), "Dirty python context!");

	PyObject* pyDict = PyDict_New();

#ifndef NDEBUG
	int result =
#endif
		PyDict_SetItemString(pyDict, "__builtins__", PyImport_ImportModule("__builtin__"));
        assure(result==0, "Initializing __builtins__ failed!");

	PyObject* o = PyRun_String(s.c_str(),
				   Py_eval_input,
				   pyDict,
				   pyDict);

	Py_DECREF(pyDict);

	if (o == NULL)
	{
		Exception e;
		e << "Erroneous Python Expression: \n\n" << s;
		throw e;
	}

	assure(PySequence_Check(o)==1, "Python expression '" << s << "' is not a sequence!");

	Sequence seq(o, "<string>");
	Py_DECREF(o);
	return seq;
}

/*
  Local Variables:
  mode: c++
  fill-column: 80
  c-basic-offset: 8
  c-comment-only-line-offset: 0
  c-tab-always-indent: t
  indent-tabs-mode: t
  tab-width: 8
  End:
*/
