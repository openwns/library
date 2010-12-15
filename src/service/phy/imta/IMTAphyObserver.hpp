#ifndef LTEPHYOBSERVER_HPP
#define LTEPHYOBSERVER_HPP

#include <WNS/osi/PDU.hpp>
#include <WNS/SmartPtr.hpp>

namespace wns { namespace service { namespace phy { namespace imta {

class IMTAphyObserver
{
    public:
	virtual void onNewTTI(unsigned int ttiNumber) = 0;
};

}}}}

#endif // LTEPHYOBSERVER_HPP
