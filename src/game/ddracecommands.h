#ifndef GAME_SERVER_DDRACECOMMANDS_H
#define GAME_SERVER_DDRACECOMMANDS_H
#undef GAME_SERVER_DDRACECOMMANDS_H // this file can be included several times

#ifndef CONSOLE_COMMAND
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help, level)
#endif

CONSOLE_COMMAND("clear_votes", "", CFGFLAG_SERVER, ConClearVotes, this, "Clears the vote list", 4)
CONSOLE_COMMAND("kill", "v", CFGFLAG_SERVER, ConKillPlayer, this, "Kills player i and announces the kill", 2)
CONSOLE_COMMAND("logout", "v", CFGFLAG_SERVER, ConLogOut, this, "If you are a helper or didn't specify [i] it logs you out, otherwise it logs player i out", -1)
CONSOLE_COMMAND("helper", "v", CFGFLAG_SERVER, ConSetlvl1, this, "Authenticates player i to the Level of 1", 2)
CONSOLE_COMMAND("moder", "v", CFGFLAG_SERVER, ConSetlvl2, this, "Authenticates player i to the Level of 2", 3)
CONSOLE_COMMAND("admin", "v", CFGFLAG_SERVER, ConSetlvl3, this, "Authenticates player i to the Level of 3 (CAUTION: Irreversible, once he is an admin you can't control him)", 3)
CONSOLE_COMMAND("mute", "vi", CFGFLAG_SERVER, ConMute, this, "Mutes player i1 for i2 seconds", 2)
CONSOLE_COMMAND("invis", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConInvis, this, "Makes player i invisible", 2)
CONSOLE_COMMAND("vis", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConVis, this, "Makes player i visible again", 2)
CONSOLE_COMMAND("timerstop", "v", CFGFLAG_SERVER|CMDFLAG_TIMER, ConTimerStop, this, "Stops the timer of player i", 2)
CONSOLE_COMMAND("timerstart", "v", CFGFLAG_SERVER|CMDFLAG_TIMER, ConTimerStart, this, "Starts the timer of player i", 2)
CONSOLE_COMMAND("timerrestart", "v", CFGFLAG_SERVER|CMDFLAG_TIMER, ConTimerReStart, this, "Sets the timer of player i to 0 and starts it", 2)
CONSOLE_COMMAND("timerzero", "v", CFGFLAG_SERVER|CMDFLAG_TIMER, ConTimerZero, this, "Sets the timer of player i to 0 and stops it", 2)
CONSOLE_COMMAND("tele", "vi", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConTeleport, this, "Teleports player i1 to player i2", 2)
CONSOLE_COMMAND("freeze", "v?i", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConFreeze, this, "Freezes player i1 for i2 seconds (infinity by default)", 2)
CONSOLE_COMMAND("unfreeze", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConUnFreeze, this, "Unfreezes player i", 2)
CONSOLE_COMMAND("addweapon", "v?i", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConAddWeapon, this, "First optional parameter is client id, next parameter is weapon (all = -1, hammer = 0, gun = 1, shotgun = 2, grenade = 3, rifle = 4, ninja = 5)", 1)
CONSOLE_COMMAND("removeweapon", "v?i", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConRemoveWeapon, this, "First optional parameter is client id, next parameter is weapon (all = -1, hammer = 0, gun = 1, shotgun = 2, grenade = 3, rifle = 4)", 1)
CONSOLE_COMMAND("shotgun", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConShotgun, this, "Gives a shotgun to player i", 2)
CONSOLE_COMMAND("grenade", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConGrenade, this, "Gives a grenade launcher to player i", 2)
CONSOLE_COMMAND("rifle", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConRifle, this, "Gives a rifle to player i", 2)
CONSOLE_COMMAND("weapons", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConWeapons, this, "Gives all weapons to player i", 2)
CONSOLE_COMMAND("unshotgun", "v", CFGFLAG_SERVER|CMDFLAG_HELPERCMD, ConUnShotgun, this, "Takes a shotgun from player i", 2)
CONSOLE_COMMAND("ungrenade", "v", CFGFLAG_SERVER|CMDFLAG_HELPERCMD, ConUnGrenade, this, "Takes a grenade launcher from player i", 2)
CONSOLE_COMMAND("unrifle", "v", CFGFLAG_SERVER|CMDFLAG_HELPERCMD, ConUnRifle, this, "Takes a rifle from player i", 2)
CONSOLE_COMMAND("unweapons", "v", CFGFLAG_SERVER|CMDFLAG_HELPERCMD, ConUnWeapons, this, "Takes all weapons from player i", 2)
CONSOLE_COMMAND("ninja", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConNinja, this, "Makes player i a ninja", 2)
CONSOLE_COMMAND("hammer", "vi", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConHammer, this, "Sets the hammer power of player i1 to i2", 2)
CONSOLE_COMMAND("super", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConSuper, this, "Makes player i super", 2)
CONSOLE_COMMAND("unsuper", "v", CFGFLAG_SERVER|CMDFLAG_HELPERCMD, ConUnSuper, this, "Removes super from player i", 2)
CONSOLE_COMMAND("left", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConGoLeft, this, "Makes you or player i move 1 tile left", 1)
CONSOLE_COMMAND("right", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConGoRight, this, "Makes you or player i move 1 tile right", 1)
CONSOLE_COMMAND("up", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConGoUp, this, "Makes you or player i move 1 tile up", 1)
CONSOLE_COMMAND("down", "v", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConGoDown, this, "Makes you or player i move 1 tile down", 1)
CONSOLE_COMMAND("move", "vii", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConMove, this, "First optional parameter is client id, next parameters are x-axis change and y-axis change (1 = 1 tile)", 1)
CONSOLE_COMMAND("move_raw", "vii", CFGFLAG_SERVER|CMDFLAG_CHEAT|CMDFLAG_HELPERCMD, ConMoveRaw, this, "First optional parameter is client id, next parameters are x-axis change and y-axis change (1 = 1 pixel)", 1)
CONSOLE_COMMAND("broadtime", "", CFGFLAG_SERVER, ConBroadTime, this, "Toggles Showing the time string in race", -1)
CONSOLE_COMMAND("credits", "", CFGFLAG_SERVER, ConCredits, this, "Shows the credits of the DDRace mod", -1)
CONSOLE_COMMAND("emote", "?si", CFGFLAG_SERVER, ConEyeEmote, this, "Sets your tee's eye emote", -1)
CONSOLE_COMMAND("broadmsg", "", CFGFLAG_SERVER, ConToggleBroadcast, this, "Toggle Showing the Server's Broadcast message during race", -1)
CONSOLE_COMMAND("eyeemote", "", CFGFLAG_SERVER, ConEyeEmote, this, "Toggles whether you automatically use eyeemotes with standard emotes", -1)
CONSOLE_COMMAND("flags", "", CFGFLAG_SERVER, ConFlags, this, "Shows gameplay information for this server", -1)
CONSOLE_COMMAND("fly", "", CFGFLAG_SERVER, ConToggleFly, this, "Toggles whether you fly by pressing jump", 1)
CONSOLE_COMMAND("help", "?r", CFGFLAG_SERVER, ConHelp, this, "Helps you with commands", -1)
CONSOLE_COMMAND("info", "", CFGFLAG_SERVER, ConInfo, this, "Shows info about this server", -1)
CONSOLE_COMMAND("kill", "", CFGFLAG_SERVER, ConKill, this, "Kills you", -1)
CONSOLE_COMMAND("me", "r", CFGFLAG_SERVER, ConMe, this, "Like the famous irc commands /me says hi, will display YOURNAME says hi", -1)
CONSOLE_COMMAND("pause", "", CFGFLAG_SERVER, ConTogglePause, this, "If enabled on this server it pauses the game for you", -1)
CONSOLE_COMMAND("rank", "?r", CFGFLAG_SERVER, ConRank, this, "Shows either your rank or the rank of the given player", -1)
CONSOLE_COMMAND("rules", "", CFGFLAG_SERVER, ConRules, this, "Shows the rules of this server", -1)
//CONSOLE_COMMAND("team", "?i", CFGFLAG_SERVER, ConJoinTeam, this, "Lets you join the specified team", -1)
CONSOLE_COMMAND("top5", "?i", CFGFLAG_SERVER, ConTop5, this, "Shows the top 5 from the 1st, or starting at the specified number", -1)

#undef CONSOLE_COMMAND

#endif
