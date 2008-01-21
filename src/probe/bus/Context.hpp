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

#ifndef WNS_PROBE_BUS_CONTEXT_HPP
#define WNS_PROBE_BUS_CONTEXT_HPP


#include <WNS/Assure.hpp>
#include <WNS/NonCopyable.hpp>
#include <WNS/IOutputStreamable.hpp>
#include <WNS/pyconfig/Object.hpp>

#include <iostream>

namespace wns { namespace probe { namespace bus {

    namespace context {
        /**
		 * @brief Exception thrown by the Context if no value exists for a
		 * given key
		 */
        class NotFound :
            public wns::Exception
        {};

        /**
		 * @brief Exception thrown by the Context if a value already exists
		 * for a given key
		 */
        class DuplicateKey :
            public wns::Exception
        {};

        /**
		 * @brief Exception thrown by the Context if a value for key exists but
		 * is of different type than the one requested
		 */
        class TypeError :
            public wns::Exception
        {};
    }

    /**
	 * @brief Interface for the Context Container
	 *
	 * @todo pab: Add setter/getter methods for other PODs
	 * (i.e. double and bool)
	 */
    class IContext :
        public wns::IOutputStreamable
    {
    public:
        virtual ~IContext(){}

        /**
		 * @brief returns whether an element with the given key exists in the
		 * Context
		 */
        virtual bool
        knows(const std::string& key) const = 0;

        /**
		 * @brief inserts an integer value under the given key (thereby
		 * checking that no duplicates exist)
		 */
        virtual void
        insertInt(const std::string& key, int value) = 0;

        /**
		 * @brief inserts a string value under the given key (thereby checking
		 * that no duplicates exist)
		 */
        virtual void
        insertString(const std::string& key, const std::string& value) = 0;

        /**
		 * @brief get integer value stored under 'key'
		 */
        virtual int
        getInt(const std::string& key) const = 0;

        /**
		 * @brief get string value stored under 'key'
		 */
        virtual std::string
        getString(const std::string& key) const = 0;
    };

    class Context :
        virtual public IContext,
        private NonCopyable
    {
        friend class PythonProbeBus;
    public:
        Context();

        ~Context();

        virtual bool
        knows(const std::string& key) const;

        virtual void
        insertInt(const std::string& key, int value);

        virtual void
        insertString(const std::string& key, const std::string& value);

        virtual int
        getInt(const std::string& key) const;

        virtual std::string
        getString(const std::string& key) const;

    private:
        virtual std::string
        doToString() const;

        wns::pyconfig::Object pyDict_;
    };

} // bus
} // probe
} // wns

#endif // WNS_PROBE_BUS_CONTEXT_HPP
