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

#ifndef WNS_SERVICE_QOS_HPP
#define WNS_SERVICE_QOS_HPP

#include <string>
#include <WNS/Enum.hpp>

namespace wns {
	namespace service {
		namespace qos {
			// this must be consistent with Python file Qos.py
			ENUM_BEGIN(QoSClasses);
			ENUM(UNDEFINED,		0);     // should not occur? Or map to worst priority?
			ENUM(CONTROLPLANE,	1);	// Control Messages
			ENUM(REALTIME,		2);	// VoIP, VT, etc.
			ENUM(GUARANTEEDTP,	3);	// Streaming (one side traffic)
			ENUM(BESTEFFORT,	4);	// FileTransfer/Background
			ENUM_END();
			typedef int QoSClass;
			// wns::service::qos::QoSClasses::UNDEFINED() // use this to get a QoSClass
			const unsigned int NUMBEROFQOSCLASSES = 5;

			// wns::service::qos::QoSClasses::toString(qosClass);
			// wns::service::qos::QoSClasses::fromString("BESTEFFORT")
			// wns::service::qos::QoSClasses::CONTROLPLANE()

			// TODO: "UMTS Bearer Service Attributes"
			// http://www.umtsworld.com/technology/qos.htm

			/** @brief define stream operator for class QoSClass */
			inline std::ostream&
			operator<< (std::ostream& s, const QoSClass& qosClass) {
			  return s << wns::service::qos::QoSClasses::toString(qosClass);
			}
		}
	}
}

#endif // WNS_SERVICE_QOS_HPP
