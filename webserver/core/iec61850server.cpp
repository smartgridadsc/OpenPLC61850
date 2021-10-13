//-----------------------------------------------------------------------------
// Copyright 2021 ADSC
//
// This file is part of the OpenPLC61850 Software Stack.
//
// OpenPLC61850 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenPLC61850 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenPLC61850.  If not, see <http://www.gnu.org/licenses/>.
//------
//
// This file contains the IEC61850 server component. The server
// component is responsible for handling commands received from
// SCADA.

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <vector>
#include <fstream>
#include <sstream>

#include "hal_thread.h"
#include "static_model.h"
#include "iec61850_server.h"

#include "ladder.h"

#define OPLC_CYCLE  50000000

#define SERVERMAP_FILENAME "core/iecserver.map"

unsigned char log_msg_iecserver[1000];

IedServer iedServer = NULL;

std::vector<std::string> control_nodes;
std::vector<std::string> monitor_nodes;
std::unordered_map<std::string, std::string> serverside_mapping; //iec61850 da -> plc address

static ControlHandlerResult
controlHandler(ControlAction action, void* parameter, MmsValue* value, bool test) {
    if (test) {
        return CONTROL_RESULT_FAILED;
    }

    std::string &node_string = *(static_cast<std::string*>(parameter));

    sprintf(log_msg_iecserver, "ControlHandler called for %s\n", node_string.c_str());
    log(log_msg_iecserver);

    write_to_address(value, serverside_mapping[node_string]);
}

void update_server() {
    IedServer_lockDataModel(iedServer);
    bool isSupported;
    for (std::string &da_string : monitor_nodes) {
        DataAttribute* da = (DataAttribute *) IedModel_getModelNodeByObjectReference(&iedModel, da_string.c_str());
        if (da == NULL) {
            sprintf(log_msg_iecserver, "DataAttribute not found for %s\n", da_string.c_str());
            log(log_msg_iecserver);
            continue;
        }
        
        if (!serverside_mapping.count(da_string)) {
            sprintf(log_msg_iecserver, "Mapping not found for %s\n", da_string.c_str());
            log(log_msg_iecserver);
            continue;
        }
        std::string address = serverside_mapping[da_string];

        MmsValue* value;
        isSupported = true;
        switch(da->type) {
            case IEC61850_BOOLEAN:
            {
                bool temp = read_bool(address);
                value = MmsValue_newBoolean(temp);
                break;
            }
            case IEC61850_INT16:
            {
                int16_t temp = read_int16(address);
                value = MmsValue_newIntegerFromInt16(temp);
                break;
            }
            case IEC61850_INT32:
            {
                int32_t temp = read_int32(address);
                value = MmsValue_newIntegerFromInt32(temp);
                break;
            }
            case IEC61850_INT64:
            {
                int64_t temp = read_int64(address);
                value = MmsValue_newIntegerFromInt64(temp);
                break;
            }
            case IEC61850_INT16U:
            {
                uint16_t temp = read_uint16(address);
                value = MmsValue_newUnsigned(temp);
                break;
            }
            case IEC61850_INT32U:
            {
                uint32_t temp = read_uint32(address);
                value = MmsValue_newUnsignedFromUint32(temp);
                break;
            }
            case IEC61850_FLOAT32:
            {
                float temp = read_float(address);
                value = MmsValue_newFloat(temp);
                break;
            }
            case IEC61850_FLOAT64:
            {
                double temp = read_double(address);
                value = MmsValue_newDouble(temp);
                break;
            }
            default:
            {   
                sprintf(log_msg_iecserver, "Unsupported DA type(%i) for %s\n", da->type, da_string.c_str());
                log(log_msg_iecserver);
                isSupported = false;
                break;
            }  
        }
        
        if (isSupported) IedServer_updateAttributeValue(iedServer, da, value);
    }

    IedServer_unlockDataModel(iedServer);
}

void set_control_handlers() {
    for (std::string &node_string : control_nodes) {
        sprintf(log_msg_iecserver, "Setting control handler for %s\n", node_string.c_str());
        log(log_msg_iecserver);

        DataObject* data_object = (DataObject *) IedModel_getModelNodeByObjectReference(&iedModel, node_string.c_str());

        IedServer_setControlHandler(iedServer, data_object, (ControlHandler) controlHandler, static_cast<void*>(&node_string));
    }
}

void process_server_mapping() {

    std::ifstream mapfile(SERVERMAP_FILENAME);
    if (!mapfile.is_open()) {
        sprintf(log_msg_iecserver, "Fail to open iecserver.map\n");
        log(log_msg_iecserver);
        return;
    }
    if (!mapfile.good()) {
        sprintf(log_msg_iecserver, "Server Mapfile is not good\n    failbit=%i\n     badbit=%i\n", mapfile.fail(), mapfile.bad());
        log(log_msg_iecserver);
        return;
    }
    
    std::string line;
    std::string type, reference, address;
    while(!mapfile.eof()) {
        std::getline(mapfile, line);

        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ' ');
        type = token;
        std::getline(ss, token, ' ');
        reference = token;
        std::getline(ss, token, ' ');
        address = token;
        
        if (type.compare("MONITOR") == 0) {
            monitor_nodes.push_back(reference);
        }
        else if (type.compare("CONTROL") == 0) {
            int index = reference.find(".");
            int cutoff = reference.find(".", index+1);
            reference = reference.substr(0, cutoff);
            control_nodes.push_back(reference);
        }

        serverside_mapping[reference] = address;
    }

    /*
    sprintf(log_msg_iecserver, "MONITOR NODES:\n");
    log(log_msg_iecserver);
    for (std::string &str : monitor_nodes) {
        sprintf(log_msg_iecserver, "%s\n", str.c_str());
        log(log_msg_iecserver);
    }

    sprintf(log_msg_iecserver, "CONTROL NODES:\n");
    log(log_msg_iecserver);
    for (std::string &str : control_nodes) {
        sprintf(log_msg_iecserver, "%s\n", str.c_str());
        log(log_msg_iecserver);
    }
    */

    printf("mapping done\n");
}


void startIec61850Server(int port) 
{
    sprintf(log_msg_iecserver, "Starting IEC61850SERVER\n");
    log(log_msg_iecserver);
    
    iedServer = IedServer_create(&iedModel);

    process_server_mapping();

    IedServer_setWriteAccessPolicy(iedServer, IEC61850_FC_ALL, ACCESS_POLICY_ALLOW);

    set_control_handlers();

    //start server
    IedServer_start(iedServer, port);

    if (!IedServer_isRunning(iedServer)) {
        sprintf(log_msg_iecserver, "Starting IEC61850 Server failed! (Ensure sudo permissions)\n");
        log(log_msg_iecserver);
        IedServer_destroy(iedServer);
        exit(-1);
    }

    //==============================================
    //   MAIN LOOP
    //==============================================
    struct timespec timer_start;
    clock_gettime(CLOCK_MONOTONIC, &timer_start);
    
    while(run_iec61850) {
        update_server();
        //sleep_until(&timer_start, OPLC_CYCLE);
        Thread_sleep(500);
    }

    //clean up
    IedServer_stop(iedServer);
    IedServer_destroy(iedServer);
}
