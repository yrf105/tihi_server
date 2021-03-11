#ifndef TIHI_CONFIG_CONFIG_H_
#define TIHI_CONFIG_CONFIG_H_

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <exception>
#include <memory>
#include <string>
#include <vector>
#include "yaml-cpp/yaml.h"

#include "log/log.h"

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

template <typename T>
class ConfigVar : public ConfigVarInterface {
public:
    using ptr = std::shared_ptr<ConfigVar>;

    ConfigVar(std::string name, const T& default_val,
              const std::string& description)
        : ConfigVarInterface(name, description), val_(default_val) {}

    std::string toString() override {
        try {
            return boost::lexical_cast<std::string>(val_);
        } catch (std::exception& e) {
            TIHI_LOG_ERROR(TIHI_LOG_ROOT())
                << "ConfigVar::toString exception " << e.what()
                << "convert: " << typeid(val_).name() << "to string";
        }
        return "";
    }

    bool formString(const std::string& v) override {
        try {
            val_ = boost::lexical_cast<T>(v);
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
    using ConfigVarMap = std::map<std::string, ConfigVarInterface::ptr>;

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