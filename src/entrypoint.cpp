#include "entrypoint.h"
#include "utils.h"

#include <thread>
#include <fstream>

//////////////////////////////////////////////////////////////
/////////////////        Core Variables        //////////////
////////////////////////////////////////////////////////////

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK1_void(IServerGameDLL, ServerHibernationUpdate, SH_NOATTRIB, 0, bool);

HotReload g_Ext;
CUtlVector<FuncHookBase *> g_vecHooks;

IVEngineServer2* engine = nullptr;
ISource2Server* server = nullptr;

CREATE_GLOBALVARS();

//////////////////////////////////////////////////////////////
/////////////////          Core Class          //////////////
////////////////////////////////////////////////////////////

void HotReloadError(std::string text)
{
    if (!g_SMAPI)
        return;

    g_SMAPI->ConPrintf("[Hot Reload] %s\n", text.c_str());
}

EXT_EXPOSE(g_Ext);
bool HotReload::Load(std::string& error, SourceHook::ISourceHook *SHPtr, ISmmAPI* ismm, bool late)
{
    SAVE_GLOBALVARS();
    if(!InitializeHooks()) {
        error = "Failed to initialize hooks.";
        return false;
    }

    engine = (IVEngineServer *)ismm->VInterfaceMatch(ismm->GetEngineFactory(), INTERFACEVERSION_VENGINESERVER); 
    if (!engine) { 
        error = "Could not find interface: " INTERFACEVERSION_VENGINESERVER;
        return false; 
    }

    server = (ISource2Server *)ismm->VInterfaceMatch(ismm->GetServerFactory(), INTERFACEVERSION_SERVERGAMEDLL, 0); 
    if (!server) {
        error = "Could not find interface: " INTERFACEVERSION_SERVERGAMEDLL;
        return false;
    }

    // SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &HTTPExtension::Hook_GameFrame, true);
    // SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerHibernationUpdate, server, this, &HTTPExtension::Hook_ServerHibernationUpdate, true);

    LoadSettings();

    return true;
}

bool HotReload::Unload(std::string& error)
{
    // SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &HTTPExtension::Hook_GameFrame, true);
    // SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerHibernationUpdate, server, this, &HTTPExtension::Hook_ServerHibernationUpdate, true);

    UnloadHooks();
    return true;
}

void HotReload::LoadSettings()
{
    m_status = false;
    m_plugins.clear();
    m_interval = 5000;

    std::ifstream ifs("addons/swiftly/configs/hotreload.json");
    if(!ifs.is_open()) {
        HotReloadError("Failed to open 'addons/swiftly/configs/hotreload.json'.");
        return;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document hotreloadFile;
    hotreloadFile.ParseStream(isw);

    if (hotreloadFile.HasParseError())
        return HotReloadError(string_format("A parsing error has been detected.\nError (offset %u): %s\n", (unsigned)hotreloadFile.GetErrorOffset(), GetParseError_En(hotreloadFile.GetParseError())));

    if (hotreloadFile.IsArray())
        return HotReloadError("Hot reload configuration file cannot be an array.");

    if(hotreloadFile["enabled"].IsBool())
        SetStatus(hotreloadFile["enabled"].GetBool());

    if(hotreloadFile["interval"].IsUint())
        m_interval = hotreloadFile["interval"].GetUint();

    if(hotreloadFile["plugins"].IsArray()) {
        for(rapidjson::SizeType i = 0; i < hotreloadFile["plugins"].Size(); i++) {
            if(hotreloadFile["plugins"][i].IsString()) {
                m_plugins.push_back(hotreloadFile["plugins"][i].GetString());
            }
        }
    }
}

void HotReload::SetStatus(bool status)
{
    m_status = status;
}

void HotReload::AllExtensionsLoaded()
{

}

namespace fs = std::filesystem;

void watchDirectory(const fs::path& path) {
    std::unordered_map<std::string, fs::file_time_type> fileTimestamps;

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry)) {
            fileTimestamps[entry.path().string()] = fs::last_write_time(entry);
        }
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(g_Ext.GetInterval()));
        if(g_Ext.GetStatus()) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (fs::is_regular_file(entry)) {
                    auto path = entry.path().string();
                    auto currentTimestamp = fs::last_write_time(entry);

                    if (fileTimestamps.find(path) == fileTimestamps.end()) {

                        fileTimestamps[path] = currentTimestamp;
                    } else if (fileTimestamps[path] != currentTimestamp) {

                        fileTimestamps[path] = currentTimestamp;
                    }
                }
            }

            for (auto it = fileTimestamps.begin(); it != fileTimestamps.end();) {
                if (!fs::exists(it->first)) {

                    it = fileTimestamps.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

void HotReload::AllPluginsLoaded()
{
    std::thread(watchDirectory, "addons/swiftly/plugins").detach();
}

bool HotReload::OnPluginLoad(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error)
{
    return true;
}

bool HotReload::OnPluginUnload(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error)
{
    return true;
}

const char* HotReload::GetAuthor()
{
    return "Swiftly Development Team";
}

const char* HotReload::GetName()
{
    return "Base Extension";
}

const char* HotReload::GetVersion()
{
#ifndef VERSION
    return "Local";
#else
    return VERSION;
#endif
}

const char* HotReload::GetWebsite()
{
    return "https://swiftlycs2.net/";
}
