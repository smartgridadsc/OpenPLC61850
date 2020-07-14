#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <string>

#include "hal_thread.h"
#include "iec61850_client.h"

#include "ladder.h"

unsigned char log_msg_iecclient[1000];

void reportCallbackFunction(void *parameter, ClientReport report)
{
    MmsValue *dataSetValues = ClientReport_getDataSetValues(report);

    int location = 20;

    std::string reportref = ClientReport_getRcbReference(report);
    if (reportref.compare("IEDServer01LogicalDevice/LLN0.RP.Measurements01") == 0) {
        location = 0;
    }
    else if (reportref.compare("IEDServer02LogicalDevice/LLN0.RP.Measurements01") == 0){
        location = 10;
    }
    
    MmsValue* magf = MmsValue_getElement(dataSetValues, 0);
    *dint_memory[location] = (IEC_DINT*) MmsValue_toFloat(magf);
    MmsValue_delete(magf);

    MmsValue* q = MmsValue_getElement(dataSetValues, 1);
    *dint_memory[location+1] = (IEC_DINT*) MmsValue_toFloat(q);
    MmsValue_delete(q);
    /*
    sprintf(log_msg_iecclient, "received report for %s with rptId %s\n",
           ClientReport_getRcbReference(report), ClientReport_getRptId(report));
    log(log_msg_iecclient);

    if (ClientReport_hasTimestamp(report)) {
        time_t unixTime = ClientReport_getTimestamp(report) / 1000;

#ifdef WIN32
        char *timeBuf = ctime(&unixTime);
#else
        char timeBuf[30];
        ctime_r(&unixTime, timeBuf);
#endif

        sprintf(log_msg_iecclient, "  report contains timestamp (%u): %s", (unsigned int)unixTime, timeBuf);
        log(log_msg_iecclient);
    }

    for (int i = 0; i < MmsValue_getArraySize(dataSetValues); i++) {
        ReasonForInclusion reason = ClientReport_getReasonForInclusion(report, i);

        char valBuffer[500];
        sprintf(valBuffer, "no value");
        MmsValue *value = MmsValue_getElement(dataSetValues, i);
        if (value) {
                MmsValue_printToBuffer(value, valBuffer, 500);
        }
        sprintf(log_msg_iecclient, "  DataSetEntry[%d] (included for reason %s): %s\n", i, ReasonForInclusion_getValueAsString(reason), valBuffer);
        log(log_msg_iecclient);
    }
    printf("\n");
    */
}

void run_iec61850_client()
{
    IedClientError error;

    IedConnection con1 = IedConnection_create();
    int port1 = 11111;
    char* hostname1 = "192.168.194.129";

    IedConnection_connect(con1, &error, hostname1, port1);
    ClientReportControlBlock rcb;

    if (error == IED_ERROR_OK) {
        //printf("**************SETTING UP CRCB FOR IEDONE***************\n");

        /* Read RCB values */
        rcb = IedConnection_getRCBValues(
                con1, &error, "IEDServer01LogicalDevice/LLN0.RP.Measurements01", NULL);
        if (rcb) {
            /* Install handler for reports */
            IedConnection_installReportHandler(con1, "IEDServer01LogicalDevice/LLN0.RP.Measurements01", ClientReportControlBlock_getRptId(rcb), reportCallbackFunction, NULL);

            /* Set trigger options and enable report */
            ClientReportControlBlock_setTrgOps(rcb, TRG_OPT_DATA_UPDATE | TRG_OPT_INTEGRITY | TRG_OPT_GI);
            ClientReportControlBlock_setRptEna(rcb, true);
            ClientReportControlBlock_setIntgPd(rcb, 5000);
            IedConnection_setRCBValues(con1, &error, rcb, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_TRG_OPS | RCB_ELEMENT_INTG_PD, true);

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
    
    IedConnection con2 = IedConnection_create();
    int port2 = 11112;
    char* hostname2 = "192.168.194.129";

    IedConnection_connect(con2, &error, hostname2, port2);
    ClientReportControlBlock rcb2;
    
    //printf("**************SETTING UP CRCB FOR IEDTWO***************\n");
    //second connection

    if (error != IED_ERROR_OK) {
        rcb2 = IedConnection_getRCBValues(
                con2, &error, "IEDServer02LogicalDevice/LLN0.RP.Measurements01", NULL);
        if (rcb2) {
            /* Install handler for reports */
            IedConnection_installReportHandler(con2, "IEDServer02LogicalDevice/LLN0.RP.Measurements01", ClientReportControlBlock_getRptId(rcb2), reportCallbackFunction, NULL);

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
        sprintf(log_msg_iecclient, "Cannot connect to server1: IED_ERROR %d", error);
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
    ClientReportControlBlock_destroy(rcb);
    ClientReportControlBlock_destroy(rcb2);
    IedConnection_close(con1);
    IedConnection_close(con2);
}
