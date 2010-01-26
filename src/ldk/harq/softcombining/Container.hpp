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

#ifndef WNS_LDK_HARQ_SOFTCOMBINING_CONTAINER_HPP
#define WNS_LDK_HARQ_SOFTCOMBINING_CONTAINER_HPP

#include <WNS/Exception.hpp>

#include <list>
#include <vector>

namespace wns { namespace ldk { namespace harq { namespace softcombining {

     template <class T>
     class Container
     {
     public:

         typedef std::list<T> EntryList;

         typedef std::vector<EntryList> EntryListVector;

         class InvalidRV :
             public Exception
         {
         };

         Container(int numRVs)
         {
             receivedEntries_.resize(numRVs);
         }

         void
         clear()
         {
             int size = receivedEntries_.size();
             receivedEntries_.clear();
             receivedEntries_.resize(size);
         }


         int
         getNumRVs() const
         {
             return receivedEntries_.size();
         }


         EntryList
         getEntriesForRV(int rv) const
         {
             checkIfValidRV(rv);

             return receivedEntries_[rv];
         }


         void
         appendEntryForRV(int rv, T compound)
         {
             checkIfValidRV(rv);

             receivedEntries_[rv].push_back(compound);
         }

     private:

         void
         checkIfValidRV(int rv) const
         {
             if (rv < 0 || rv >= getNumRVs())
             {
                 throw typename Container::InvalidRV();
             }
         }

         EntryListVector receivedEntries_;
     };

} // softcombining
} // harq
} // ldk
} // wns

#endif // WNS_LDK_HARQ_SOFTCOMBINING_CONTAINER_HPP
