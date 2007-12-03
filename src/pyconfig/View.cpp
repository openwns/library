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

#include <WNS/pyconfig/View.hpp>
#include <WNS/Exception.hpp>
#include <WNS/Assure.hpp>

#include <sstream>
#include <iostream>
#include <csignal>

using namespace wns::pyconfig;

View::View() :
	dict(NULL),
	viewExpression()
{
	++View::getCount();
	this->initializePython();
}

View::View(const View& other, const std::string &newViewExpression) :
	dict(NULL),
	viewExpression("")
{
	++View::getCount();
	this->initializePython();

	assure(!PyErr_Occurred(), "Dirty python context!");

	PyObject* pyObject = other.getObject(newViewExpression);
	if(pyObject == NULL)
	{
		other.couldntRetrieve(newViewExpression);
		this->showdown("Couldn't resolve viewExpression.");
	}

	// not needed any longer
	Py_DECREF(pyObject);

	std::stringstream ss;
	// the following python code snippet creates the context
	// for the new view.
	// View is a subclass of dict. you can find View in the
	// pyconfig::Parser module of the wns package.
	ss << "__import__('openwns.PyConfig').PyConfig.View(" << newViewExpression << ")";

	this->dict = PyRun_String(ss.str().c_str(),
			    Py_eval_input,
			    other.dict,
			    other.dict);
	if(this->dict == NULL)
	{
		other.couldntRetrieve(newViewExpression);
		showdown("Couldn't resolve viewExpression.");
	}
	assert(this->dict != NULL);

	this->viewExpression = other.viewExpression + "::" + newViewExpression;
} // View


View::View(const View& other, const std::string &newViewExpression, int at) :
	dict(NULL),
	viewExpression("")
{
	++View::getCount();
	this->initializePython();
	assure(!PyErr_Occurred(), "Dirty python context!");

	std::stringstream subscription;
	subscription << newViewExpression << "[" << at << "]";

	PyObject* pyObject = other.getObject(subscription.str());
	if(pyObject == NULL)
	{
		other.couldntRetrieve(newViewExpression);
		showdown("Couldn't resolve viewExpression.");
	}

	// Not needed any longer
	Py_DECREF(pyObject);

	{
		std::stringstream ss;
		ss << "__import__('openwns.PyConfig').PyConfig.View("
		   << newViewExpression
		   << "[" << at << "]"
		   << ")";

		dict = PyRun_String(ss.str().c_str(),
				    Py_eval_input,
				    other.dict,
				    other.dict);
		assert(NULL != dict);
	}

	{
		std::stringstream ss;

		ss << other.viewExpression
		   << "::" << newViewExpression << "[" << at << "]";
		viewExpression = ss.str();
	}
} // View


View::View(const Sequence& seq, int at) :
	dict(NULL),
	viewExpression("")
{
	++View::getCount();
	this->initializePython();
	assure(!PyErr_Occurred(), "Dirty python context!");

	assure(!seq.isSequenceAt(at), "Trying to create pyconfig::View from nested sequence!");

	PyObject* pyDict = PyDict_New();
        assure(pyDict != NULL, "Creation of Python Dict Failed");

	int result = PyDict_SetItemString(pyDict, "dummy", seq.sequence);
        assure(result==0, "Inserting of element into Dict failed!");

	result = PyDict_SetItemString(pyDict, "__builtins__", PyImport_ImportModule("__builtin__"));
        assure(result==0, "Initializing __builtins__ failed!");

	{
		std::stringstream ss;
		ss << "__import__('openwns.PyConfig').PyConfig.View("
		   << "dummy[" << at << "]"
		   << ")";

		dict = PyRun_String(ss.str().c_str(),
				    Py_eval_input,
				    pyDict,
				    pyDict);
		if (dict == NULL)
			showdown("couldn't create View from sequence item!", true);
	}
	Py_DECREF(pyDict);

	{
		std::stringstream ss;

		ss << seq.pathName << "[" << at << "]";
		viewExpression = ss.str();
	}
}

View::~View()
{
	// can happen that dict is NULL if View() has been called but not object
	// has not been used.
	if(dict != NULL)
	{
		Py_DECREF(dict);
	}

	--View::getCount();
	this->finalizePython();
} //~View

wns::pyconfig::View
View::getView(const std::string& newViewExpression) const
{
	return View(*this, newViewExpression);
} // getView

wns::pyconfig::Sequence
View::getSequence(const std::string& sequenceExpression) const
{
	if (!this->isSequence(sequenceExpression))
	{
		Exception e;
		e << "Cannot retrieve sequence named '" << sequenceExpression
		  << "' from this view.\n\n"
		  << "Context: " << this->context() << "\n\n"
		  << "Content: " << this->asString() << "\n\n";
		throw e;
	}
	return pyconfig::Sequence(getObject(sequenceExpression),
				  this->viewExpression+"::"+sequenceExpression);
} // getSequence

wns::pyconfig::View
View::getView(const std::string& newViewExpression, int at) const
{
	return View(*this, newViewExpression, at);
} // getView

PyObject*
View::getObject(const std::string& optionExpression) const
{
	assure(!PyErr_Occurred(), "Dirty python context!");

	return PyRun_String(optionExpression.c_str(),
			    Py_eval_input,
			    dict,
			    dict);
} // getObject


PyObject*
View::getObject(const std::string& optionExpression, int at) const
{
	PyObject* seq = getObject(optionExpression);
	if(seq == NULL)
	{
		return NULL;
	}

	if(!PySequence_Check(seq))
	{
		Py_DECREF(seq);
		return NULL;
	}

	PyObject* o = PySequence_GetItem(seq, at);
	Py_DECREF(seq);

	return o;
} // getObject


bool
View::len(int& result, const std::string& optionExpression) const
{
	assure(!PyErr_Occurred(), "Dirty python context!");

	PyObject* o = getObject(optionExpression);
	if(!o) {
		PyErr_Clear();
		return false;
	}

	if(!PySequence_Check(o)) {
		Py_DECREF(o);
		return false;
	}

	result = PySequence_Length(o);
	Py_DECREF(o);
	return true;
} // len


int
View::len(const std::string& optionExpression) const
{
	assure(!PyErr_Occurred(), "Dirty python context!");

	int result;

	if(!len(result, optionExpression)) {
		couldntRetrieve(optionExpression);

		showdown("Unknown thang in View::len.");
	}

	return result;
} // len

bool
View::knows(const std::string& optionExpression) const
{
	assure(!PyErr_Occurred(), "Dirty python context!");

	PyObject* o = getObject(optionExpression);
	if(!o) {
		PyErr_Clear();
		return false;
	}

	Py_DECREF(o);
	return true;
} // knows

long
View::getId() const
{
	return get<long>("__id__");
} // getId

std::string
View::context() const
{
	return viewExpression;
} // context


void
View::patch(const std::string& expression)
{
	assure(!PyErr_Occurred(), "Dirty python context!");

	PyObject* o = PyRun_String(expression.c_str(), Py_file_input, dict, dict);
	if(!o)
		showdown("couldn't patch View.", true);

	Py_DECREF(o);
} // patch

bool
View::isNone(const std::string& expression) const
{
	PyObject* o = this->getObject(expression);
	bool isNone = (o == Py_None);
	Py_DECREF(o);
	return isNone;
} // isNone

bool
View::isSequence(const std::string& expression) const
{
	PyObject* o = this->getObject(expression);

        if(o == NULL)
        {
                this->couldntRetrieve(expression);
                this->showdown("Couldn't resolve viewExpression.");
        }

	if (PySequence_Check(o) == 1)
	{
		Py_DECREF(o);
		return true;
	}
	else
	{
		Py_DECREF(o);
		return false;
	}
}

View::View(const View& other)
{
	++View::getCount();
	this->initializePython();
	viewExpression = other.viewExpression;
	dict = other.dict;
	Py_INCREF(dict);
} // copy constructor


View&
View::operator=(const View& other)
{
	viewExpression = other.viewExpression;
	dict = other.dict;
	Py_INCREF(dict);

	return *this;
} // assignment operator


bool
View::operator==(const View& other) const
{
	return getId() == other.getId();
} // equality operator

bool
View::operator!=(const View& other) const
{
	return !(*this == other);
} // inequality operator

bool
View::operator<(const View& other) const
{
	return getId() < other.getId();
} // less than operator

bool
View::doConvert(bool& value, PyObject* o) const
{
	switch(PyObject_IsTrue(o)) {
		case 1:
			value = true;
			break;
		case 0:
			value = false;
			break;
		default:
			return false;
	}

	return true;
} // convert

bool
View::doConvert(std::string& value, PyObject* o) const
{
	PyObject* s = PyObject_Str(o);

	if(s == NULL)
		return false;

	value = std::string(PyString_AS_STRING(s));

	Py_DECREF(s);
	return true;
} // convert

void
View::couldntRetrieve(const std::string& optionExpression) const
{
	std::cerr << "PyConfig says:\n"
		  << "The configuration entry "
		  << "'" << optionExpression << "' "
		  << "could not be retrieved from:\n    "
		  << context() << "\n"
		  << "Following configuration entries are available:\n";
	{
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		PyRun_String(
			"__import__('sys').stderr.write( __visible__ + '\\n')\n",
			Py_file_input,
			dict,
			dict);

		PyErr_Restore(ptype, pvalue, ptraceback);
	}
} // couldntRetrieve


void
View::showdown(const std::string& reason, bool raise) const
{
	std::cerr << "PyConfig says:\n";
	PyErr_Print();
	std::cerr << "\n";

	PyErr_Clear();

	if(raise)
		throw Exception(reason);
} // showdown

std::string
View::asString() const
{
	return this->get<std::string>("__stringRepresentation__");
} // asString

void
View::initializePython()
{
	if(View::getCount() == 1)
	{
		assure(Py_IsInitialized() == false, "called initializePython() although interpreter is already initialized!");
		void (*handler)(int);
		handler = std::signal(2, SIG_DFL);
		Py_Initialize();
		std::signal(2, handler);
	}
}

void
View::finalizePython()
{
	if(View::getCount() == 0)
	{
		assure(Py_IsInitialized() == true, "called finalizePython although interpreter has not been initialized!");
		void (*handler)(int);
		handler = std::signal(2, SIG_DFL);
		Py_Finalize();
		std::signal(2, handler);
	}
}

int&
View::getCount()
{
	static int count = 0;
	assure(count >=0, "Internal reference counting is broken!");
	return count;
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
