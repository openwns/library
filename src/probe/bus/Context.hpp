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
		 * @brief Exception thrown by the Context if a value already exists for a
		 * given key
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
	 * @todo pab: Add setter/getter methods for other PODs (i.e. double and bool)
	 */
	class IContext :
		public wns::IOutputStreamable
	{
	public:
		virtual ~IContext(){}
		/**
		 * @brief returns whether an element with the given key exists in the Context
		 */
        virtual bool
        knows(const std::string& key) const = 0;

		/**
		 * @brief inserts an integer value under the given key (thereby checking
		 * that no duplicates exist)
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

        wns::pyconfig::Object pyDict;
    };

} // bus
} // probe
} // wns

#endif // WNS_PROBE_IDREGISTRY_HPP
