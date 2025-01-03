#ifndef __CONFIG_MGR_H__
#define __CONFIG_MGR_H__

#include <map>
#include <string>

struct SectionInfo {
    SectionInfo(){}
    ~SectionInfo() {
        m_section_datas.clear();
    }

    SectionInfo(const SectionInfo& section_info) {
        m_section_datas = section_info.m_section_datas;
    }

    SectionInfo& operator=(const SectionInfo& section_info) {
        if(&section_info == this) {
            return *this;
        }
        m_section_datas = section_info.m_section_datas;
        return *this;
    }

    std::string operator[](const std::string& key) {
        if(m_section_datas.find(key) == m_section_datas.end()) {
            return "";
        }
        return m_section_datas[key];
    }

    std::string getValue(const std::string& key) {
        if(m_section_datas.find(key) == m_section_datas.end()) {
            return "";
        }
        return m_section_datas[key];
    }

    std::map<std::string, std::string> m_section_datas;
};

class ConfigMgr {
public:
    ConfigMgr(const ConfigMgr& config_mgr);
    ~ConfigMgr();
    ConfigMgr& operator=(const ConfigMgr& config_mgr);
    SectionInfo operator[](const std::string& key);
    SectionInfo getValue(const std::string& key);
    
    static ConfigMgr& Inst();
private:
    ConfigMgr();
    std::map<std::string, SectionInfo> m_config_map;
};

#endif // __CONFIG_MGR_H__
