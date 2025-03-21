#ifndef __MSG_NODE_H__
#define __MSG_NODE_H__
#include "logic_system.h"
class MsgNode {
public:
    MsgNode(short max_len);
    virtual ~MsgNode();
    void clear();

    short m_cur_len;
    short m_total_len;
    char* m_data;
};

class RecvNode : public MsgNode {
friend class LogicSystem;
public:
    RecvNode(short max_len, short msg_id);
private:
    short m_msg_id;
};

class SendNode : public MsgNode {
friend class LogicSystem;
public:
    SendNode(const char* msg, short max_len, short msg_id);
private:
    short m_msg_id;
};

#endif // __MSG_NODE_H__
