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

#include "FRPortOutput.h"

namespace FiCo4OMNeT {

Define_Module(FRPortOutput);

void FRPortOutput::initialize() {
    bandwidth = getParentModule()->getParentModule()->par("bandwidth");
    initializeStatisticValues();
}

void FRPortOutput::finish() {
//    collectStats();
}

void FRPortOutput::initializeStatisticValues() {
//    sentDFSignal = registerSignal("sentDF");
//    sentRFSignal = registerSignal("sentRF");
}

void FRPortOutput::handleMessage(cMessage *msg) {
    FRFrame *frMsg = dynamic_cast<FRFrame *>(msg);
    colorBusy();
    //TODO stats
    send(frMsg, "out");
}

void FRPortOutput::sendingCompleted() {
    colorIdle();
}

void FRPortOutput::colorBusy() {
    getParentModule()->getParentModule()->getDisplayString().setTagArg("i", 1,
            "yellow");
    getParentModule()->getParentModule()->gate("gate$i")->getDisplayString().setTagArg(
            "ls", 0, "yellow");
    getParentModule()->getParentModule()->gate("gate$i")->getDisplayString().setTagArg(
            "ls", 1, "3");
    getParentModule()->getParentModule()->gate("gate$i")->getPreviousGate()->getDisplayString().setTagArg(
            "ls", 0, "yellow");
    getParentModule()->getParentModule()->gate("gate$i")->getPreviousGate()->getDisplayString().setTagArg(
            "ls", 1, "3");
}

void FRPortOutput::colorIdle() {
    getParentModule()->getParentModule()->getDisplayString().setTagArg("i", 1,
            "");
    getParentModule()->getParentModule()->gate("gate$i")->getDisplayString().setTagArg(
            "ls", 0, "black");
    getParentModule()->getParentModule()->gate("gate$i")->getDisplayString().setTagArg(
            "ls", 1, "1");
    getParentModule()->getParentModule()->gate("gate$i")->getPreviousGate()->getDisplayString().setTagArg(
            "ls", 0, "black");
    getParentModule()->getParentModule()->gate("gate$i")->getPreviousGate()->getDisplayString().setTagArg(
            "ls", 1, "1");
}

}
