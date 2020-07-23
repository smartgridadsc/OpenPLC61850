#include <string>
#include <sstream>

#include "iec61850_client.h"
#include "ladder.h"

#define MAX_BUFFER_SIZE 1024

unsigned char log_msg_iecrw[1000];
/*
IEC_BOOL read_bool(std::string address);
IEC_UINT read_int16(std::string address);
IEC_DINT read_int32(std::string address);
IEC_LINT read_int64(std::string address);

int write_bool(std::string address, IEC_BOOL value);
int write_int16(std::string address, IEC_UINT value);
int write_int32(std::string address, IEC_DINT value);
int write_int64(std::string address, IEC_LINT value);
*/
/*
    Writes the given MmsValue to the specified address.
    Returns 0 if successful, returns 0 if failed.
*/
int write_to_address(MmsValue* mmsval, std::string address) {
    MmsType type = MmsValue_getType(mmsval);
    std::string buffertype = address.substr(0,3);
    std::pair<int,int> location;

    address = address.substr(3,10);
    std::stringstream ss(address);
    
    if (address.find('.', 0) == std::string::npos) {
        location.first = std::stoi(address);
        location.second = -1;
    }
    else {
        std::string token;
        std::getline(ss, token, '.');
        location.first = std::stoi(token);
        std::getline(ss, token, '.');
        location.second = std::stoi(token);
    }
    
    pthread_mutex_lock(&bufferLock);
    if (buffertype.compare("%QX") == 0) {
        IEC_BOOL val = (IEC_BOOL) MmsValue_getBoolean(mmsval);
        if(bool_output[location.first][location.second] != NULL) {
            *bool_output[location.first][location.second] = val;
        }
    }
    else if (buffertype.compare("%IX") == 0) {
        IEC_BOOL val = (IEC_BOOL) MmsValue_getBoolean(mmsval);
        if(bool_input[location.first][location.second] != NULL) {
            *bool_input[location.first][location.second] = val;
        }
    }
    else if (buffertype.compare("%IW") == 0) {
        //16 bit not supported by libiec?
    }
    else if (buffertype.compare("%QW") == 0) {
        //16 bit not supported by libiec?
    }
    else if (buffertype.compare("%MW") == 0) {
        //16 bit not supported by libiec?
    }
    else if (buffertype.compare("%MD") == 0) {
        if (type == MMS_INTEGER) {
            IEC_DINT val = (IEC_DINT) MmsValue_toInt32(mmsval);
            if (dint_memory[location.first] != NULL) {
                *dint_memory[location.first] = val;
            }
        }
        else if (type == MMS_FLOAT) {
            float flt = MmsValue_toFloat(mmsval);
            if (dint_memory[location.first] != NULL) {
                *dint_memory[location.first] = *(reinterpret_cast<int32_t *>(&flt));
            }
        }   
    }
    else if (buffertype.compare("%ML") == 0) {
        //not done yet
    }
    else {
        return 1;
    }
    pthread_mutex_unlock(&bufferLock);

    return 0;
}
