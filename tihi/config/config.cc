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

    if (node.IsScalar()) {
        ret.push_back(std::make_pair(prefix, node));
    } else if (node.IsMap()) {
        for (auto& it : node) {
            ListAllMembers(
                prefix.empty() ? it.first.Scalar() : prefix + "." + it.first.Scalar(),
                it.second, ret);
        }
    }
}

void Config::LoadFromYAML(const YAML::Node& root) {
    std::list<std::pair<std::string, YAML::Node>> all_members;
    ListAllMembers("", root, all_members);

    for (auto& it : all_members) {
        std::string key = it.first;
        if (key.empty()) {
            return ;
        }

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