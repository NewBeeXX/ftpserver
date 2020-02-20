#ifndef FTPPROTO_H_INCLUDED
#define FTPPROTO_H_INCLUDED

#include "session.h"
void handle_child(session_t* sess);
void ftp_reply(session_t* sess,int status,const char* text);

#endif // FTPPROTO_H_INCLUDED
