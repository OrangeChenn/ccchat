#include "config_mgr.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

ConfigMgr& ConfigMgr::Inst() {
    static ConfigMgr cfg_mgr;
    return cfg_mgr;
}

ConfigMgr::ConfigMgr() {
    boost::filesystem::path current_path = boost::filesystem::current_path();
    boost::filesystem::path config_path = current_path / "config.ini";
    // std::string config_path = "/home/chen/Code/cpp/ccchat/server/GateServer/src/config.ini";

    // 使用boost.PropertyTree读取ini文件
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(config_path.string(), pt);

    // 解析数据，初始化
    for(const auto& section : pt) {
        std::string section_name = section.first;
        boost::property_tree::ptree section_tree = section.second;
        std::map<std::string, std::string> section_config;
        for(const auto& key_value_pair : section_tree) {
            section_config[key_value_pair.first] = 
                key_value_pair.second.get_value<std::string>();
        }
        SectionInfo section_info;
        section_info.m_section_datas = section_config;
        m_config_map[section_name] = section_info;
    }

    // 打印数据
    for(const auto& section : m_config_map) {
        std::cout << "[" << section.first << "]" << std::endl;
        for(const auto& key_value_pair : section.second.m_section_datas) {
            std::cout << key_value_pair.first << "=" 
                      << key_value_pair.second << std::endl;
        }
    }
}

ConfigMgr::ConfigMgr(const ConfigMgr& config_mgr) {
    this->m_config_map = config_mgr.m_config_map;
}

ConfigMgr::~ConfigMgr() {
    m_config_map.clear();
}

ConfigMgr& ConfigMgr::operator=(const ConfigMgr& config_mgr) {
    if(&config_mgr == this) {
        return *this;
    }
    this->m_config_map = config_mgr.m_config_map;
    return *this;
}

SectionInfo ConfigMgr::operator[](const std::string& key) {
    if(m_config_map.find(key) == m_config_map.end()) {
        return SectionInfo();
    }
    return m_config_map[key];
}

SectionInfo ConfigMgr::getValue(const std::string& key) {
    if(m_config_map.find(key) == m_config_map.end()) {
        return SectionInfo();
    }
    return m_config_map[key];
}
