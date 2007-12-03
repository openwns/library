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

#ifndef WNS_PYCONFIG_CONVERTER_HPP
#define WNS_PYCONFIG_CONVERTER_HPP

#include <python/Python.h>
#include <string>

namespace wns { namespace pyconfig {
	template <typename T>
	class Converter
	{
	public:
		typedef T TargetType;

		// boolean converter
		bool
		convert(bool& value, PyObject* o) const
		{
			switch(PyObject_IsTrue(o)) {
					case 1:
						value = true;
						break;
					case 0:
						value = false;
						break;
			}
			return false;
		} // convert bool

		// std::string converter
		bool
		convert(std::string& value, PyObject* o) const
		{
			PyObject* s = PyObject_Str(o);

			if(s == NULL)
				return false;

			value = std::string(PyString_AS_STRING(s));

			Py_DECREF(s);
			return true;
		} // convert string


		// default converter
		template <typename U>
		bool convert(U &value, PyObject *o) const
		{
			PyObject* s = PyObject_Str(o);

			if(s == NULL)
				return false;

			std::istringstream os(PyString_AS_STRING(s));
			os >> value;

			Py_DECREF(s);
			return true;
		} // convert
	};

}}

#endif // NOT defined WNS_PYCONFIG_CONVERTER_HPP

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
