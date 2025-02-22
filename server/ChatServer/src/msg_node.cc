#include "msg_node.h"
#include <cstring>
#include <boost/asio.hpp>
#include <iostream>
#include "const.h"

MsgNode::MsgNode(short max_len) : m_cur_len(0), m_total_len(max_len) {
    m_data = new char[max_len + 1];
    m_data[max_len] = '/0';
}

MsgNode::~MsgNode() {
    std::cout << "MsgNode destruct" << std::endl;
    delete[] m_data;
}

void MsgNode::clear() {
    memset(m_data, 0, m_total_len);
    m_cur_len = 0;
}

RecvNode::RecvNode(short max_len, short msg_id) 
    : MsgNode(max_len), m_msg_id(msg_id) {

}

SendNode::SendNode(const char* msg, short max_len, short msg_id) 
    : MsgNode(max_len + MAX_LENGTH), m_msg_id(msg_id) {
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(m_data, &msg_id_host, HEAD_ID_LEN);

    short msg_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(m_data + HEAD_ID_LEN, &msg_len_host, HEAD_DATA_LEN);

    memcpy(m_data + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
}
