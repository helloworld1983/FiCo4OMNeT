//Copyright (c) 2014, CoRE Research Group, Hamburg University of Applied Sciences
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification,
//are permitted provided that the following conditions are met:
//
//1. Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
//2. Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
//3. Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software without
//   specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
//ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "fico4omnet/buffer/can/CanBuffer.h"

namespace FiCo4OMNeT {

void CanBuffer::initialize(){
    Buffer::initialize();
    MOB = par("MOB");
    currentFrame = nullptr;
}

CanDataFrame* CanBuffer::getFrame(unsigned int canID) {
    for (std::list<cMessage*>::iterator it = frames.begin();
            it != frames.end(); ++it) {
        CanDataFrame* tmp = dynamic_cast<CanDataFrame*>(*it);
        unsigned int i = tmp->getCanID();
        if (i == canID) {
            return tmp;
        }
    }
    return nullptr;
}

void CanBuffer::putFrame(cMessage* msg){
    CanDataFrame *frame = check_and_cast<CanDataFrame *>(msg);
    frames.push_back(frame);
    emit(queueLengthSignal, static_cast<unsigned long>(frames.size()));
    queueSize+=static_cast<size_t>(frame->getByteLength());
    emit(queueSizeSignal, static_cast<unsigned long>(queueSize));
}

void CanBuffer::deleteFrame(unsigned int canID) {
    Enter_Method_Silent();
    CanDataFrame *tmp = getFrame(canID);
    frames.remove(tmp);
    emit(queueLengthSignal, static_cast<unsigned long>(frames.size()));
    queueSize-=static_cast<size_t>(tmp->getByteLength());
    emit(queueSizeSignal, static_cast<unsigned long>(queueSize));
    delete tmp;
}

void CanBuffer::deleteFrame(CanDataFrame* frame) {
    Enter_Method_Silent();
    frames.remove(frame);
    emit(queueLengthSignal, static_cast<unsigned long>(frames.size()));
    queueSize-=static_cast<size_t>(frame->getByteLength());
    emit(queueSizeSignal, static_cast<unsigned long>(queueSize));
    delete frame;
}

void CanBuffer::deliverFrame(unsigned int canID) {
    Enter_Method_Silent();
    currentFrame = getFrame(canID);
    sendToDestinationGates(currentFrame->dup());
}

void CanBuffer::deliverPrioFrame() {
    Enter_Method_Silent();
    unsigned int prioId = INT_MAX;
    CanDataFrame *prioFrame = nullptr;
    for (std::list<cMessage*>::iterator it = frames.begin();
            it != frames.end(); ++it) {
        CanDataFrame *tmp = dynamic_cast<CanDataFrame*>(*it);
        unsigned int i = tmp->getCanID();
        if ((i < prioId)) {
            prioFrame = tmp;
            prioId = i;
        }
    }
    sendToDestinationGates(prioFrame->dup());
}

void CanBuffer::deliverNextFrame() {
    Enter_Method_Silent();
    sendToDestinationGates(frames.front()->dup());
}

CanDataFrame* CanBuffer::getCurrentFrame(){
    return currentFrame;
}

}
