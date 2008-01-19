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

#ifndef WNS_PROBE_BUS_PROBEBUS_HPP
#define WNS_PROBE_BUS_PROBEBUS_HPP

#include <WNS/simulator/Time.hpp>

#include <WNS/Subject.hpp>
#include <WNS/Observer.hpp>

#include <WNS/probe/bus/Context.hpp>

#include <WNS/PyConfigViewCreator.hpp>
#include <WNS/StaticFactory.hpp>

namespace wns { namespace probe { namespace bus {

    void
    addProbeBusses(const wns::pyconfig::View& pyco);

    /**
     * @brief Internal Interface for Subject/Observer implementation which is used as backend for the ProbeBus
     *
     * @author Daniel Bültmann <dbn@comnets.rwth-aachen.de>
     */
    class ProbeBusNotificationInterface
    {
    public:
        virtual ~ProbeBusNotificationInterface() {};

        /**
         * @brief Send a value on this ProbeBus
         * @param probeId The ProbeID of the associated probe
         * @param aValue The value to send (this was formerly the argument to probe->put)
         * @param idRegistry The set of IDs known in this scope. You may sort according to these.
         * @note The Context is deleted after the send call. Do not store it!
         *
         * This method is used to forward measurements. Call this if you generate a new measurement
         * and want it to propagate through the ProbeBus Hierarchy.
         */
        virtual void
        forwardMeasurement(const wns::simulator::Time&,
                           const double&,
                           const IContext&) = 0;

        /**
         * @brief Trigger writing of output.
         */
        virtual void
        forwardOutput() = 0;
    };

    /**
     * @brief Interface for a ProbeBus. ProbeBusses may be chained.
     *
     * ProbeBus is the basis for measurement distribution within the simulator. Each concrete
     * ProbeBus Instance must implement three methods
     * accepts : Which is called to determine if the ProbeBus would accept the measurement
     * onMeasurement : Which is called when accepts returned true and delivers the measurement
     * output : Is called when attached ProbeBusses should write theirselves to persistant storage.
     *
     * The ProbeBus uses wns::Subject and wns::Observer as its backend. The NotficationInterface is defined
     * by ProbeBusNotificationInterface. Since ProbeBusses may be chained each ProbeBus is a Subject and an
     * Observer by itself.
     *
     * Use startObserving and stopObserving to interconnect ProbeBusses
     * @note This is a new feature. Consider it beta. Subject to change.
     * @author Daniel Bültmann <dbn@comnets.rwth-aachen.de>
     */
    class ProbeBus:
        virtual private wns::Subject<ProbeBusNotificationInterface>,
        virtual private wns::Observer<ProbeBusNotificationInterface>
    {
    public:

        /**
         * @brief Called to decide wether the current ProbeBus wants to receive this measurement
         * @param probeId This is the ProbeID which is the source of the measurement
         * @param registry This is a wns::Registry that contains key, value pairs that you may use for filtering
         */
        virtual bool
        accepts(const wns::simulator::Time&, const IContext&) = 0;

        /**
         * @brief Called to deliver a measurement to client classes
         * @param probeId This is the ProbeID which is the source of the measurement
         * @param aValue The measurement itself
         * @param registry This is a wns::Registry that contains key, value pairs that you may use for filtering
         */
        virtual void
        onMeasurement(const wns::simulator::Time&,
                      const double&,
                      const IContext&) = 0;

        /**
         * @brief Called to trigger writing to persistant storage.
         */
        virtual void
        output() = 0;

        /**
         * @brief Method used by backend to propagate measurements. This is the entry point for new measurements.
         *
         * It wraps the subject/observer backend nicely and allows users to determine the behaviour by implementing
         * accepts and onMeasurement. When forwardMeasurement is called it is checked at first wether this ProbeBus
         * accepts this Measurement. If accepts returns true the measurement is locally delivered by calling onMeasurement.
         * After local delivery the measurement is propagated to all Observers.
         * If the call to accepts returns false, the measurement is not delivered locally and is not forwarded to
         * the Observers.
         *
         */
        virtual void
        forwardMeasurement(const wns::simulator::Time&,
                           const double&,
                           const IContext&);

        /**
         * @brief Method used by backend to propagate the output trigger.
         *
         * This method unconditionally calls the output method to trigger writing to persistant storage.
         * After this it unconditionally propagates the trigger to all its Observers.
         */
        virtual void
        forwardOutput();

        virtual void
        addReceivers(const wns::pyconfig::View& pyco);

        virtual void
        startReceiving(ProbeBus* other);

        virtual void
        stopReceiving(ProbeBus* other);

    };

    typedef PyConfigViewCreator<ProbeBus> ProbeBusCreator;
    typedef StaticFactory<ProbeBusCreator> ProbeBusFactory;

    /**
     * @brief Functor that is used send notifies using the forwardMeasurement method.
     *
     * @author Daniel Bültmann <dbn@comnets.rwth-aachen.de>
     */
    class ProbeBusMeasurementFunctor
    {
        typedef void (ProbeBusNotificationInterface::*fPtr)(const wns::simulator::Time&,
                                                            const double&,
                                                            const IContext&);
    public:
        ProbeBusMeasurementFunctor(fPtr _f,
                                   const wns::simulator::Time& _time,
                                   const double& _value,
                                   const IContext& _reg):
            f(_f),
            time(_time),
            value(_value),
            registry(_reg)
            {}

        void
        operator()(ProbeBusNotificationInterface* observer)
            {
                (*observer.*f)(time, value, registry);
            }
    private:
        fPtr f;
        const wns::simulator::Time& time;
        const double& value;
        const IContext& registry;
    };
} // bus
} // probe
} // wns

#endif // WNS_PROBE_BUS_PROBEBUS_HPP
