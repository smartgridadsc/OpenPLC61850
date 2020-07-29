#include <fstream>
#include <iostream>
#include <map>
#include <pthread.h>
#include <regex>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "hal_thread.h"
#include "iec61850_client.h"

#include "ladder.h"

#define CLIENTMAP_FILENAME "core/iecclient.map"

unsigned char log_msg_iecclient[1000];

struct ied_t {
    std::string ipaddr;
    std::vector<std::pair<std::string, std::string>> rcb_dataset_list; //report -> dataset
    std::unordered_map<std::string, IEC_BOOL> controlWatch;            //iec61850 da -> ctl value
};
typedef struct ied_t IED;

std::vector<IedConnection> conList;
std::vector<IED> iedlist;
std::unordered_map<std::string, std::string> mapping; //iec61850 da -> plc address

std::string trimFC(char *entryName)
{
    std::string object = "";

    for (int i = 0; i < strlen(entryName) && entryName[i] != '['; i++) {
        object += entryName[i];
    }

    return object;
}

void reportCallbackFunction(void *parameter, ClientReport report)
{
    //read from config to determine what element is of what type and writes to what
    MmsValue *dataSetValues = ClientReport_getDataSetValues(report);

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

                LinkedList entry = LinkedList_get(dataSetDirectory, i);

                char *entryName = (char *)entry->data;
                std::string object = trimFC(entryName);

                if (mapping.count(object)) {
                    write_to_address(value, mapping[object]);
                }
                else {
                    sprintf(log_msg_iecclient, "  Mapping not found for %s\n", object.c_str());
                    log(log_msg_iecclient);
                }
            }
        }
    }
    else {
        sprintf(log_msg_iecclient, "Datasetdirectory not found for %s\n", ClientReport_getRcbReference(report));
        log(log_msg_iecclient);
    }
}

void sendOperateCommand(int iedindex, std::string target, bool newVal)
{
    int temp = target.find('.');
    int cutoff = target.find('.', temp + 1);
    std::string trimmedtarget = target.substr(0, cutoff);

    IedConnection con = conList.at(iedindex);
    if (con == NULL) {
        printf("Connection had failed for this IED, not sending operate command\n");
        return;
    }

    ControlObjectClient control = ControlObjectClient_create(trimmedtarget.c_str(), con);

    if (!control) {
        sprintf(log_msg_iecclient, "Failed to create ControltObjectClient for %s\n", trimmedtarget.c_str());
        log(log_msg_iecclient);

        return;
    }

    MmsValue *ctlVal = MmsValue_newBoolean(newVal);

    ControlObjectClient_setOrigin(control, NULL, 3);

    if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */)) {
        sprintf(log_msg_iecclient, "%s operated successfully\n", trimmedtarget.c_str());
        log(log_msg_iecclient);
    }
    else {
        sprintf(log_msg_iecclient, "Failed to operate %s\n", trimmedtarget.c_str());
        log(log_msg_iecclient);
    }

    MmsValue_delete(ctlVal);

    ControlObjectClient_destroy(control);
}

void checkControlChanges()
{
    for (int i = 0; i < iedlist.size(); i++) {
        IED &ied = iedlist.at(i);
        for (auto &it : ied.controlWatch) {
            bool newvalue = read_bool(mapping[it.first]);

            if (newvalue != it.second) {
                it.second = newvalue;
                if (newvalue) {
                    sendOperateCommand(i, it.first, true);
                    sprintf(log_msg_iecclient, "Change in value detected (%s) = true\n", it.first.c_str());
                }
                else {
                    sendOperateCommand(i, it.first, false);
                    sprintf(log_msg_iecclient, "Change in value detected (%s) = false\n", it.first.c_str());
                }
                log(log_msg_iecclient);
            }
            else {
                //no change
            }
        }
    }
}

void process_mapping()
{
    std::ifstream mapfile(CLIENTMAP_FILENAME);
    if (!mapfile.is_open()) {
        sprintf(log_msg_iecclient, "Fail to open iecclient.map\n");
        log(log_msg_iecclient);
    }
    if (!mapfile.good()) {
        sprintf(log_msg_iecclient, "Mapfile is not good\n    failbit=%i\n     badbit=%i\n", mapfile.fail(), mapfile.bad());
        log(log_msg_iecclient);
    }

    std::string line;
    bool iedconfigDone = false;
    bool inIed = false;

    //Get Ied details (IP addr, Report ref, dataset ref)
    std::regex ipregex("[0-9]+.[0-9]+.[0-9]+.[0-9]+");
    IED newied;
    while (!mapfile.eof() && !iedconfigDone) {
        std::getline(mapfile, line);

        if (line.empty()) {
            iedlist.push_back(newied);
            iedconfigDone = true;
        }
        else if (std::regex_match(line, ipregex)) {
            if (inIed) {
                iedlist.push_back(newied);
                IED temp;
                newied = temp;
                newied.ipaddr = line;
            }
            else {
                newied.ipaddr = line;
                inIed = true;
            }
        }
        else {
            std::stringstream ss(line);
            std::string token;
            std::pair<std::string, std::string> p;
            std::getline(ss, token, ' ');
            p.first = token;
            std::getline(ss, token, ' ');
            p.second = token;

            if (p.second.at(0) == '%') {
                newied.controlWatch[p.first] = false;
            }
            else {
                newied.rcb_dataset_list.push_back(p);
            }
        }
    }

    //Get iec61850 object to plc addr mapping
    while (!mapfile.eof()) {
        std::getline(mapfile, line);
        std::stringstream ss(line);
        std::string token;

        std::getline(ss, token, ' ');
        std::string type = token;

        std::getline(ss, token, ' ');
        std::string var = token;

        std::getline(ss, token, ' ');
        std::string addr = token;

        if (type.compare("CONTROL") == 0) {
        }

        if (addr.compare("X") != 0) {
            mapping[var] = addr;
        }
        else {
            sprintf(log_msg_iecclient, "Invalid mapping for %s\n", var.c_str());
            log(log_msg_iecclient);
        }
    }
}

void run_iec61850_client()
{
    sprintf(log_msg_iecclient, "Starting IEC61850CLIENT\n");
    log(log_msg_iecclient);

    //==============================================
    //   PROCESS MAPPING FILE
    //==============================================
    process_mapping();

    Thread_sleep(1000);

    IedClientError error;
    int port = 102;

    for (int i = 0; i < iedlist.size(); i++) {
        IED &ied = iedlist.at(i);
        sprintf(log_msg_iecclient, "For %s:\n", ied.ipaddr.c_str());
        log(log_msg_iecclient);
        IedConnection con = IedConnection_create();
        IedConnection_connect(con, &error, ied.ipaddr.c_str(), port);

        if (error != IED_ERROR_OK) {
            sprintf(log_msg_iecclient, "    Failed to connect to %s\n", ied.ipaddr.c_str());
            log(log_msg_iecclient);
            conList.push_back(NULL);
            continue;
        }

        ClientReportControlBlock rcb;
        bool isConnected = false;
        if (ied.rcb_dataset_list.size() == 0) {
            sprintf(log_msg_iecclient, "    No RCB in mappingfile\n");
            log(log_msg_iecclient);
        }

        for (int j = 0; j < ied.rcb_dataset_list.size() && !isConnected; j++) {
            std::pair<std::string, std::string> &report_dataset = ied.rcb_dataset_list.at(i);

            rcb = IedConnection_getRCBValues(con, &error, report_dataset.first.c_str(), NULL);
            if (error != IED_ERROR_OK) {
                sprintf(log_msg_iecclient, "    Fail to get RCB[%i] values (code: %i)\n", j, error);
                log(log_msg_iecclient);
                continue;
            }

            LinkedList dir = IedConnection_getDataSetDirectory(con, &error, report_dataset.second.c_str(), NULL);
            if (error != IED_ERROR_OK) {
                sprintf(log_msg_iecclient, "    Fail to get Dataset Dir[%i] (code: %i)\n", j, error);
                log(log_msg_iecclient);
                continue;
            }

            IedConnection_installReportHandler(con, report_dataset.first.c_str(), ClientReportControlBlock_getRptId(rcb), reportCallbackFunction, dir);

            /* Set trigger options and enable report */
            ClientReportControlBlock_setTrgOps(rcb, TRG_OPT_DATA_UPDATE | TRG_OPT_INTEGRITY | TRG_OPT_GI);
            ClientReportControlBlock_setRptEna(rcb, true);
            ClientReportControlBlock_setIntgPd(rcb, 5000);
            IedConnection_setRCBValues(con, &error, rcb, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_TRG_OPS | RCB_ELEMENT_INTG_PD, true);

            if (error != IED_ERROR_OK) {
                sprintf(log_msg_iecclient, "    Report activation[%i] failed (code: %i)\n", error);
                log(log_msg_iecclient);
                continue;
            }

            isConnected = true;

            ClientReportControlBlock_destroy(rcb);
        }

        if (isConnected) {
            sprintf(log_msg_iecclient, "    IED setup success\n");
            log(log_msg_iecclient);
            conList.push_back(con);
        }
        else {
            sprintf(log_msg_iecclient, "Failed to setup reporting\n");
            log(log_msg_iecclient);
            conList.push_back(NULL);
        }
    }

    //==============================================
    //   MAIN LOOP
    //==============================================
    while (run_iec61850) {
        //check for change in control values
        Thread_sleep(100);
        checkControlChanges();
    }

    for (auto con : conList) {
        IedConnection_close(con);
    }
}
