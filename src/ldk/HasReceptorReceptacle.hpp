/******************************************************************************
 * WNS (Wireless Network Simulator)                                           *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2004-2008                                                    *
 * Chair of Communication Networks (ComNets)                                  *
 * Kopernikusstr. 1, D-52074 Aachen, Germany                                  *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
 ******************************************************************************/

#ifndef WNS_LDK_HASRECEPTORRECEPTACLE_HPP
#define WNS_LDK_HASRECEPTORRECEPTACLE_HPP

#include <WNS/ldk/IReceptorReceptacle.hpp>
#include <WNS/ldk/ReceptorReceptacleRegistry.hpp>
#include <WNS/ldk/LinkHandlerInterface.hpp>
#include <WNS/ldk/SinglePort.hpp>
#include <WNS/ldk/Port.hpp>
#include <WNS/ldk/fun/FUN.hpp>

#include <WNS/Assure.hpp>

#include <string>

namespace wns { namespace ldk {

        template <typename CLASS, typename PORTID = SinglePort>
        class HasReceptorReceptacle
            : public virtual ReceptorReceptacleRegistry
        {
        public:
            HasReceptorReceptacle(CLASS* fu)
                : ReceptorReceptacleRegistry(),
                  receptorReceptacle_(fu)
            {
                addToReceptorReceptacleRegistry(PORTID().name, &receptorReceptacle_);
            }

            virtual
            ~HasReceptorReceptacle()
            {}

            class ReceptorReceptacle
                : public virtual IReceptorReceptacle
            {
            public:
                ReceptorReceptacle(CLASS* fu)
                    : fu_(fu)
                {}

                virtual
                ~ReceptorReceptacle()
                {}

                virtual void
                wakeup()
                {
                    fu_->getFUN()->getLinkHandler()->wakeup(this);
                }

                virtual void
                doWakeup()
                {
                    fu_->doWakeup(Port<PORTID>());
                }

                virtual FunctionalUnit*
                getFU()
                {
                    return fu_;
                }

            private:
                CLASS* fu_;
            };

        private:
            //            HasReceptorReceptacle()
            //            {}

            ReceptorReceptacle receptorReceptacle_;
        };


        template <typename CLASS>
        class HasReceptorReceptacle<CLASS, SinglePort>
            : public virtual IReceptorReceptacle,
              public virtual ReceptorReceptacleRegistry
        {
        public:
            HasReceptorReceptacle()
                : ReceptorReceptacleRegistry()
            {
                //                assureNotNull(dynamic_cast<CLASS*>(this));
                //                fu_ = dynamic_cast<CLASS*>(this);
                addToReceptorReceptacleRegistry(SinglePort().name, this);
            }

            HasReceptorReceptacle(FunctionalUnit* fu)
                : ReceptorReceptacleRegistry(),
                  fu_(fu)
            {
                addToReceptorReceptacleRegistry(SinglePort().name, this);
            }

            virtual
            ~HasReceptorReceptacle()
            {}

            virtual void
            wakeup()
            {
                dynamic_cast<CLASS*>(this)->getFUN()->getLinkHandler()->wakeup(this);
            }

            virtual FunctionalUnit*
            getFU()
            {
                return dynamic_cast<CLASS*>(this);
            }

        private:
            CLASS* fu_;
        };


    } // ldk
} // wns

#endif // NOT defined WNS_LDK_HASRECEPTORRECEPTACLE_HPP
