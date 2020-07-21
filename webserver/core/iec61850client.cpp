#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <string>
#include <time.h>

#include "hal_thread.h"
#include "iec61850_client.h"

#include "ladder.h"

unsigned char log_msg_iecclient[1000];

void reportCallbackFunction(void *parameter, ClientReport report)
{
    //read from config to determine what element is of what type and writes to what
    MmsValue *dataSetValues = ClientReport_getDataSetValues(report);

    int location = 20;

    std::string reportref = ClientReport_getRcbReference(report);
    if (reportref.compare("IEDServer01LogicalDevice/LLN0.RP.Measurements01") == 0) {
        location = 0;
    }
    else if (reportref.compare("IEDServer02LogicalDevice/LLN0.RP.Measurements01") == 0) {
        location = 10;
    }

    pthread_mutex_lock(&bufferLock);
    MmsValue *magf = MmsValue_getElement(dataSetValues, 0);
    auto flt = MmsValue_toFloat(magf);
    if (dint_memory[location] != NULL) {
        *dint_memory[location] = *(reinterpret_cast<int32_t *>(&flt));
    }
    pthread_mutex_unlock(&bufferLock);
    /*
    MmsValue* q = MmsValue_getElement(dataSetValues, 1);
    auto gg = MmsValue_getBitStringAsInteger(q);
    sprintf(log_msg_iecclient, "Q = %f\n", gg);
    log(log_msg_iecclient);
    if(dint_memory[location+1] != NULL) {
        *dint_memory[location+1] = gg;
    }
    pthread_mutex_unlock(&bufferLock);
    */

    LinkedList dataSetDirectory = (LinkedList)parameter;
    if (dataSetDirectory) {
        int i;
        for (i = 0; i < LinkedList_size(dataSetDirectory); i++) {
            ReasonForInclusion reason = ClientReport_getReasonForInclusion(report, i);
            char valBuffer[500];
            sprintf(valBuffer, "no value");

            if (dataSetValues) {
                MmsValue *value = MmsValue_getElement(dataSetValues, i);

                if (value) {
                    MmsValue_printToBuffer(value, valBuffer, 500);
                }
            }

            LinkedList entry = LinkedList_get(dataSetDirectory, i);

            char *entryName = (char *)entry->data;
            sprintf(log_msg_iecclient, "  %s (included for reason %i): %s\n", entryName, reason, valBuffer);
            log(log_msg_iecclient);
        }
    }
}

void run_iec61850_client()
{
    Thread_sleep(5000);

    //read clientconfig

    //read mapping

    IedClientError error;

    //printf("**************SETTING UP CRCB FOR IEDONE***************\n");
    IedConnection con1 = IedConnection_create();
    int port1 = 11111;
    char *hostname1 = "192.168.194.129";

    IedConnection_connect(con1, &error, hostname1, port1);
    ClientReportControlBlock rcb1;

    if (error == IED_ERROR_OK) {

        /* Read RCB values */
        rcb1 = IedConnection_getRCBValues(
                con1, &error, "IEDServer01LogicalDevice/LLN0.RP.Measurements01", NULL);
        if (error != IED_ERROR_OK) {
            sprintf(log_msg_iecclient, "Fail to get RCB1 values (code: %i)\n", error);
            log(log_msg_iecclient);
        }
        if (rcb1) {
            /* Install handler for reports */
            LinkedList dir1 = IedConnection_getDataSetDirectory(con1, &error, "IEDServer01LogicalDevice/LLN0$Measurements", NULL);
            IedConnection_installReportHandler(con1, "IEDServer01LogicalDevice/LLN0.RP.Measurements01", ClientReportControlBlock_getRptId(rcb1), reportCallbackFunction, dir1);

            /* Set trigger options and enable report */
            ClientReportControlBlock_setTrgOps(rcb1, TRG_OPT_DATA_UPDATE | TRG_OPT_INTEGRITY | TRG_OPT_GI);
            ClientReportControlBlock_setRptEna(rcb1, true);
            ClientReportControlBlock_setIntgPd(rcb1, 5000);
            IedConnection_setRCBValues(con1, &error, rcb1, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_TRG_OPS | RCB_ELEMENT_INTG_PD, true);

            if (error != IED_ERROR_OK) {
                sprintf(log_msg_iecclient, "report(con1) activation failed (code: %i)\n", error);
                log(log_msg_iecclient);
            }
        }
    }
    else {
        sprintf(log_msg_iecclient, "Cannot connect to server1: IED_ERROR %d", error);
        log(log_msg_iecclient);
    }

    //printf("**************SETTING UP CRCB FOR IEDTWO***************\n");
    //second connection

    IedConnection con2 = IedConnection_create();
    int port2 = 11112;
    char *hostname2 = "192.168.194.129";

    IedConnection_connect(con2, &error, hostname2, port2);
    ClientReportControlBlock rcb2;

    if (error == IED_ERROR_OK) {
        rcb2 = IedConnection_getRCBValues(
                con2, &error, "IEDServer02LogicalDevice/LLN0.RP.Measurements01", NULL);
        if (error != IED_ERROR_OK) {
            sprintf(log_msg_iecclient, "Fail to get RCB2 values (code: %i)\n", error);
            log(log_msg_iecclient);
        }
        if (rcb2) {
            /* Install handler for reports */
            LinkedList dir2 = IedConnection_getDataSetDirectory(con2, &error, "IEDServer02LogicalDevice/LLN0$Measurements", NULL);
            IedConnection_installReportHandler(con2, "IEDServer02LogicalDevice/LLN0.RP.Measurements01", ClientReportControlBlock_getRptId(rcb2), reportCallbackFunction, dir2);

            /* Set trigger options and enable report */
            ClientReportControlBlock_setTrgOps(rcb2, TRG_OPT_DATA_UPDATE | TRG_OPT_INTEGRITY | TRG_OPT_GI);
            ClientReportControlBlock_setRptEna(rcb2, true);
            ClientReportControlBlock_setIntgPd(rcb2, 5000);
            IedConnection_setRCBValues(con2, &error, rcb2, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_TRG_OPS | RCB_ELEMENT_INTG_PD, true);

            if (error != IED_ERROR_OK) {
                sprintf(log_msg_iecclient, "report(con2) activation failed (code: %i)\n", error);
                log(log_msg_iecclient);
            }
        }
    }
    else {
        sprintf(log_msg_iecclient, "Cannot connect to server2: IED_ERROR %d", error);
        log(log_msg_iecclient);
    }

    //printf("***********************LOOPING***********************\n");
    //==============================================
    //   MAIN LOOP
    //==============================================
    while (run_iec61850) {
        //check for change in control values
        Thread_sleep(1000);
    }
    //printf("***********************CLEANING***********************\n");
    ClientReportControlBlock_destroy(rcb1);
    ClientReportControlBlock_destroy(rcb2);
    IedConnection_close(con1);
    IedConnection_close(con2);
}
