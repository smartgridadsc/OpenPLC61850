#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <time.h>

#include "iec61850_server.h"
#include "hal_thread.h"
#include "static_model.h"

#include "ladder.h"

#define OPLC_CYCLE  50000000

extern IedModel iedModel;

IedServer iedServer = NULL;

unsigned char log_msg_iecserver[1000];

static ControlHandlerResult
controlHandlerForBinaryOutput(ControlAction action, void* parameter, MmsValue* value, bool test)
{
    uint64_t timestamp = Hal_getTimeInMs();

    sprintf(log_msg_iecserver, "control handler called\n");
    log(log_msg_iecserver);
    sprintf(log_msg_iecserver, "  ctlNum: %i\n", ControlAction_getCtlNum(action));
    log(log_msg_iecserver);

    ClientConnection clientCon = ControlAction_getClientConnection(action);

    if (clientCon) {
        sprintf(log_msg_iecserver, "Control from client %s\n", ClientConnection_getPeerAddress(clientCon));
        log(log_msg_iecserver);
    }
    else {
        sprintf(log_msg_iecserver, "clientCon == NULL!\n", ClientConnection_getPeerAddress(clientCon));
        log(log_msg_iecserver);
    }

    if (parameter == IEDMODEL_LogicalDevice_GGIO5_SPCSO) {
        //IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_LogicalDevice_GGIO5_SPCSO_Oper_T, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_LogicalDevice_GGIO5_SPCSO_Oper_ctlVal, value);
    }
    else
        return CONTROL_RESULT_FAILED;

    return CONTROL_RESULT_OK;
}

void startIec61850Server(int port) 
{
    iedServer = IedServer_create(&iedModel);

    //Map objects to addresses (maybe map during compilation time)

    //IedServer_setControlHandler();
    //report sending

    IedServer_setWriteAccessPolicy(iedServer, IEC61850_FC_ALL, ACCESS_POLICY_ALLOW);

    IedServer_setControlHandler(
            iedServer, 
            IEDMODEL_LogicalDevice_GGIO5_SPCSO,
            (ControlHandler) controlHandlerForBinaryOutput,
            IEDMODEL_LogicalDevice_GGIO5_SPCSO);

    //Initialize values
    DataAttribute* ctlVal = (DataAttribute*) 
                IedModel_getModelNodeByObjectReference(&iedModel, "WAGO61850ServerLogicalDevice/GGIO5.SPCSO.Oper.ctlVal");
    MmsValue* mmstrue = MmsValue_newBoolean(true);
    MmsValue* mmsfalse = MmsValue_newBoolean(false);
    ctlVal->mmsValue = mmsfalse;


    //start server
    IedServer_start(iedServer, port);

    if (!IedServer_isRunning(iedServer)) {
        sprintf(log_msg_iecserver, "Starting IEC61850 Server failed! (Ensure sudo permissions)\n");
        log(log_msg_iecserver);
        IedServer_destroy(iedServer);
        exit(-1);
    }

    // Continuously update
    struct timespec timer_start;
    clock_gettime(CLOCK_MONOTONIC, &timer_start);


    //==============================================
    //   MAIN LOOP
    //==============================================
    while(run_iec61850) {
        /*
        //mutex lock buffer
        pthread_mutex_lock(&bufferLock);

        //update values of objects in IedServer to buffer

        //mutex release
        
        IedServer_lockDataModel(iedServer);
        
        //bool boolean = MmsValue_getBoolean(ctlVal->mmsValue);

        //std::cout << "print this: [" << boolean << "]\n";

        IedServer_unlockDataModel(iedServer);
        
        //mutex lock buffer

        //update values of objects in IedServer to buffer

        //mutex release
        pthread_mutex_unlock(&bufferLock);

        sleep_until(&timer_start, OPLC_CYCLE);
        */
    }

    //clean up
    IedServer_stop(iedServer);
    IedServer_destroy(iedServer);
}