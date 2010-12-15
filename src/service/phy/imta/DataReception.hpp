#ifndef WNS_SERVICE_PHY_LTE_DATARECEPTION_HPP
#define WNS_SERVICE_PHY_LTE_DATARECEPTION_HPP

#include <WNS/osi/PDU.hpp>
#include <WNS/SmartPtr.hpp>
#include <WNS/node/Interface.hpp>
#include <WNS/service/phy/imta/ChannelStateInfo.hpp>

namespace wns { namespace service { namespace phy { namespace imta {
	
	class DataReception :
    public virtual service::Service {
	public:
	  virtual void onData(wns::osi::PDUPtr transportBlock, wns::node::Interface* source, wns::SmartPtr<ChannelStateInfo> csi) = 0;
	  
	};
	
      }}}}
#endif


  
