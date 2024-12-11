#ifndef _entrypoint_h
#define _entrypoint_h

#include <string>
#include <vector>

#include <swiftly-ext/core.h>
#include <swiftly-ext/extension.h>
#include <swiftly-ext/hooks/NativeHooks.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include <filesystem>
#include <unordered_map>

class HotReload : public SwiftlyExt
{
private:
    bool m_status = false;
    unsigned int m_interval = 5000;
    std::vector<std::string> m_plugins;

public:
    std::unordered_map<std::string, std::filesystem::file_time_type> m_paths;

    bool Load(std::string& error, SourceHook::ISourceHook *SHPtr, ISmmAPI* ismm, bool late);
    bool Unload(std::string& error);
    
    void AllExtensionsLoaded();
    void AllPluginsLoaded();

    bool OnPluginLoad(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error);
    bool OnPluginUnload(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error);

    void LoadSettings();
    
    void SetStatus(bool status);
    bool GetStatus() { return m_status; }

    std::vector<std::string> GetPlugins() { return m_plugins; }
    
    unsigned int GetInterval() { return m_interval; }

public:
    const char* GetAuthor();
    const char* GetName();
    const char* GetVersion();
    const char* GetWebsite();
};

extern HotReload g_Ext;
DECLARE_GLOBALVARS();

#endif