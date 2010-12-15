#ifndef WNS_SERVICE_PHY_LTE_DATATRANSMISSION_HPP
#define WNS_SERVICE_PHY_LTE_DATATRANSMISSION_HPP

#include <WNS/node/Node.hpp>
#include <WNS/PowerRatio.hpp>
#include <WNS/osi/PDU.hpp>
#include <WNS/service/Service.hpp>
#include <WNS/service/phy/imta/ChannelStateInfo.hpp>
#include <vector>

namespace wns { namespace service { namespace phy { namespace imta {
	
   
 
	class DataTransmission :
    public virtual service::Service {
	public:
	  virtual void registerTransmission(
					    // wns::node::Node* source should be known to the LTE Phy component
					    wns::node::Interface* destination,
					    // later: precoding (vector/matrix?),
					    wns::Power txPower,
					    wns::osi::PDUPtr transportBlock,
					    PRBList transmitPRBs,
					    unsigned int TTI) = 0; // in which TTI to send this, we might omit this if we always schedule for immediate transmission
					    //LTEphyMode phyMode)
	    };
	
      }}}}
#endif
