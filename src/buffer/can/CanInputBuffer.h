//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __FICO4OMNET_CANINPUTBUFFER_H_
#define __FICO4OMNET_CANINPUTBUFFER_H_

#include <omnetpp.h>
#include "CanBuffer.h"


namespace FiCo4OMNeT {
/**
 * @brief This buffer holds messages which were received by this node.
 *
 * @ingroup Buffer
 *
 * @author Stefan Buschmann
 */
class CanInputBuffer :public CanBuffer{

public:
    /**
     * @brief Puts the frame into the collection and informs the connected gates about the receiption.
     *
     * @param frame The DataFrame to put in the buffer.
     *
     */
    virtual void putFrame(cMessage* msg);
};

}
#endif /* CANINPUTBUFFER_H_ */