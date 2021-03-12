#include "config.h"

namespace tihi {
Config::ConfigVarMap Config::datas_;

ConfigVarInterface::ptr Config::LookupInterface(const std::string& name) {
    auto it = datas_.find(name);
    return it == datas_.end() ? nullptr : it->second;
}

void ListAllMembers(const std::string& prefix, const YAML::Node& node,
                    std::list<std::pair<std::string, YAML::Node>>& ret) {
    if (node.IsNull()) {
        TIHI_LOG_ERROR(TIHI_LOG_ROOT()) << "YAML::NODE is null!";
        return;
    }

    if (prefix.find_first_not_of("abcdefghigklmnopqrstuvwxyz._1234567890") !=
        std::string::npos) {
        TIHI_LOG_ERROR(TIHI_LOG_ROOT())
            << "Invalid name: " << prefix << " : " << node;
        return;
    }

    ret.push_back(std::make_pair(prefix, node));

    /*
    如果是 map 的话，会把每一层都保存一下
    */
    if (node.IsMap()) {
        for (auto& it : node) {
            ListAllMembers(
                prefix.empty() ? it.first.Scalar() : prefix + "." + it.first.Scalar(),
                it.second, ret);
        }
    } 
    // else {
    //     ret.push_back(std::make_pair(prefix, node));
    // }
}

void Config::LoadFromYAML(const YAML::Node& root) {
    std::list<std::pair<std::string, YAML::Node>> all_members;
    ListAllMembers("", root, all_members);

    for (auto& it : all_members) {
        std::string key = it.first;
        // TIHI_LOG_DEBUG(TIHI_LOG_ROOT()) << key;
        if (key.empty()) {
            continue ;
        }

        /*
        只能配置程序里有的参数，程序里没有的参数就算配置了也不起作用
        */
        ConfigVarInterface::ptr var = LookupInterface(it.first);
        if (var) {
            if (it.second.IsScalar()) {
                var->formString(it.second.Scalar());
            } else {
                std::stringstream ss;
                ss << it.second;
                var->formString(ss.str());
            }
        }
    }

}

}  // namespace tihi