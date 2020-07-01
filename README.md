## gmsv_voicehook

Experimental gmod module hooking into voice system. Also contains a VSCode Remote container setup for developing GMod modules for Linux SRCDS, if that's your thing.

Requires garrysmod_common in same folder.

Credits to Autogain author (https://forums.alliedmods.net/showthread.php?t=189527) and Metastruct

## Example

```lua
require "voicehook"

concommand.Add("recordme", function(ply)
    voicehook.Start(ply)
    timer.Simple(2, function()
        voicehook.End(ply)
        -- there is now voicedata file in garrysmod/data/<ply:EntIndex()>.dat
        -- which you can play with ffplay -f s16le -ar 44k 1.dat
    end)
end)

```

## Misc stuff

**Premake**
premake5 gmake2 --gmcommon=garrysmod_common --autoinstall=../../../srcds/garrysmod/lua/bin/

**Install GMod srcds (in dev container)**
/opt/steamcmd/steamcmd +login anonymous +force_install_dir /workspaces/gm_voicehook/srcds +app_update 4020 validate +quit

**Run GMod srcds (in dev container)**
../../../srcds/srcds_run +maxplayers 20 -console +gamemode sandbox +map gm_construct +sv_lan 1
