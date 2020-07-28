#include <string>
#include <sstream>

#include "iec61850_client.h"
#include "ladder.h"

#define MAX_BUFFER_SIZE 1024

struct address_t {
    std::string buffertype;
    std::pair<int,int> location;
};

typedef struct address_t ADDRESS;

unsigned char log_msg_iecrw[1000];

ADDRESS getAddress(std::string address) {
    ADDRESS addr;
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

IEC_BOOL read_bool(std::string address) {
    ADDRESS addr = getAddress(address);
    IEC_BOOL readvalue  = false;
    
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
        sprintf(log_msg_iecrw, "Read address not found %s\n", address.c_str());
        log(log_msg_iecrw);
    }
    pthread_mutex_unlock(&bufferLock);

    return readvalue;
}

/*
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
    ADDRESS addr = getAddress(address);
    
    pthread_mutex_lock(&bufferLock);
    if (addr.buffertype.compare("%QX") == 0) {
        IEC_BOOL val = (IEC_BOOL) MmsValue_getBoolean(mmsval);
        if(bool_output[addr.location.first][addr.location.second] != NULL) {
            *bool_output[addr.location.first][addr.location.second] = val;
        }
    }
    else if (addr.buffertype.compare("%IX") == 0) {
        IEC_BOOL val = (IEC_BOOL) MmsValue_getBoolean(mmsval);
        if(bool_input[addr.location.first][addr.location.second] != NULL) {
            *bool_input[addr.location.first][addr.location.second] = val;
        }
    }
    else if (addr.buffertype.compare("%IW") == 0) {
        //16 bit not supported by libiec?
    }
    else if (addr.buffertype.compare("%QW") == 0) {
        //16 bit not supported by libiec?
    }
    else if (addr.buffertype.compare("%MW") == 0) {
        //16 bit not supported by libiec?
    }
    else if (addr.buffertype.compare("%MD") == 0) {
        if (type == MMS_INTEGER) {
            IEC_DINT val = (IEC_DINT) MmsValue_toInt32(mmsval);
            if (dint_memory[addr.location.first] != NULL) {
                *dint_memory[addr.location.first] = val;
            }
        }
        else if (type == MMS_FLOAT) {
            float flt = MmsValue_toFloat(mmsval);
            if (dint_memory[addr.location.first] != NULL) {
                *dint_memory[addr.location.first] = *(reinterpret_cast<int32_t *>(&flt));
            }
        }   
    }
    else if (addr.buffertype.compare("%ML") == 0) {
        //not done yet
    }
    else {
        sprintf(log_msg_iecrw, "Write address not found %s\n", address.c_str());
        log(log_msg_iecrw);
        return 1;
    }
    pthread_mutex_unlock(&bufferLock);

    return 0;
}
