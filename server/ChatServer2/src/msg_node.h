#ifndef __MSG_NODE_H__
#define __MSG_NODE_H__

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
public:
    RecvNode(short max_len, short msg_id);
private:
    short m_msg_id;
};

class SendNode : public MsgNode {
public:
    SendNode(const char* msg, short max_len, short msg_id);
private:
    short m_msg_id;
};

#endif // __MSG_NODE_H__
