/*
(C) LKN, TUM 2010
*/

#ifndef CHANNELSTATEINFO_HPP
#define CHANNELSTATEINFO_HPP

#include <WNS/RefCountable.hpp>
#include <WNS/service/phy/imta/IMTAphyObserver.hpp>
#include <WNS/PowerRatio.hpp>
#include <map>


namespace wns { namespace service { namespace phy { namespace imta {

	 typedef unsigned int PRB;
	 typedef std::list<PRB> PRBList;
	 typedef std::map<PRB, wns::Ratio> PRBMapdB;
	 typedef std::map<PRB, wns::Power> PRBMapdBm;
	 

	 class ChannelStateInfo :
              public wns::RefCountable
	 {
	 public:
	   PRBMapdB  sinrs;
	   PRBMapdBm rxPowers;
	 };

	 typedef wns::SmartPtr<ChannelStateInfo> ChannelStateInfoPtr;
       
       }
     }
   }
}

#endif // CHANNELSTATEINFO_H
