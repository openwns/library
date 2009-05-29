/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 5, D-52074 Aachen, Germany
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
      ENUM(PCCH,		1);	// Physical Control Channel for UL resource requests and CQI reporting
      ENUM(PHICH,		2);	// Physical HARQ Indicator Channel for HARQ
      ENUM(DCCH,		3);	// Dedicated Control Channel for RRC control signalling like admission control
      ENUM(CONVERSATIONAL,	4);	// VoIP, video telephony
      ENUM(STREAMING,		5);	// video or audio streaming
      ENUM(INTERACTIVE,		6);	// WWW
      ENUM(BACKGROUND,		7);	// file transfer, e-mail
      ENUM_END();
      typedef int QoSClass;
      // wns::service::qos::QoSClasses::UNDEFINED() // use this to get a QoSClass
      const unsigned int NUMBEROFQOSCLASSES = 8;

      // Usage:
      // wns::service::qos::QoSClasses::toString(qosClass);
      // wns::service::qos::QoSClasses::fromString("BACKGROUND")
      // wns::service::qos::QoSClasses::CONVERSATIONAL()

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
