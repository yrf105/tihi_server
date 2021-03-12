#include "log/log.h"
#include "config/config.h"
#include "yaml-cpp/yaml.h"

#include <iostream>
#include <vector>

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsNull()) {
        TIHI_LOG_INFO(TIHI_LOG_ROOT()) << std::string(level * 4, ' ') << "Null - " << node.Type() << " - " << level;
    } else if (node.IsScalar()) {
        TIHI_LOG_INFO(TIHI_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            TIHI_LOG_INFO(TIHI_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    } else if (node.IsMap()) {
        for (auto& it : node) {
            TIHI_LOG_INFO(TIHI_LOG_ROOT()) << std::string(level * 4, ' ') << it.first << " - " << it.second.Type() << " - " << level;
            print_yaml(it.second, level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("../bin/config.yml");

    print_yaml(root, 0);
}

tihi::ConfigVar<std::vector<int>>::ptr g_vec_int_value_config = 
    tihi::Config::Lookup<std::vector<int>>("system.vec", std::vector<int>({1,2}), "system vec");

void test_config() {
    tihi::Config::Lookup<int>("system.port", 80, "http_port");
    TIHI_LOG_DEBUG(TIHI_LOG_ROOT()) << "before: " << tihi::Config::Lookup<int>("system.port")->toString();
    auto vec = g_vec_int_value_config->value();
    for (auto i : vec) {
        TIHI_LOG_DEBUG(TIHI_LOG_ROOT()) << "before: " << i;
    } 

    YAML::Node root = YAML::LoadFile("../bin/config.yml");
    tihi::Config::LoadFromYAML(root);

    TIHI_LOG_DEBUG(TIHI_LOG_ROOT()) << "after: " << tihi::Config::Lookup<int>("system.port")->toString();

    g_vec_int_value_config = tihi::Config::Lookup<std::vector<int>>("system.vec");
    vec = g_vec_int_value_config->value();
    for (auto i : vec) {
        TIHI_LOG_DEBUG(TIHI_LOG_ROOT()) << "after: " << i;
    }
}

void test() {
    test_config();
}

int main(int argc, char** argv) {

    test();

    // test_yaml();

    TIHI_LOG_INFO(TIHI_LOG_ROOT()) << "end";
    return 0;
}