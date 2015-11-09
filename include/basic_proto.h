//
//  basic_proto.h
//  
//
//  Created by Xinyue Huang on 10/1/15.
//
//

#ifndef _basic_protoco_h
#define _basic_protoco_h

#include <string>

class basic_proto{
    public:
        basic_proto();
        basic_proto(const char* buf, int buf_len);
        virtual ~basic_proto();
    
    public:
        void add_int(int retval);
        int read_int (int &retval);
    
        void add_float(float retval);
        int read_float(float &retval);
    
        void add_double(double retval);
        int read_double(double &retval);

        void add_binary(const char* bin, int bin_len);
        int read_binary(int bin_len, char* &bin);

        void add_matrix(int *data, int row, int col);
        int read_matrix(int *&data, int &row, int &col);

        void add_array(int *data, int size);
        int read_array(int *&data, int &size);
    
        void add_string(int str_len, const char* str);
        int read_string(int &str_len, char* &bin);
    
        int get_buf_len();
        const char* get_buf();
    
    protected:
        int m_encoded_len;
        std::string m_buf;
};
#endif
