// Real library types for the port build (PC and Vita). Included by the generated
// compat/lib_types.h before any game header, so nothing here may name a game type.
#pragma once

// ---- MSVC spellings that appear in the PDB-derived signatures
#if !defined(_MSC_VER)
#  ifndef __int64
#    define __int64 long long
#  endif
#  if !defined(_WIN32)
#    define __cdecl
#    define __stdcall
#    define __thiscall
#    define _iobuf __sFILE  // MSVC FILE is struct _iobuf; newlib's is struct __sFILE
#  endif
#endif

#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ---- SFML 2.x (PC: upstream SFML; Vita: SFML ported onto vitaGL/openal-soft)
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

// ---- rapidjson (header-only, used as is)
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// The game was written against pre-1.0 rapidjson, whose FILE* wrapper 1.x removed.
namespace rapidjson {
class FileStream {
public:
    typedef char Ch;
    explicit FileStream(std::FILE *fp) : fp_(fp), current_('\0'), count_(0) { Read(); }
    char Peek() const { return current_; }
    char Take() { char c = current_; Read(); return c; }
    std::size_t Tell() const { return count_; }
    void Put(char c) { std::fputc(c, fp_); }
    void Flush() { std::fflush(fp_); }
    char *PutBegin() { return 0; }
    std::size_t PutEnd(char *) { return 0; }
private:
    void Read() { int c = std::fgetc(fp_); if (c != EOF) { current_ = (char)c; count_++; } else current_ = '\0'; }
    std::FILE *fp_; char current_; std::size_t count_;
};
}

// ---- boost: the game only keeps boost::filesystem::path by value
namespace boost { namespace filesystem { using path = std::filesystem::path; } }

// ---- Steamworks: offline stubs. Achievements, lobbies and Workshop are out of scope.
class CSteamID {
public:
    CSteamID() : m_steamid(0) {}
    explicit CSteamID(std::uint64_t v) : m_steamid(v) {}
    std::uint64_t ConvertToUint64() const { return m_steamid; }
    bool IsValid() const { return m_steamid != 0; }
    bool operator==(const CSteamID &o) const { return m_steamid == o.m_steamid; }
    bool operator!=(const CSteamID &o) const { return m_steamid != o.m_steamid; }
    bool operator<(const CSteamID &o) const { return m_steamid < o.m_steamid; }
    std::uint64_t m_steamid;
};
class CGameID {
public:
    CGameID() : m_ulGameID(0) {}
    explicit CGameID(std::uint64_t v) : m_ulGameID(v) {}
    std::uint64_t ToUint64() const { return m_ulGameID; }
    std::uint64_t m_ulGameID;
};
class CCallbackBase { public: virtual ~CCallbackBase() {} };
template<int Size> class CCallbackImpl : public CCallbackBase {};
template<class T, class P, bool GameServer = false> class CCallback : public CCallbackImpl<sizeof(P *)> {
public:
    typedef void (T::*func_t)(P *);
    CCallback() {}
    CCallback(T *, func_t) {}
    void Register(T *, func_t) {}
    void Unregister() {}
};
template<class T, class P> class CCallResult : public CCallbackBase {
public:
    typedef void (T::*func_t)(P *, bool);
    void Set(std::uint64_t, T *, func_t) {}
    bool IsActive() const { return false; }
    void Cancel() {}
};
class ISteamUser; class ISteamUserStats; class ISteamFriends; class ISteamMatchmaking;
class ISteamNetworking; class ISteamUGC; class ISteamRemoteStorage; class ISteamUtils; class ISteamApps;
class ISteamMatchmakingPingResponse { public: virtual ~ISteamMatchmakingPingResponse() {} };
struct SteamServersConnected_t; struct SteamServersDisconnected_t; struct SteamServerConnectFailure_t;
struct SteamUGCQueryCompleted_t; struct SteamUGCRequestUGCDetailsResult_t;
struct SteamUGCDetails_t { std::uint64_t m_nPublishedFileId = 0; char m_rgchTitle[129] = {}; };

// ---- Windows-only types referenced by crash reporting and DirectInput code (not ported)
// Never pull <windows.h> in here: the game headers only need these names to exist.
struct _GUID { std::uint32_t Data1; std::uint16_t Data2, Data3; std::uint8_t Data4[8]; };
struct IUnknown { virtual ~IUnknown() {} };
struct HWND__;
struct __std_type_info_data { const char *_UndecoratedName; const char _DecoratedName[1]; };
struct _IMAGEHLP_LINE64; struct _IMAGEHLP_SYMBOL64; struct _tagSTACKFRAME64; struct _tagADDRESS64;
struct _CONTEXT; struct _EXCEPTION_POINTERS;
struct _DIDATAFORMAT; struct _DIACTIONFORMATA; struct _DIDEVICEIMAGEINFOHEADERA; struct _DICONFIGUREDEVICESPARAMSA;

// ---- imgui (editor tools only; not part of the port)
struct ImVec2 { float x, y; };
