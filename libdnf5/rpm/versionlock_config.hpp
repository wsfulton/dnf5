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


#ifndef LIBDNF5_RPM_VERSIONLOCK_CONFIG_HPP
#define LIBDNF5_RPM_VERSIONLOCK_CONFIG_HPP

#include "libdnf5/common/sack/query_cmp.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace libdnf5::rpm {

class VersionlockCondition {
public:
    enum class Keys { EPOCH, VERSION, EVR, ARCH };

    VersionlockCondition(const std::string & key_str, const std::string & comparator_str, const std::string & value);

    bool is_valid() const { return valid; }
    Keys get_key() const { return key; }
    libdnf5::sack::QueryCmp get_comparator() const { return comparator; }
    std::string get_value() const { return value; }

    std::string get_key_str() const { return key_str; }
    std::string get_comparator_str() const { return comparator_str; }

    std::vector<std::string> get_errors() const { return errors; }

    std::string to_string() const;

private:
    // map string comparator to query cmp_type
    static const std::map<std::string, libdnf5::sack::QueryCmp> VALID_COMPARATORS;
    bool valid;
    std::string key_str;
    Keys key;
    std::string comparator_str;
    libdnf5::sack::QueryCmp comparator{0};
    std::string value;
    std::vector<std::string> errors{};
};

class VersionlockPackage {
public:
    VersionlockPackage(const std::string & name);

    bool is_valid() const { return valid; }
    std::string get_name() const { return name; }
    std::vector<VersionlockCondition> get_conditions() const { return conditions; }
    void set_conditions(std::vector<VersionlockCondition> && conditions);

    std::vector<std::string> get_errors() const { return errors; }

private:
    bool valid;
    std::string name;
    std::vector<VersionlockCondition> conditions{};
    std::vector<std::string> errors{};
};

class VersionlockConfig {
public:
    /// Creates an instance of `VersionlockConfig` specifying the config file
    /// to read.
    /// @param path Path to versionlock configuration file.
    VersionlockConfig(const std::filesystem::path & path);

    /// Get list of configured versionlock entries.
    std::vector<VersionlockPackage> get_packages() { return packages; }

private:
    std::filesystem::path path;
    std::vector<VersionlockPackage> packages{};
};

}  // namespace libdnf5::rpm

#endif  // LIBDNF5_RPM_VERSIONLOCK_CONFIG_HPP
