/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <mutex>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem/operations.hpp>
#include "Config.h"
#include "Log.h"

using namespace boost::property_tree;
namespace bfs = boost::filesystem;

bool ConfigMgr::LoadInitial(std::string const& fileOrDirectory, std::string& error)
{
    std::lock_guard<std::mutex> lock(_configLock);

    _filename = fileOrDirectory;

    ptree newConfig;
    const auto load_tree = [&](const std::string& filename) {
        try
        {
            ptree localTree;
            ini_parser::read_ini(filename, localTree);

            if (localTree.empty())
            {
                error = "empty file (" + fileOrDirectory + ")";
                return false;
            }

            // Since we're using only one section per config file, we skip the section and have direct property access
            for (auto& entry : localTree.begin()->second)
            {
                ptree::path_type path(entry.first, '/');
                if (newConfig.erase(path.dump()) > 0)
                    printf("Config: file '%s' overwriting option '%s'\n", filename.c_str(), entry.first.c_str());
                newConfig.add_child(path, entry.second);
            }
        }
        catch (ini_parser::ini_parser_error const& e)
        {
            if (e.line() == 0)
                error = e.message() + " (" + e.filename() + ")";
            else
                error = e.message() + " (" + e.filename() + ":" + std::to_string(e.line()) + ")";
            return false;
        }

        return true;
    };

    if (bfs::is_directory(fileOrDirectory))
    {
        std::vector<bfs::path> files;
        std::copy(bfs::directory_iterator(fileOrDirectory), bfs::directory_iterator(), std::back_inserter(files));
        files.erase(std::remove_if(files.begin(), files.end(),
                [](const bfs::path& file) { return file.extension() != ".conf"; }),
                files.end());
        std::sort(files.begin(), files.end());

        for (const auto& file : files)
            if (!load_tree(file.string()))
                return false;
    }
    else
    {
        if (!load_tree(fileOrDirectory))
            return false;
    }

    _config = newConfig;

    return true;
}

ConfigMgr* ConfigMgr::instance()
{
    static ConfigMgr instance;
    return &instance;
}

bool ConfigMgr::Reload(std::string& error)
{
    return LoadInitial(_filename, error);
}

template<class T>
T ConfigMgr::GetValueDefault(std::string const& name, T def) const
{
    try
    {
        return _config.get<T>(ptree::path_type(name, '/'));
    }
    catch (boost::property_tree::ptree_bad_path)
    {
        TC_LOG_WARN("server.loading", "Missing name %s in config file %s, add \"%s = %s\" to this file",
            name.c_str(), _filename.c_str(), name.c_str(), std::to_string(def).c_str());
    }
    catch (boost::property_tree::ptree_bad_data)
    {
        TC_LOG_ERROR("server.loading", "Bad value defined for name %s in config file %s, going to use %s instead",
            name.c_str(), _filename.c_str(), std::to_string(def).c_str());
    }

    return def;
}

template<>
std::string ConfigMgr::GetValueDefault<std::string>(std::string const& name, std::string def) const
{
    try
    {
        return _config.get<std::string>(ptree::path_type(name, '/'));
    }
    catch (boost::property_tree::ptree_bad_path)
    {
        TC_LOG_WARN("server.loading", "Missing name %s in config file %s, add \"%s = %s\" to this file",
            name.c_str(), _filename.c_str(), name.c_str(), def.c_str());
    }
    catch (boost::property_tree::ptree_bad_data)
    {
        TC_LOG_ERROR("server.loading", "Bad value defined for name %s in config file %s, going to use %s instead",
            name.c_str(), _filename.c_str(), def.c_str());
    }

    return def;
}

std::string ConfigMgr::GetStringDefault(std::string const& name, const std::string& def) const
{
    std::string val = GetValueDefault(name, def);
    val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
    return val;
}

bool ConfigMgr::GetBoolDefault(std::string const& name, bool def) const
{
    std::string val = GetValueDefault(name, std::string(def ? "1" : "0"));
    val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
    return (val == "1" || val == "true" || val == "TRUE" || val == "yes" || val == "YES");
}

int ConfigMgr::GetIntDefault(std::string const& name, int def) const
{
    return GetValueDefault(name, def);
}

float ConfigMgr::GetFloatDefault(std::string const& name, float def) const
{
    return GetValueDefault(name, def);
}

std::string const& ConfigMgr::GetFilename()
{
    std::lock_guard<std::mutex> lock(_configLock);
    return _filename;
}

std::list<std::string> ConfigMgr::GetKeysByString(std::string const& name)
{
    std::lock_guard<std::mutex> lock(_configLock);

    std::list<std::string> keys;

    for (const ptree::value_type& child : _config)
        if (child.first.compare(0, name.length(), name) == 0)
            keys.push_back(child.first);
   
    return keys;
}
