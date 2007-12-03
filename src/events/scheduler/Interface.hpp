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

#ifndef WNS_EVENTS_SCHEDULER_INTERFACE_HPP
#define WNS_EVENTS_SCHEDULER_INTERFACE_HPP

#include <WNS/events/scheduler/ICommand.hpp>
#include <WNS/events/scheduler/IEvent.hpp>
#include <WNS/events/scheduler/Callable.hpp>
#include <WNS/events/scheduler/INotification.hpp>
#include <WNS/simulator/Time.hpp>
#include <WNS/StaticFactory.hpp>
#include <WNS/NonCopyable.hpp>
#include <WNS/SubjectInterface.hpp>

namespace wns { namespace events { namespace scheduler {

    /**
     * @brief Schedules and excutes Events in simulation
     *
     * @author Marc Schinnenburg <marc@schinnenburg.com>
     * @author Karsten Klagges <kks@comnets.rwth-aachen.de>
     * @author Daniel Bültmann <dbn@comnets.rwth-aachen.de>
     *
     * The event scheduler is the "clock" of a simulation. If somethings needs to be
     * done in the "future" you have to create an Event and tell the event scheduler
     * when to execute the Event (scheduler->scheduleAt(event, 22.3)).
     *
     * The methods of the event scheduler have been logically grouped. The most
     * important groups are: Schedule Events and Delete Events.
     *
     * This is an NVI (non-virtual interface) for the implementation of
     * different event scheduler.
     */
    class Interface :
        virtual public SubjectInterface<IObserver>,
        private NonCopyable
    {
    public:
        /**
         * @name Current time
         */
        //@{
        /**
         * @brief Return current global simulation time
         *
         * @todo Marc Schinnenburg <marc@schinnenburg.com>: will probably move
         * into simulator ...
         */
        wns::simulator::Time
        getTime() const;
        //@}

        /**
         * @name Schedule Events
         * @brief Schedule Events for later execution
         */
        //@{

        /**
         * @brief Schedule a callable object for immediate execution.
         *
         * @note T must have implemented the call operator (
         * operator()() ).
         */
        IEventPtr
        scheduleNow(const Callable& callable)
        {
            this->sendScheduleNowNotification();
            return this->doScheduleNow(callable);
        }

        /**
         * @brief Schedule a callable object with a certain delay.
         *

         * @note T must have implemented the call operator (
         * operator()() ).
         */
        IEventPtr
        scheduleDelay(const Callable& callable, wns::simulator::Time delay)
        {
            this->sendScheduleDelayNotification();
            return this->doSchedule(callable, this->getTime() + delay);
        }

        /**
         * @brief Schedule a callable object for a given time.
         *
         * @note T must have implemented the call operator (
         * operator()() ).
         */
        IEventPtr
        schedule(const Callable& callable, wns::simulator::Time at)
        {
            this->sendScheduleNotification();
            return this->doSchedule(callable, at);
        }

         /**
         * @brief Queue a command to be executed as very next event
         */
        ICommandPtr
        queueCommand(const Callable& callable)
        {
            return this->doQueueCommand(callable);
        }

        /**
         * @brief Remove a command from the Scheduler
         */
        void
        dequeueCommand(const ICommandPtr& command);
        //@}

        /**
         * @name Delete Events
         * @brief Remove already scheduled Events (e.g. stop a Timer)
         */
        //@{

        /**
         * @brief Cancel the given event.
         */
        void
        cancelEvent(const IEventPtr& event);
        //@}

        /**
         * @name For testing purpose
         * @brief Used in unit tests
         */
        //@{
        /**
         * @brief Reset the scheduler to its initial state (for testing)
         */
        void
        reset();

        /**
         * @brief Process exactly one event
         */
        bool
        processOneEvent();
        //@}

        /**
         * @name Administrative
         * @brief Used by simulation environment
         */
        //@{
        /**
         * @brief Start scheduler (runs until no events left)
         */
        void
        start();

        /**
         * @brief Stop scheduler
         */
        void
        stop();

        /**
         * @brief Returns number of evnts currently in the queue
         */
        size_t
        size() const;
        //@}

        /**
         * @brief Destructor
         */
        virtual
        ~Interface();

    protected:

        /**
         * @brief Constructor
         *
         * Protected since it is an interface and can only be called from
         * derived classes.
         */
        Interface();

    private:
        // NVI (non-virtual interface) - For explanation of the methods see
        // above
        virtual wns::simulator::Time
        doGetTime() const = 0;

	    virtual IEventPtr
	    doSchedule(const Callable& callable, wns::simulator::Time at) = 0;

	    virtual IEventPtr
	    doScheduleNow(const Callable& callable) = 0;

        virtual ICommandPtr
        doQueueCommand(const Callable& callable) = 0;

        virtual void
        doDequeueCommand(const ICommandPtr& command) = 0;

	    virtual void
	    doCancelEvent(const IEventPtr& event) = 0;

        virtual void
        doReset() = 0;

        virtual bool
        doProcessOneEvent() = 0;

        virtual void
        doStart() = 0;

        virtual void
        doStop() = 0;

        virtual size_t
        doSize() const = 0;

        // end NVI


        /**
         * @name Notification
         */
        //{@
        virtual void
        sendProcessOneEventNotification() = 0;

        virtual void
        sendCancelEventNotification() = 0;

        virtual void
        sendScheduleNotification() = 0;

        virtual void
        sendScheduleDelayNotification() = 0;

        virtual void
        sendScheduleNowNotification() = 0;
        //@}
    };

    typedef wns::Creator<wns::events::scheduler::Interface> Creator;
    typedef StaticFactory<wns::events::scheduler::Creator> Factory;

} // scheduler
} // events
} // wns

#endif // NOT defined WNS_EVENTS_SCHEDULER_INTERFACE_HPP
