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
// This file is the read/write utility file for IEC 61850 server 
// and client components. The functions are used to read/write to 
// PLC memory.
//-----------------------------------------------------------------------------

#include <string>
#include <sstream>

#include "ladder.h"
#include "static_model.h"
#include "iec61850_model.h"

#define MAX_BUFFER_SIZE 1024

extern IedModel iedModel;

struct address_t {
    std::string buffertype;
    std::pair<int,int> location;
};

typedef struct address_t ADDRESS;

unsigned char log_msg_iecrw[1000];

ADDRESS getAddress(std::string address) {
    ADDRESS addr;
    if (address.length() < 3) {
	return {"error", {-1,-1}};
    }

    addr.buffertype = address.substr(0,3);

    address = address.substr(3,10);
    std::stringstream ss(address);
    
    if (address.find('.', 0) == std::string::npos) {
        addr.location.first = std::stoi(address);
        addr.location.second = -1;
    }
    else {
        std::string token;
        std::getline(ss, token, '.');
        addr.location.first = std::stoi(token);
        std::getline(ss, token, '.');
        addr.location.second = std::stoi(token);
    }

    return addr;
}

bool read_bool(std::string address) {
    ADDRESS addr = getAddress(address);
    bool readvalue = false;
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%QX") == 0) {
        if(bool_output[addr.location.first][addr.location.second] != NULL) {
            readvalue = *bool_output[addr.location.first][addr.location.second];
        }
    }
    else if (addr.buffertype.compare("%IX") == 0) {
        if(bool_input[addr.location.first][addr.location.second] != NULL) {
            readvalue = *bool_input[addr.location.first][addr.location.second];
        }
    }
    else {
        sprintf(log_msg_iecrw, "read_bool(): Read address not found/not compatible with bool %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

int16_t read_int16(std::string address) {
    ADDRESS addr = getAddress(address);
    int16_t readvalue = 0;
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%QW") == 0) {
        if(int_output[addr.location.first] != NULL) {
            readvalue = *((int16_t*) int_output[addr.location.first]);
        }
    }
    else if (addr.buffertype.compare("%IW") == 0) {
        if(int_input[addr.location.first] != NULL) {
            readvalue = *((int16_t*) int_input[addr.location.first]);
        }
    }
    else if (addr.buffertype.compare("%MW") == 0) {
        if(int_memory[addr.location.first] != NULL) {
            readvalue = *((int16_t*) int_memory[addr.location.first]);
        }
    }
    else {
        sprintf(log_msg_iecrw, "read_int16(): Read address not found/not compatible with int16 %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

int32_t read_int32(std::string address) {
    ADDRESS addr = getAddress(address);
    int32_t readvalue = 0;
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%MD") == 0) {
        if(dint_memory[addr.location.first] != NULL) {
            readvalue = *((int32_t*) dint_memory[addr.location.first]);
        }
    }
    else {
        sprintf(log_msg_iecrw, "read_int32(): Read address not found/not compatible with int32 %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

int64_t read_int64(std::string address) {
    ADDRESS addr = getAddress(address);
    int64_t readvalue = 0;
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%ML") == 0) {
        if(lint_memory[addr.location.first] != NULL) {
            readvalue = *((int64_t*) lint_memory[addr.location.first]);
        }
    }
    else {
        sprintf(log_msg_iecrw, "read_int64(): Read address not found/not compatible with int64 %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

uint16_t read_uint16(std::string address) {
    ADDRESS addr = getAddress(address);
    uint16_t readvalue = 0;
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%QW") == 0) {
        if(int_output[addr.location.first] != NULL) {
            readvalue = *(int_output[addr.location.first]);
        }
    }
    else if (addr.buffertype.compare("%IW") == 0) {
        if(int_input[addr.location.first] != NULL) {
            readvalue = *(int_input[addr.location.first]);
        }
    }
    else if (addr.buffertype.compare("%MW") == 0) {
        if(int_memory[addr.location.first] != NULL) {
            readvalue = *(int_memory[addr.location.first]);
        }
    }
    else {
        sprintf(log_msg_iecrw, "read_uint16(): Read address not found/not compatible with uint16 %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

uint32_t read_uint32(std::string address) {
    ADDRESS addr = getAddress(address);
    uint32_t readvalue = 0;
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%MD") == 0) {
        if(dint_memory[addr.location.first] != NULL) {
            readvalue = *(dint_memory[addr.location.first]);
        }
    }
    else {
        sprintf(log_msg_iecrw, "read_uint32(): Read address not found/not compatible with uint32 %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

float read_float(std::string address) {
    ADDRESS addr = getAddress(address);
    float readvalue = 0;
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%MD") == 0) {
        if(dint_memory[addr.location.first] != NULL) {
            readvalue = *((float*) dint_memory[addr.location.first]);
        }
    }
    else {
        sprintf(log_msg_iecrw, "read_float(): Read address not found/not compatible with float %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

double read_double(std::string address) {
    ADDRESS addr = getAddress(address);
    double readvalue = 0;
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%ML") == 0) {
        if(lint_memory[addr.location.first] != NULL) {
            readvalue = *((double*) lint_memory[addr.location.first]);
        }
    }
    else {
        sprintf(log_msg_iecrw, "read_double(): Read address not found/not compatible with double %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

/*
    Writes the given MmsValue to the specified address.
    Returns 0 if successful, returns 0 if failed.
*/

int write_to_address(MmsValue* mmsval, std::string address) {
    MmsType type = MmsValue_getType(mmsval);
    ADDRESS addr = getAddress(address);
    //sprintf(log_msg_iecrw, "TEMP DEBUG Write address %s\n", address.c_str());
    //log(log_msg_iecrw);
    //debug
    /*
    if (type == MMS_INTEGER) {
        sprintf(log_msg_iecrw, "TEMP DEBUG MMS_INTEGER\n");
        log(log_msg_iecrw);
    }
    else if (type == MMS_FLOAT) {
        sprintf(log_msg_iecrw, "TEMP DEBUG MMS_FLOAT\n");
        log(log_msg_iecrw);
    }
    else if (type == MMS_BOOLEAN) {
        sprintf(log_msg_iecrw, "TEMP DEBUG MMS_BOOLEAN\n");
        log(log_msg_iecrw);
    }
    else {
        sprintf(log_msg_iecrw, "TEMP DEBUG MMS_OTHER\n");
        log(log_msg_iecrw);
    }
    */
    //end-debug

    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%QX") == 0) { //Output Coils %QX0.0 - %QX99.7
        IEC_BOOL val;
        if (type == MMS_BOOLEAN){

            val = (IEC_BOOL) MmsValue_getBoolean(mmsval);

        }else{
            //here, assume it is stVal (80 is true, 40 is false)
            int value = MmsValue_getBitStringAsInteger(mmsval);
            //sprintf(log_msg_iecrw, "TEMP DEBUG QX: %d\n", value);
            //log(log_msg_iecrw);

            //TODO Seems like the value is 2 when false, 1 when true
            if(value ==1){
                //sprintf(log_msg_iecrw, "TEMP DEBUG CB STATUS TRUE\n");
                //log(log_msg_iecrw);
                val = true;
            }else{
                //sprintf(log_msg_iecrw, "TEMP DEBUG CB STATUS FALSE\n");
                //log(log_msg_iecrw);
                val = false;
            }

        }
        //IEC_BOOL val = (IEC_BOOL) MmsValue_getBoolean(mmsval);
        if(bool_output[addr.location.first][addr.location.second] != NULL) {
            *bool_output[addr.location.first][addr.location.second] = val;
        }
    }
    else if (addr.buffertype.compare("%IX") == 0) { //Input Contacts %IX0.0 - %IX99.7
        IEC_BOOL val = (IEC_BOOL) MmsValue_getBoolean(mmsval);
        if(bool_input[addr.location.first][addr.location.second] != NULL) {
            *bool_input[addr.location.first][addr.location.second] = val;
        }
    }
    else if (addr.buffertype.compare("%IW") == 0) { //Input Registers %IW0 - %IW1023
        //16 bit not supported by libiec?
        if (type == MMS_INTEGER) {
            int16_t val = MmsValue_toInt32(mmsval);
            if (int_input[addr.location.first] != NULL) {
                *int_input[addr.location.first] = *((IEC_UINT*) &val);
            }
        }
        else if (type == MMS_UNSIGNED) {
            //16 bit unsigned has no specific function
            uint16_t val = MmsValue_toUint32(mmsval);
            if (int_input[addr.location.first] != NULL) {
                *int_input[addr.location.first] = val;
            }
        } 
    }
    else if (addr.buffertype.compare("%QW") == 0) { //Holding Registers %QW0 - %QW1023
        if (type == MMS_INTEGER) {
            int16_t val = MmsValue_toInt32(mmsval);
            if (int_output[addr.location.first] != NULL) {
                *int_output[addr.location.first] = *((IEC_UINT*) &val);
            }
        }
        else if (type == MMS_UNSIGNED) {
            //16 bit unsigned has no specific function
            uint16_t val = MmsValue_toUint32(mmsval);
            if (int_output[addr.location.first] != NULL) {
                *int_output[addr.location.first] = val;
            }
        } 
    }
    else if (addr.buffertype.compare("%MW") == 0) { //General 16bit Register %Mw0 - %MW1023
        if (type == MMS_INTEGER) {
            int16_t val = MmsValue_toInt32(mmsval);
            if (int_memory[addr.location.first] != NULL) {
                *int_memory[addr.location.first] = *((IEC_UINT*) &val);
            }
        }
        else if (type == MMS_UNSIGNED) {
            //16 bit unsigned has no specific function
            uint16_t val = MmsValue_toUint32(mmsval);
            if (int_memory[addr.location.first] != NULL) {
                *int_memory[addr.location.first] = val;
            }
        }  
    }
    else if (addr.buffertype.compare("%MD") == 0) { //General 32bit Register %MD0 - %MD1023
        if (type == MMS_INTEGER) {
            int32_t val = MmsValue_toInt32(mmsval);
            if (dint_memory[addr.location.first] != NULL) {
                *dint_memory[addr.location.first] = val;

                //sprintf(log_msg_iecrw, "TEMP DEBUG INT VALUE IS UPDATED: %d\n", *dint_memory[addr.location.first]);
                //log(log_msg_iecrw);

            }
        }
        else if (type == MMS_UNSIGNED) {
            uint32_t val = MmsValue_toUint32(mmsval);
            if (dint_memory[addr.location.first] != NULL) {
                *dint_memory[addr.location.first] = *((IEC_DINT*) &val);
            }
        }  
        else if (type == MMS_FLOAT) {
            float flt = MmsValue_toFloat(mmsval);
            if (dint_memory[addr.location.first] != NULL) {
                *dint_memory[addr.location.first] = *((IEC_DINT*) &flt);
            }
        }   
    }
    else if (addr.buffertype.compare("%ML") == 0) { //General 64bit Register %ML0 - %ML1023
        if (type == MMS_INTEGER) {
            IEC_LINT val = (IEC_LINT) MmsValue_toInt64(mmsval);
            if (lint_memory[addr.location.first] != NULL) {
                *lint_memory[addr.location.first] = val;
            }
        }
        //64 bit unsigned has not supported
        else if (type == MMS_FLOAT) {
            double db = MmsValue_toDouble(mmsval);
            if (lint_memory[addr.location.first] != NULL) {
                *lint_memory[addr.location.first] = *((IEC_LINT*) &db);
            }
        }
    }
    else {
        sprintf(log_msg_iecrw, "Write address not found %s\n", address.c_str());
        log(log_msg_iecrw);
        return 1;
    }
    pthread_mutex_unlock(&bufferLock);

    return 0;
}
