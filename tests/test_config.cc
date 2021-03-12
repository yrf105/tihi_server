#include <iostream>
#include <vector>

#include "config/config.h"
#include "log/log.h"
#include "yaml-cpp/yaml.h"


#if 0
void print_yaml(const YAML::Node& node, int level) {
    if (node.IsNull()) {
        TIHI_LOG_INFO(TIHI_LOG_ROOT())
            << std::string(level * 4, ' ') << "Null - " << node.Type() << " - "
            << level;
    } else if (node.IsScalar()) {
        TIHI_LOG_INFO(TIHI_LOG_ROOT())
            << std::string(level * 4, ' ') << node.Scalar() << " - "
            << node.Type() << " - " << level;
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            TIHI_LOG_INFO(TIHI_LOG_ROOT())
                << std::string(level * 4, ' ') << i << " - " << node[i].Type()
                << " - " << level;
            print_yaml(node[i], level + 1);
        }
    } else if (node.IsMap()) {
        for (auto& it : node) {
            TIHI_LOG_INFO(TIHI_LOG_ROOT())
                << std::string(level * 4, ' ') << it.first << " - "
                << it.second.Type() << " - " << level;
            print_yaml(it.second, level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("../bin/config.yml");

    print_yaml(root, 0);
}

tihi::ConfigVar<std::vector<int>>::ptr g_vec_int_value_config =
    tihi::Config::Lookup<std::vector<int>>(
        "system.vec", std::vector<int>({1, 2}), "system vec");

tihi::ConfigVar<std::list<int>>::ptr g_list_int_value_config =
    tihi::Config::Lookup<std::list<int>>("system.list", std::list<int>({3, 4}),
                                         "system list");

tihi::ConfigVar<std::set<int>>::ptr g_set_int_value_config =
    tihi::Config::Lookup<std::set<int>>("system.set", std::set<int>({5, 6}),
                                        "system set");

tihi::ConfigVar<std::unordered_set<int>>::ptr g_uset_int_value_config =
    tihi::Config::Lookup<std::unordered_set<int>>(
        "system.uset", std::unordered_set<int>({7, 8}), "system uset");

tihi::ConfigVar<std::map<std::string, int>>::ptr g_map_int_value_config =
    tihi::Config::Lookup<std::map<std::string, int>>(
        "system.map", std::map<std::string, int>({{"nine", 9}, {"ten", 10}}),
        "system map");

tihi::ConfigVar<std::unordered_map<std::string, int>>::ptr g_umap_int_value_config =
    tihi::Config::Lookup<std::unordered_map<std::string, int>>(
        "system.umap", std::unordered_map<std::string, int>({{"eleven", 11}, {"twelve", 12}}),
        "system umap");

void test_config() {
    tihi::Config::Lookup<int>("system.port", 80, "http_port");
    TIHI_LOG_DEBUG(TIHI_LOG_ROOT())
        << "before: " << tihi::Config::Lookup<int>("system.port")->toString();
    tihi::Config::Lookup<float>("system.port", 80, "http_port");
    // TIHI_LOG_DEBUG(TIHI_LOG_ROOT())
    //     << "before: " << tihi::Config::Lookup<float>("system.port")->toString();

#define XX(g_val, name, prefix)                                              \
    do {                                                                     \
        auto vec = g_val->value();                                           \
        for (auto i : vec) {                                                 \
            TIHI_LOG_DEBUG(TIHI_LOG_ROOT()) << #prefix " " #name " : " << i; \
        }                                                                    \
        TIHI_LOG_DEBUG(TIHI_LOG_ROOT())                                      \
            << #prefix " " #name " yaml : " << g_val->toString();            \
    } while (0)

#define XX_MAP(g_val, name, prefix)                                         \
    do {                                                                    \
        auto m = g_val->value();                                            \
        for (auto i : m) {                                                  \
            TIHI_LOG_DEBUG(TIHI_LOG_ROOT())                                 \
                << #prefix " " #name " : " << i.first << " : " << i.second; \
        }                                                                   \
        TIHI_LOG_DEBUG(TIHI_LOG_ROOT())                                     \
            << #prefix " " #name " yaml : " << g_val->toString();           \
    } while (0)

    XX(g_vec_int_value_config, "system.vec", "before");
    XX(g_list_int_value_config, "system.list", "before");
    XX(g_set_int_value_config, "system.set", "before");
    XX(g_uset_int_value_config, "system.uset", "before");
    XX_MAP(g_map_int_value_config, "system.map", "before");
    XX_MAP(g_umap_int_value_config, "system.umap", "before");

    YAML::Node root = YAML::LoadFile("../bin/config.yml");
    tihi::Config::LoadFromYAML(root);

    TIHI_LOG_DEBUG(TIHI_LOG_ROOT())
        << "after: " << tihi::Config::Lookup<int>("system.port")->toString();

    XX(g_vec_int_value_config, "system.vec", "after");
    XX(g_list_int_value_config, "system.list", "after");
    XX(g_set_int_value_config, "system.set", "after");
    XX(g_uset_int_value_config, "system.uset", "after");
    XX_MAP(g_map_int_value_config, "system.map", "after");
    XX_MAP(g_umap_int_value_config, "system.umap", "after");

#undef XX
#undef XX_MAP
}
#endif

struct Person {
    std::string name_;
    int age_ = 0;
    bool man_ = true;

    std::string ToString() {
        std::stringstream ss;
        ss << "name: " << name_ << " age: " << age_ << " man: " << man_;
        return ss.str();
    }
};

namespace tihi {
template <>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person ret;
        std::stringstream ss;
        ret.name_ = node["name"].as<std::string>();
        ret.age_ = node["age"].as<int>();
        ret.man_ = node["man"].as<bool>();

        return ret;
    }
};

template <>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& v) {
        YAML::Node node;
        node["name"] = v.name_;
        node["age"] = v.age_;
        node["man"] = v.man_;

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};
}  // namespace tihi

tihi::ConfigVar<Person>::ptr g_person =
    tihi::Config::Lookup<Person>("class.person", Person(), "class person");

tihi::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
    tihi::Config::Lookup<std::map<std::string, Person>>(
        "class.map", {{"yue", Person()}}, "class map");


void test_class() {
#define XX(g_val, name, prefix)                                             \
    do {                                                                    \
        auto val = g_val->value();                                          \
        TIHI_LOG_DEBUG(TIHI_LOG_ROOT()) << #prefix " size: " << val.size(); \
        for (auto& i : val) {                                               \
            TIHI_LOG_DEBUG(TIHI_LOG_ROOT())                                 \
                << #prefix " " #name << i.first << " : "                    \
                << i.second.ToString() << "\n";                             \
        }                                                                   \
    } while (0)

    TIHI_LOG_DEBUG(TIHI_LOG_ROOT())
        << "before: " << g_person->toString() << "\n"
        << g_person->value().ToString();
    XX(g_person_map, "class.map", "before");

    YAML::Node root = YAML::LoadFile("../bin/config.yml");
    tihi::Config::LoadFromYAML(root);

    TIHI_LOG_DEBUG(TIHI_LOG_ROOT()) << "after: " << g_person->toString() << "\n"
                                    << g_person->value().ToString();
    XX(g_person_map, "class.map", "after");
#undef XX
}

void test() {
    // test_config();
    test_class();
}

int main(int argc, char** argv) {
    test();

    // test_yaml();

    TIHI_LOG_INFO(TIHI_LOG_ROOT()) << "end";
    return 0;
}