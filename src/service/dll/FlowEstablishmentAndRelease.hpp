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

#ifndef WNS_SERVICE_DLL_FLOWESTABLISHMENTANDRELEASE_HPP
#define WNS_SERVICE_DLL_FLOWESTABLISHMENTANDRELEASE_HPP

#include <WNS/service/Service.hpp>
#include <WNS/service/nl/Address.hpp>
#include <WNS/service/tl/FlowID.hpp>
#include <WNS/service/qos/QoSClasses.hpp>

namespace wns { namespace service { namespace dll {

	/**
	 * @brief tl service specification.
	 * @ingroup interface
	 */
	class FlowEstablishmentAndRelease:
		virtual public wns::service::Service
	{
	public:
		/**
		 * @brief Establish a new flow.
		 *
		 * @param[in] _sourcePort Own local port.
		 *
		 * @param[in] _destPort On which port to connect peer.
		 *
		 * @param[in] _peer Own IP address.
		 *
		 * @param[in] _peer On which address to contact peer.
		 */
		virtual void
		establishFlow(wns::service::tl::FlowID flowID, wns::service::qos::QoSClass qosClass)=0;

		/**
		 * @brief Release an existing flow.
		 *
		 * @param[in] _sourcePort Own local port.
		 *
		 * @param[in] _destPort On which port to connect peer.
		 *
		 * @param[in] _peer Own IP address.
		 *
		 * @param[in] _peer On which address to contact peer.
		 */
		virtual void
		releaseFlow(wns::service::tl::FlowID flowID)=0;

	};
}
}
}

#endif // WNS_SERVICE_DLL_FLOWESTABLISHMENTANDRELEASE_HPP

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
