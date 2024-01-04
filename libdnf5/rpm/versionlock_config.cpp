/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "rpm/versionlock_config.hpp"

#include "libdnf5/common/sack/query_cmp.hpp"

#include <fmt/format.h>
#include <toml.hpp>

#include <map>
#include <utility>

namespace {
// supported config file version
const std::string_view CONFIG_FILE_VERSION = "1.0";
}  // namespace


namespace toml {

template <>
struct from<libdnf5::rpm::VersionlockCondition> {
    static libdnf5::rpm::VersionlockCondition from_toml(const toml::value & val) {
        auto key = toml::find_or<std::string>(val, "key", "");
        auto comparator = toml::find_or<std::string>(val, "comparator", "");
        auto value = toml::find_or<std::string>(val, "value", "");

        libdnf5::rpm::VersionlockCondition condition(key, comparator, value);

        return condition;
    }
};

template <>
struct into<libdnf5::rpm::VersionlockCondition> {
    static toml::value into_toml(const libdnf5::rpm::VersionlockCondition & condition) {
        toml::value res;

        res["key"] = condition.get_key_str();
        res["comparator"] = condition.get_comparator_str();
        res["value"] = condition.get_value();

        return res;
    }
};

template <>
struct from<libdnf5::rpm::VersionlockPackage> {
    static libdnf5::rpm::VersionlockPackage from_toml(const toml::value & val) {
        auto name = toml::find_or<std::string>(val, "name", "");
        libdnf5::rpm::VersionlockPackage package(name);
        package.set_conditions(toml::find_or<std::vector<libdnf5::rpm::VersionlockCondition>>(val, "conditions", {}));

        return package;
    }
};

template <>
struct into<libdnf5::rpm::VersionlockPackage> {
    static toml::value into_toml(const libdnf5::rpm::VersionlockPackage & package) {
        toml::value res;

        res["name"] = package.get_name();
        res["conditions"] = package.get_conditions();

        return res;
    }
};

}  // namespace toml


namespace libdnf5::rpm {

const std::map<std::string, libdnf5::sack::QueryCmp> VersionlockCondition::VALID_COMPARATORS = {
    {"=", libdnf5::sack::QueryCmp::EQ},
    {"==", libdnf5::sack::QueryCmp::EQ},
    {"<", libdnf5::sack::QueryCmp::LT},
    {"<=", libdnf5::sack::QueryCmp::LTE},
    {">", libdnf5::sack::QueryCmp::GT},
    {">=", libdnf5::sack::QueryCmp::GTE},
    {"<>", libdnf5::sack::QueryCmp::NEQ},
    {"!=", libdnf5::sack::QueryCmp::NEQ},
};

VersionlockCondition::VersionlockCondition(
    const std::string & key_str, const std::string & comparator_str, const std::string & value)
    : valid(true),
      key_str(key_str),
      comparator_str(comparator_str),
      value(value) {
    // check that condition key is present and valid
    if (key_str == "epoch") {
        key = Keys::EPOCH;
    } else if (key_str == "version") {
        key = Keys::VERSION;
    } else if (key_str == "evr") {
        key = Keys::EVR;
    } else if (key_str == "arch") {
        key = Keys::ARCH;
    } else {
        valid = false;
        if (key_str.empty()) {
            errors.emplace_back("Missing condition key.");
        } else {
            errors.emplace_back("Invalid condition key.");
        }
    }

    // check that condition comparison operator is present and valid
    if (VALID_COMPARATORS.contains(comparator_str)) {
        comparator = VALID_COMPARATORS.at(comparator_str);
    } else {
        valid = false;
        if (comparator_str.empty()) {
            errors.emplace_back("Missing condition comparator.");
        } else {
            errors.emplace_back("Invalid condition comparator.");
        }
    }

    // check that condition value is present
    if (value.empty()) {
        valid = false;
        errors.emplace_back("Missing condition value.");
    }

    if (valid) {
        // additional checks for specific keys
        switch (key) {
            case Keys::EPOCH:
                // the epoch condition requires a valid integer as a value
                try {
                    std::stoul(value);
                } catch (...) {
                    valid = false;
                    errors.emplace_back("Epoch condition needs to be an unsigned integer value.");
                }
                break;
            case Keys::ARCH:
                if (comparator != libdnf5::sack::QueryCmp::EQ && comparator != libdnf5::sack::QueryCmp::NEQ) {
                    valid = false;
                    errors.emplace_back("Arch condition only supports '=' and '!=' comparison operators.");
                }
            default:
                break;
        }
    }
}

std::string VersionlockCondition::to_string() const {
    return fmt::format("{} {} {}", key_str, comparator_str, value);
}


VersionlockPackage::VersionlockPackage(const std::string & name) : valid(true), name(name) {
    // check that package name is present
    if (name.empty()) {
        valid = false;
        errors.emplace_back("Missing package name.");
    }
}

void VersionlockPackage::set_conditions(std::vector<VersionlockCondition> && conditions) {
    this->conditions = std::move(conditions);
}


VersionlockConfig::VersionlockConfig(const std::filesystem::path & path) : path(path) {
    if (!std::filesystem::exists(path)) {
        return;
    }

    auto toml_value = toml::parse(this->path);

    if (!toml_value.contains("version")) {
        // TODO(mblaha) Log unversioned versionlock file?
        return;
    } else if (toml::find<std::string>(toml_value, "version") != CONFIG_FILE_VERSION) {
        // TODO(mblaha) Log unsupported versionlock file version?
        return;
    }

    packages = toml::find_or<std::vector<VersionlockPackage>>(toml_value, "packages", {});
}

}  // namespace libdnf5::rpm
