#include "stdint.h"
#include "sys/stat.h"
#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Symbol.hpp>
#include <scanning/symbolfinder.hpp>
#include <detouring/hook.hpp>
#include <Platform.hpp>
#include <steam/isteamclient.h>
#include <steam/isteamuser.h>

#define MAX_PLAYERS		129

static const char steamclient_name[] = "SteamClient012";

class IClient;

namespace global {

    SourceSDK::FactoryLoader engine_loader( "engine" );

    typedef void (* tBroadcastVoiceData ) ( IClient * , int , char * , int64  ) ;
    static Symbol sym_BroadcastVoiceData = Symbol::FromName("_Z21SV_BroadcastVoiceDataP7IClientiPcx");

    typedef int (* tGetPlayerSlot)( IClient * );
    tGetPlayerSlot fGetPlayerSlot = NULL;
    static Symbol sym_GetPlayerSlot = Symbol::FromName("_ZNK11CBaseClient13GetPlayerSlotEv");

    static Detouring::Hook bvd_hook;

    static int intercepts = 0;

    HSteamPipe g_hPipe;
    HSteamUser g_hUser;
    ISteamUser* g_user;

    bool hooking[MAX_PLAYERS];
    FILE *m_VoiceHookFiles[MAX_PLAYERS];

    LUA_FUNCTION_STATIC(GetIntercepts)
    {
        LUA->PushNumber(intercepts);
        return 1;
    }

    inline int GetEntityIndex( GarrysMod::Lua::ILuaBase *LUA, int i )
    {
        LUA->Push( i );
        LUA->GetField( -1, "EntIndex" );
        LUA->Push( -2 );
        LUA->Call( 1, 1 );

        return static_cast<int32_t>( LUA->GetNumber( -1 ) );
    }

    LUA_FUNCTION_STATIC( Start )
    {
        LUA->CheckType(1, GarrysMod::Lua::Type::Entity);
        int index = GetEntityIndex(LUA, 1);

        char fname[64];
        sprintf(fname, "garrysmod/data/voicehook/%d.dat", index);
        m_VoiceHookFiles[index] = fopen(fname, "ab");

        hooking[index] = true;
        return 0;
    }

    LUA_FUNCTION_STATIC( End )
    {
        LUA->CheckType(1, GarrysMod::Lua::Type::Entity);
        int index = GetEntityIndex(LUA, 1);

        hooking[index] = false;

        if (m_VoiceHookFiles[index]) {
            fclose(m_VoiceHookFiles[index]);
            m_VoiceHookFiles[index] = NULL;
        }

        return 0;
    }


    void hook_BroadcastVoiceData(IClient * pClient, int nBytes, char * data, int64 xuid)
    {
        bvd_hook.GetTrampoline<tBroadcastVoiceData>()(pClient, nBytes, data, xuid);
        intercepts++;
        
        if (pClient && nBytes && data) {
            int playerslot = fGetPlayerSlot(pClient);
            if (!hooking[playerslot]) {
                return;
            }

            FILE* voicefile = m_VoiceHookFiles[playerslot];

            int nVoiceBytes = nBytes;
            char *pVoiceData = data;

            if (g_user) {
                static char decompressed[20000];

                uint32 numUncompressedBytes = 0; 
                int res = g_user->DecompressVoice(pVoiceData, nVoiceBytes,
                    decompressed, sizeof(decompressed), &numUncompressedBytes, 44100 );

                if ( res == k_EVoiceResultOK && numUncompressedBytes > 0 )
                {
                    fwrite(decompressed, 1, numUncompressedBytes, voicefile);
                }
            }
        }
    }

    void start(GarrysMod::Lua::ILuaBase* LUA) {
        if (!engine_loader.IsValid( )) return;

        struct stat st;
        if (stat("garrysmod/data/voicehook", &st) == -1) {
            mkdir("garrysmod/data/voicehook", 0777);
        }

		SymbolFinder symfinder;
        tBroadcastVoiceData original_BroadcastVoiceData = reinterpret_cast<tBroadcastVoiceData>(symfinder.Resolve(
            engine_loader.GetModule( ),
            sym_BroadcastVoiceData.name.c_str( ),
            sym_BroadcastVoiceData.length
        ));
        if (!original_BroadcastVoiceData) {
            LUA->ThrowError( "unable to find BroadcastVoiceData" );
            return;
        }
		if( !bvd_hook.Create( reinterpret_cast<void *>( original_BroadcastVoiceData ),
			reinterpret_cast<void *>( &hook_BroadcastVoiceData ) ) )
			LUA->ThrowError( "unable to create detour for BroadcastVoiceData" );
        bvd_hook.Enable();

        fGetPlayerSlot = reinterpret_cast<tGetPlayerSlot>(symfinder.Resolve(
            engine_loader.GetModule( ),
            sym_GetPlayerSlot.name.c_str( ),
            sym_GetPlayerSlot.length
        ));
        if (!fGetPlayerSlot) {
            LUA->ThrowError( "unable to find GetPlayerSlot" );
        }

        SourceSDK::FactoryLoader* mod = new SourceSDK::FactoryLoader("steamclient");
        if (!mod->IsValid()) {
            LUA->ThrowError( "failed to find steamclient" );
            return;
        }

        ISteamClient* g_pSteamClient = mod->GetInterface<ISteamClient>(steamclient_name);
        if(!g_pSteamClient)
        {
            LUA->ThrowError( "failed to acquire steamclient pointer" );
            return;
        }

        g_hUser = g_pSteamClient->CreateLocalUser(&g_hPipe, k_EAccountTypeIndividual);
        g_user = g_pSteamClient->GetISteamUser(g_hUser, g_hPipe, "SteamUser020");

        LUA->CreateTable( );

		LUA->PushCFunction( GetIntercepts );
        LUA->SetField( -2, "InterceptCount" );

		LUA->PushCFunction( Start );
        LUA->SetField( -2, "Start" );

		LUA->PushCFunction( End );
        LUA->SetField( -2, "End" );

        LUA->SetField( GarrysMod::Lua::INDEX_GLOBAL, "voicehook" );
    }

    void stop(GarrysMod::Lua::ILuaBase* LUA) {
        bvd_hook.Destroy();
    }
}
GMOD_MODULE_OPEN()
{
	for (int i = 0; i < MAX_PLAYERS; i++) {
        global::m_VoiceHookFiles[i] = NULL;
        global::hooking[i] = false;
    }

    global::start(LUA);
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	for (int i = 0; i < MAX_PLAYERS; i++) {
        if (global::m_VoiceHookFiles[i]) {
            fclose(global::m_VoiceHookFiles[i]);
            global::m_VoiceHookFiles[i] = NULL;
        }
    }

    global::stop(LUA);

	return 0;
}