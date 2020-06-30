#include "stdint.h"
#include "sys/stat.h"
#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Symbol.hpp>
#include <scanning/symbolfinder.hpp>
#include <detouring/hook.hpp>
#include "voice_silk.h"

#define MAX_PLAYERS		129

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

    Voice_Silk *m_Silk[MAX_PLAYERS];
    FILE *m_VoiceHookFiles[MAX_PLAYERS];

    LUA_FUNCTION_STATIC(GetIntercepts)
    {
        LUA->PushNumber(intercepts);
        return 1;
    }

    LUA_FUNCTION_STATIC( EnableHook )
    {
        bool enabled = bvd_hook.Enable();
        LUA->PushBool(enabled);
        return 1;
    }

    LUA_FUNCTION_STATIC( DisableHook )
    {
        bool disabled = bvd_hook.Disable();
        LUA->PushBool(disabled);

        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (m_VoiceHookFiles[i]) {
                fclose(m_VoiceHookFiles[i]);
                m_VoiceHookFiles[i] = NULL;
            }
        }

        return 1;
    }


    void hook_BroadcastVoiceData(IClient * pClient, int nBytes, char * data, int64 xuid)
    {
        bvd_hook.GetTrampoline<tBroadcastVoiceData>()(pClient, nBytes, data, xuid);
        intercepts++;
        
        if (pClient && nBytes && data) {
            int playerslot = fGetPlayerSlot(pClient);

            FILE* voicefile = m_VoiceHookFiles[playerslot];
            if (!voicefile) {
                char fname[64];
                sprintf(fname, "voicehook/%d.dat", playerslot);
                voicefile = m_VoiceHookFiles[playerslot] = fopen(fname, "w+");
            }

            int nVoiceBytes = nBytes;
            char *pVoiceData = data;
            Voice *pVoiceCodec = NULL;
            
            // Ideally we would check the sv_use_steam_voice cvar here.
            // Instead we'll look for steam headers.
            if (nBytes >= 15)
            {
                unsigned int flags = ((data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7]);
                
                // Check for silk data.
                if (flags == 0x1001001)
                {
                    static const unsigned int headerSize = 11;
                    unsigned char nPayLoad = data[8];

                    if (nPayLoad != 11)
                        return; // Not the data we're looking for.

                    // Skip the steam header.
                    nVoiceBytes = nBytes - headerSize;
                    pVoiceData = &data[headerSize];

                    // Make sure this player has a codec loaded.
                    if (!m_Silk[playerslot])
                    {
                        m_Silk[playerslot] = new Voice_Silk;
                        m_Silk[playerslot]->Init();
                    }

                    pVoiceCodec = m_Silk[playerslot];
                }
            }

            if (pVoiceCodec) {
                // Decompress voice data.
                static char decompressed[8192];
                int nDecompressed = pVoiceCodec->Decompress(pVoiceData, nVoiceBytes, decompressed, sizeof(decompressed));
                if (nDecompressed) {
                    fwrite(decompressed, 1, nDecompressed * 2, voicefile);
                }
            }
        }
    }

    void start(GarrysMod::Lua::ILuaBase* LUA) {
        if (!engine_loader.IsValid( )) return;

        struct stat st;
        if (stat("voicehook", &st) == -1) {
            mkdir("voicehook", 0777);
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

        fGetPlayerSlot = reinterpret_cast<tGetPlayerSlot>(symfinder.Resolve(
            engine_loader.GetModule( ),
            sym_GetPlayerSlot.name.c_str( ),
            sym_GetPlayerSlot.length
        ));
        if (!fGetPlayerSlot) {
            LUA->ThrowError( "unable to find GetPlayerSlot" );
        }
        
        m_Silk[0] = new Voice_Silk;

        if (!m_Silk[0]->Init())
        {
            LUA->ThrowError( "unable to init first Silk" );
            return;
        }

        LUA->CreateTable( );

		LUA->PushCFunction( GetIntercepts );
        LUA->SetField( -2, "Intercepts" );

		LUA->PushCFunction( EnableHook );
        LUA->SetField( -2, "Enable" );

		LUA->PushCFunction( DisableHook );
        LUA->SetField( -2, "Disable" );

        LUA->SetField( GarrysMod::Lua::INDEX_GLOBAL, "voicehook" );
    }

    void stop(GarrysMod::Lua::ILuaBase* LUA) {
        bvd_hook.Destroy();
    }
}
GMOD_MODULE_OPEN()
{
	for (int i = 0; i < MAX_PLAYERS; i++) {
		global::m_Silk[i] = NULL;
        global::m_VoiceHookFiles[i] = NULL;
    }

    global::start(LUA);
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (global::m_Silk[i])
		{
			global::m_Silk[i]->Release();
			global::m_Silk[i] = NULL;
		}
        if (global::m_VoiceHookFiles[i]) {
            fclose(global::m_VoiceHookFiles[i]);
            global::m_VoiceHookFiles[i] = NULL;
        }
    }

    global::stop(LUA);

	return 0;
}