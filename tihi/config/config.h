#ifndef TIHI_CONFIG_CONFIG_H_
#define TIHI_CONFIG_CONFIG_H_

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <exception>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

#include "log/log.h"
#include "yaml-cpp/yaml.h"

namespace tihi {

class ConfigVarInterface {
public:
    ConfigVarInterface(std::string name, const std::string& description)
        : name_(name), description_(description) {
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    }

    using ptr = std::shared_ptr<ConfigVarInterface>;
    virtual ~ConfigVarInterface(){};

    virtual std::string toString() = 0;
    virtual bool formString(const std::string& v) = 0;

private:
    std::string name_;
    std::string description_;
};

template <typename F, typename T>
class LexicalCast {
public:
    T operator()(const F& v) { return boost::lexical_cast<T>(v); }
};

template <typename T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::vector<T> ret;
        std::stringstream ss;
        for (const auto& it : node) {
            ss.str("");
            ss << it;
            ret.push_back(LexicalCast<std::string, T>()(ss.str()));
        }

        return ret;
    }
};

template <typename T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node;
        for (const auto& it : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(it)));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::list<T> ret;
        std::stringstream ss;
        for (const auto& it : node) {
            ss.str("");
            ss << it;
            ret.push_back(LexicalCast<std::string, T>()(ss.str()));
        }

        return ret;
    }
};

template <typename T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v) {
        YAML::Node node;
        for (const auto& it : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(it)));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::set<T> ret;
        std::stringstream ss;
        for (const auto& it : node) {
            ss.str("");
            ss << it;
            ret.insert(LexicalCast<std::string, T>()(ss.str()));
        }

        return ret;
    }
};

template <typename T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v) {
        YAML::Node node;
        for (const auto& it : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(it)));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::unordered_set<T> ret;
        std::stringstream ss;
        for (const auto& it : node) {
            ss.str("");
            ss << it;
            ret.insert(LexicalCast<std::string, T>()(ss.str()));
        }

        return ret;
    }
};

template <typename T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v) {
        YAML::Node node;
        for (const auto& it : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(it)));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::map<std::string, T> ret;
        std::stringstream ss;
        for (const auto& it : node) {
            ss.str("");
            ss << it.second;
            ret[it.first.Scalar()] = LexicalCast<std::string, T>()(ss.str());
        }
        return ret;
    }
};

template <typename T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v) {
        YAML::Node node;
        for (const auto& it : v) {
            node[it.first] = YAML::Load(LexicalCast<T, std::string>()(it.second));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::unordered_map<std::string, T> ret;
        std::stringstream ss;
        for (const auto& it : node) {
            ss.str("");
            ss << it.second;
            ret[it.first.Scalar()] = LexicalCast<std::string, T>()(ss.str());
        }
        return ret;
    }
};

template <typename T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v) {
        YAML::Node node;
        for (const auto& it : v) {
            node[it.first] = YAML::Load(LexicalCast<T, std::string>()(it.second));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

/*
Fromstr 为仿函数，需要 T opterator() (const std::string&)
来自定义从字符串到 T 的转化
ToStr 为仿函数，需要 std::string opterator() (const T&)
来自定义从 T 到字符串的转化
*/
template <typename T, typename FromStr = LexicalCast<std::string, T>,
          typename ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarInterface {
public:
    using ptr = std::shared_ptr<ConfigVar>;

    ConfigVar(std::string name, const T& default_val,
              const std::string& description)
        : ConfigVarInterface(name, description), val_(default_val) {}

    std::string toString() override {
        try {
            // return boost::lexical_cast<std::string>(val_);
            return ToStr()(value());
        } catch (std::exception& e) {
            TIHI_LOG_ERROR(TIHI_LOG_ROOT())
                << "ConfigVar::toString exception " << e.what()
                << "convert: " << typeid(val_).name() << "to string";
        }
        return "";
    }

    bool formString(const std::string& v) override {
        try {
            // val_ = boost::lexical_cast<T>(v);
            set_value(FromStr()(v));
            return true;
        } catch (std::exception& e) {
            TIHI_LOG_ERROR(TIHI_LOG_ROOT())
                << "ConfigVar::fromString exception " << e.what()
                << "convert: string to" << typeid(v).name();
        }
        return false;
    }

    T value() const { return val_; }
    void set_value(const T& v) { val_ = v; }

private:
    T val_;
};

class Config {
public:
    using ConfigVarMap = std::unordered_map<std::string, ConfigVarInterface::ptr>;

    template <typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        auto it = datas_.find(name);
        if (it == datas_.end()) {
            return nullptr;
        }

        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    template <typename T>
    static typename ConfigVar<T>::ptr Lookup(
        const std::string& name, const T& default_val,
        const std::string& description = "") {
        if (name.find_first_not_of("abcdefghigklmnopqrstuvwxyz._1234567890") !=
            name.npos) {
            throw(std::invalid_argument(name));
        }

        auto ret = Lookup<T>(name);
        if (ret) {
            TIHI_LOG_INFO(TIHI_LOG_ROOT())
                << "Lookup name= " << name << " exited";
            return ret;
        }

        typename ConfigVar<T>::ptr v(
            new ConfigVar<T>(name, default_val, description));
        datas_[name] = v;
        return v;
    }

    static ConfigVarInterface::ptr LookupInterface(const std::string& name);
    static void LoadFromYAML(const YAML::Node& root);

private:
    static ConfigVarMap datas_;
};

}  // namespace tihi

#endif  // TIHI_CONFIG_CONFIG_H_