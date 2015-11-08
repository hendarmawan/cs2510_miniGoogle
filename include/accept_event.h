#ifndef __ACCEPT_EVENT__
#define __ACCEPT_EVENT__

#include "io_event.h"

class accept_event: public io_event {
    public:
        /**
         * @brief construct
         *
         * @param svr
         */
        accept_event(svr_base *svr);

        /**
         * @brief destruct
         */
        virtual ~accept_event();

    public:
        /**
         * @brief bind to a specific port
         *
         * @param port
         * @param backlog
         *
         * @return 
         */
        int bind(unsigned short port, int backlog);

        /**
         * @brief notify incoming io event
         */
        virtual void on_event();
};

#endif
