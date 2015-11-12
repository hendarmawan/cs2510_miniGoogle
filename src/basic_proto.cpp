#include "basic_proto.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "rpc_log.h"

basic_proto::basic_proto() {
    m_encoded_len = 0;
    m_buf.resize(32);
}

basic_proto::basic_proto(const char* buf, int buf_len){
    m_buf.assign(buf,buf_len);
    m_encoded_len = 0 ;
}

basic_proto::~basic_proto() {
}

void basic_proto::add_float(float retval){
    while (m_encoded_len + sizeof(retval) > m_buf.size()){
        m_buf.resize(m_buf.size()*2);
    }
    *(float*) (m_buf.data() + m_encoded_len) = retval;
    m_encoded_len += sizeof(float);
}

int basic_proto::read_float(float &retval){
    if ((m_encoded_len + sizeof(float)) > m_buf.size()){
        return -1;
    }
    retval = *(float*)((char*)m_buf.data() + m_encoded_len);
    m_encoded_len += sizeof(float);
    return 0;
}
void basic_proto::add_double(double retval){
    while (m_encoded_len + sizeof(retval) > m_buf.size()){
        m_buf.resize(m_buf.size()*2);
    }
    *(double*) (m_buf.data() + m_encoded_len) = retval;
    m_encoded_len += sizeof(double);
}

int basic_proto::read_double(double &retval){
    if ((m_encoded_len + sizeof(double)) > m_buf.size()){
        return -1;
    }
    retval = *(double*)((char*)m_buf.data() + m_encoded_len);
    m_encoded_len += sizeof(double);
    return 0;
}

int basic_proto::read_binary(int bin_len, char* &bin){
    if((m_encoded_len + bin_len) > m_buf.size()){
        return -1;
    }
    bin = (char*)m_buf.data() + m_encoded_len;
    m_encoded_len += bin_len;
    return 0;
}

void basic_proto::add_binary(const char * bin, int bin_len){
    while (m_encoded_len + bin_len >m_buf.size()){
        m_buf.resize(m_buf.size()*2);
    }
    memcpy((char*)m_buf.data() + m_encoded_len, bin, bin_len);
    m_encoded_len +=bin_len;
}

void basic_proto::add_int(int retval){
    while (m_encoded_len + sizeof(retval) > m_buf.size()){
        m_buf.resize(m_buf.size()*2);
    }
    *(int*) (m_buf.data() + m_encoded_len) = htonl(retval);
    m_encoded_len += sizeof(int);
}

int basic_proto::read_int(int &retval){
    if ((m_encoded_len + sizeof(int)) > m_buf.size()){
        return -1;
    }
    retval = *(int*)((char*)m_buf.data() + m_encoded_len);
    retval = ntohl(retval);
    m_encoded_len += sizeof(int);
    return 0;
}

void basic_proto::add_array(int *data, int size) {
    add_int(size);
    //add_binary((const char*)data, sizeof(int) * size);
    for(int i = 0; i < size; i ++){
        add_int(data[i]);
    }
}

int basic_proto::read_array(int *&data, int &size) {
    if (read_int(size) < 0) {
        return -1;
    }
    if (m_buf.size() < size * sizeof(int) + m_encoded_len) {
        return -1;
    }
    char *num = (char*)(m_buf.data() + m_encoded_len);
    for (int i = 0; i < size; ++i, num += sizeof(int)) {
        *(int*)num = ntohl(*(int*)num);
    }
    data = (int*)(m_buf.data() + m_encoded_len);
    m_encoded_len += sizeof(int) * size;
    return 0;
}

void basic_proto::add_matrix(int *data, int row, int col) {
    add_int(row);
    add_int(col);
    //add_binary((const char*)data, sizeof(int) * row * col);
    for(int i = 0;i<row;i++){
        for(int j=0;j<col;j++){
            add_int(data[i*col+j]);
        }
    }
}

int basic_proto::read_matrix(int *&data, int &row, int &col) {
    if (read_int(row) < 0) {
        return -1;
    }
    if (read_int(col) < 0) {
        return -1;
    }
    if(m_buf.size() < row*col*sizeof(int)+m_encoded_len){
        return -1;
    }
    char*num = (char*)(m_buf.data() + m_encoded_len);
    for(int i=0;i<row;i++){
        for(int j=0;j<col;j++){
            *(int*)num = ntohl(*(int*)num);
            num += sizeof(int);
        }
    }
    data = (int*)(m_buf.data() + m_encoded_len);
    m_encoded_len += sizeof(int)*row*col;
    return 0;
    //char *ptr = NULL;
    //if (read_binary(sizeof(int) * row * col, ptr) < 0) {
     //   return -1;
    //}
    //data = (int*)ptr;
}

void basic_proto::add_string(int str_len, const char* str){
    add_int(str_len);
    add_binary(str, str_len);
}

int basic_proto::read_string(int &str_len, char* &bin){
    if(read_int(str_len) < 0){
        return -1;
    }
    if(read_binary(str_len, bin) <0){
        return -1;
    }
    return 0;
}

int basic_proto::get_buf_len(){
    return m_encoded_len;
}

const char* basic_proto::get_buf(){
    return m_buf.data();
}
