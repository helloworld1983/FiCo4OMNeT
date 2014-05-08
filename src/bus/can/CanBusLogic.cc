#include "CanBusLogic.h"

namespace FiCo4OMNeT {

Define_Module(CanBusLogic);

void CanBusLogic::initialize() {
    rcvdDFSignal = registerSignal("receivedDF");
    rcvdRFSignal = registerSignal("receivedRF");
    rcvdEFSignal = registerSignal("receivedEF");
    numDataFrames = 0;
    numRemoteFrames = 0;
    numErrorFrames = 0;

    busytime = 0.0;
    busytimestamp = 0.0;
    errpos = INT_MAX;
    errored = false;
    idle = true;
    char buf[64];
    sprintf(buf, "state: idle");
    bubble("state: idle");
    getDisplayString().setTagArg("tt", 0, buf);
    scheduledDataFrame = new CanDataFrame();

    bandwidth = getParentModule()->par("bandwidth");

    EV<< "canVersion: " << getParentModule()->par("version").stdstringValue() << "\n";
}

void CanBusLogic::finish() {
    simtime_t busload = (busytime / simTime()) * 100;
    if (busload == 0.0 && !idle) {
        busload = 100.0;
    }
    recordScalar("#Simulated_Time", simTime());
    recordScalar("%Busload", busload);
    double errpercentage = (numErrorFrames
            / (double) (numDataFrames + numRemoteFrames)) * 100;
    recordScalar("%Errors", errpercentage);
}

void CanBusLogic::handleMessage(cMessage *msg) {
//    std::string msgClass = msg->getClassName();
    if (msg->isSelfMessage()) { //Bus ist wieder im Idle-Zustand
        if (dynamic_cast<CanDataFrame *>(msg)) { //Wenn zuvor eine Nachricht gesendet wurde
            sendingCompleted();
        } else if (dynamic_cast<ErrorFrame *>(msg)) {
            colorIdle();
            if (scheduledDataFrame != NULL) {
                cancelEvent(scheduledDataFrame);
            }
            delete scheduledDataFrame;
            scheduledDataFrame = NULL;
            errored = false;
            eraseids.clear();
        }
        grantSendingPermission();
    } else if (dynamic_cast<CanDataFrame *>(msg)) { // externe Nachricht
        colorBusy();
        handleDataFrame(msg);
    } else if (dynamic_cast<ErrorFrame *>(msg)) {
        colorError();
        handleErrorFrame(msg);
    }

    delete msg;
}

void CanBusLogic::grantSendingPermission() {
    currentSendingID = INT_MAX;
    sendingNode = NULL;

    for (std::list<CanID*>::iterator it = ids.begin(); it != ids.end(); ++it) { //finden der höchsten Priorität aller angemeldeten Nachrichten
        CanID *id = *it;
        if (id->getId() < currentSendingID) {
            currentSendingID = id->getId();
            sendingNode = (CanOutputBuffer*) id->getNode();
            currsit = id->getSignInTime();
        }
    }

    int sendcount = 0;
    for (std::list<CanID*>::iterator it = ids.begin(); it != ids.end(); ++it) { //finden, ob remote frame für diese ID auch gesendet werden soll
        CanID *id = *it;
        if (id->getId() == currentSendingID) {
            if (id->getRtr() == false) { //Data-Frame
                sendingNode = (CanOutputBuffer*) id->getNode(); //der Node, der einen Data frame senden möcte wird zum senden ausgewählt
                currsit = id->getSignInTime();
                sendcount++;
            }
            eraseids.push_back(it);
        }
    }
    if (sendcount > 1) {
        EV << "More than one node sends with the same ID.\n";
        EV
                  << "This behavior is not allowed. Hence the simulation is stopped.\n";
        endSimulation();
    }
    if (sendingNode != NULL) {
        CanOutputBuffer* controller = check_and_cast<CanOutputBuffer *>(
                sendingNode);
        controller->receiveSendingPermission(currentSendingID);
    } else {
        EV << "no pending message" << endl;
        simtime_t timetaken = simTime() - busytimestamp;
        busytime += timetaken;
        EV << "Busytime: " << busytime << "\n";
        idle = true;
        char buf[64];
        sprintf(buf, "state: idle");
        getDisplayString().setTagArg("tt", 0, buf);
        bubble("state: idle");
    }
}

void CanBusLogic::sendingCompleted() {
    colorIdle();
    CanOutputBuffer* controller = check_and_cast<CanOutputBuffer*>(sendingNode);
    controller->sendingCompleted(currentSendingID);
    for (unsigned int it = 0; it != eraseids.size(); it++) {
        ids.erase(eraseids.at(it));
    }
    eraseids.clear();
    errored = false;
    if (scheduledDataFrame != NULL) {
        cancelEvent(scheduledDataFrame);
    }
    scheduledDataFrame = NULL;
}

void CanBusLogic::handleDataFrame(cMessage *msg) {
    CanDataFrame *df = check_and_cast<CanDataFrame *>(msg);
    EV << "dataframe mit canid: " << df->getCanID() << " empfangen \n";
    int length = df->getLength();
    double nextidle;
    nextidle = (double) length / (bandwidth);
    EV<< "#####message wird gescheduled#####\n";
    EV<< "Gr��e: " << df->getLength() << "nextidle: " << nextidle << "\n";
    //TODO Der naechste Idle-Zustand ist eigentlich die (berechnete Zeit - 1), aber hier ist wieder die Sicherheits-Bitzeit mit verrechnet; Ist das so?
    if (scheduledDataFrame != NULL) {
        cancelEvent(scheduledDataFrame);
    }
    delete (scheduledDataFrame);
    scheduledDataFrame = df->dup();
    scheduleAt(simTime() + nextidle, scheduledDataFrame);
    if (df->getRtr()) {
        emit(rcvdRFSignal, df);
        numRemoteFrames++;
    } else {
        emit(rcvdDFSignal, df);
        numDataFrames++;
    }
    send(msg->dup(), "gate$o");
}

void CanBusLogic::handleErrorFrame(cMessage *msg) {
    if (scheduledDataFrame != NULL) {
        cancelEvent(scheduledDataFrame);
    }
    if (!errored) {
        numErrorFrames++;
        ErrorFrame *ef2 = new ErrorFrame();
//        scheduleAt(simTime() + (12 / (bandwidth * 1000 * 1000)), ef2); //12 - maximale L�nge eines Error-Frames
        scheduleAt(simTime() + (12 / (bandwidth)), ef2); //12 - maximale L�nge eines Error-Frames
        emit(rcvdEFSignal, ef2);
        errored = true;
        send(msg->dup(), "gate$o");
    }
}

void CanBusLogic::registerForArbitration(int id, cModule *node,
        simtime_t signInTime, bool rtr) {
    Enter_Method_Silent
    ();
    ids.push_back(new CanID(id, node, signInTime, rtr));
    if (idle) {
        cMessage *self = new cMessage("idle_signin");
        EV<<"scheudle at: " << (simTime() + (1 / (bandwidth))) << "\n";
        scheduleAt(simTime() + (1 / (bandwidth)), self); //TODO was hat das mit dieser +1 auf sich?
        idle = false;
        busytimestamp = simTime();
        char buf[64];
        sprintf(buf, "state: busy");
        bubble("state: busy");
        getDisplayString().setTagArg("tt", 0, buf);
    }
}

void CanBusLogic::checkoutFromArbitration(int id) {
    Enter_Method_Silent
    ();
    for (std::list<CanID*>::iterator it = ids.begin(); it != ids.end(); ++it) {
        CanID* tmp = *it;
        if (tmp->getId() == id) {
            ids.remove(tmp);
        }
        break;
    }
}

void CanBusLogic::colorBusy() {
    if (ev.isGUI()) {
        for (int gateIndex = 0;
                gateIndex
                        < getParentModule()->gate("gate$o", 0)->getVectorSize();
                gateIndex++) {
            getParentModule()->gate("gate$o", gateIndex)->getDisplayString().setTagArg(
                    "ls", 0, "yellow");
            getParentModule()->gate("gate$o", gateIndex)->getDisplayString().setTagArg(
                    "ls", 1, "3");

            //TODO: This is necessary due to visualization issues with OMNeT++
            getParentModule()->gate("gate$i", gateIndex)->getPreviousGate()->getDisplayString().setTagArg(
                    "ls", 0, "yellow");
            getParentModule()->gate("gate$i", gateIndex)->getPreviousGate()->getDisplayString().setTagArg(
                    "ls", 1, "3");
        }
    }
}

void CanBusLogic::colorIdle() {
    if (ev.isGUI()) {
        for (int gateIndex = 0;
                gateIndex
                        < getParentModule()->gate("gate$o", 0)->getVectorSize();
                gateIndex++) {
            getParentModule()->gate("gate$o", gateIndex)->getDisplayString().setTagArg(
                    "ls", 0, "black");
            getParentModule()->gate("gate$o", gateIndex)->getDisplayString().setTagArg(
                    "ls", 1, "1");

            //TODO: This is necessary due to visualization issues with OMNeT++
            getParentModule()->gate("gate$i", gateIndex)->getPreviousGate()->getDisplayString().setTagArg(
                    "ls", 0, "black");
            getParentModule()->gate("gate$i", gateIndex)->getPreviousGate()->getDisplayString().setTagArg(
                    "ls", 1, "1");
        }
    }
}

void CanBusLogic::colorError() {
    if (ev.isGUI()) {
        for (int gateIndex = 0;
                gateIndex
                        < getParentModule()->gate("gate$o", 0)->getVectorSize();
                gateIndex++) {
            getParentModule()->gate("gate$o", gateIndex)->getDisplayString().setTagArg(
                    "ls", 0, "red");
            getParentModule()->gate("gate$o", gateIndex)->getDisplayString().setTagArg(
                    "ls", 1, "3");

            //TODO: This is necessary due to visualization issues with OMNeT++
            getParentModule()->gate("gate$i", gateIndex)->getPreviousGate()->getDisplayString().setTagArg(
                    "ls", 0, "red");
            getParentModule()->gate("gate$i", gateIndex)->getPreviousGate()->getDisplayString().setTagArg(
                    "ls", 1, "3");
        }
    }
}

}
