/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/demo.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <engine/sound.h>
#include <engine/serverbrowser.h>
#include <engine/shared/demo.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/localization.h>
#include <game/version.h>
#include "render.h"

#include "gameclient.h"

#include "components/binds.h"
#include "components/broadcast.h"
#include "components/camera.h"
#include "components/chat.h"
#include "components/console.h"
#include "components/controls.h"
#include "components/countryflags.h"
#include "components/damageind.h"
#include "components/debughud.h"
#include "components/effects.h"
#include "components/emoticon.h"
#include "components/flow.h"
#include "components/hud.h"
#include "components/items.h"
#include "components/killmessages.h"
#include "components/mapimages.h"
#include "components/maplayers.h"
#include "components/menus.h"
#include "components/motd.h"
#include "components/particles.h"
#include "components/players.h"
#include "components/nameplates.h"
#include "components/scoreboard.h"
#include "components/skins.h"
#include "components/sounds.h"
#include "components/spectator.h"
#include "components/voting.h"

CGameClient g_GameClient;

// instanciate all systems
static CKillMessages gs_KillMessages;
static CCamera gs_Camera;
static CChat gs_Chat;
static CMotd gs_Motd;
static CBroadcast gs_Broadcast;
static CGameConsole gs_GameConsole;
static CBinds gs_Binds;
static CParticles gs_Particles;
static CMenus gs_Menus;
static CSkins gs_Skins;
static CCountryFlags gs_CountryFlags;
static CFlow gs_Flow;
static CHud gs_Hud;
static CDebugHud gs_DebugHud;
static CControls gs_Controls;
static CEffects gs_Effects;
static CScoreboard gs_Scoreboard;
static CSounds gs_Sounds;
static CEmoticon gs_Emoticon;
static CDamageInd gsDamageInd;
static CVoting gs_Voting;
static CSpectator gs_Spectator;

static CPlayers gs_Players;
static CNamePlates gs_NamePlates;
static CItems gs_Items;
static CMapImages gs_MapImages;

static CMapLayers gs_MapLayersBackGround(CMapLayers::TYPE_BACKGROUND);
static CMapLayers gs_MapLayersForeGround(CMapLayers::TYPE_FOREGROUND);

CGameClient::CStack::CStack() { m_Num = 0; }
void CGameClient::CStack::Add(class CComponent *pComponent) { m_paComponents[m_Num++] = pComponent; }

const char *CGameClient::Version() { return GAME_VERSION; }
const char *CGameClient::NetVersion() { return GAME_NETVERSION; }
const char *CGameClient::GetItemName(int Type) { return m_NetObjHandler.GetObjName(Type); }

const char *CGameClient::GetTeamName(int Team, bool Teamplay) const
{
	if(Teamplay)
	{
		if(Team == TEAM_RED)
			return Localize("red team");
		else if(Team == TEAM_BLUE)
			return Localize("blue team");
	}
	else if(Team == 0)
		return Localize("game");

	return Localize("spectators");
}

enum
{
	DO_CHAT=0,
	DO_BROADCAST,
	DO_SPECIAL,

	PARA_NONE=0,
	PARA_I,
	PARA_II,
	PARA_III,
	PARA_S,
	PARA_SS,
	PARA_SSS,
};

struct CGameMsg
{
	int m_Action;
	int m_ParaType;
	const char *m_pText;
};

static CGameMsg gs_GameMsgList[NUM_GAMEMSGS] = {
	{/*GAMEMSG_TEAM_SWAP*/ DO_CHAT, PARA_NONE, "Teams were swapped"},
	{/*GAMEMSG_VOTE_ABORT*/ DO_CHAT, PARA_NONE, "Vote aborted"},
	{/*GAMEMSG_VOTE_PASS*/ DO_CHAT, PARA_NONE, "Vote passed"},
	{/*GAMEMSG_VOTE_FAIL*/ DO_CHAT, PARA_NONE, "Vote failed"},
	{/*GAMEMSG_VOTE_DENY_SPECCALL*/ DO_CHAT, PARA_NONE, "Spectators aren't allowed to start a vote."},
	{/*GAMEMSG_VOTE_DENY_ACTIVE*/ DO_CHAT, PARA_NONE, "Wait for current vote to end before calling a new one."},
	{/*GAMEMSG_VOTE_DENY_KICK*/ DO_CHAT, PARA_NONE, "Server does not allow voting to kick players"},
	{/*GAMEMSG_VOTE_DENY_KICKID*/ DO_CHAT, PARA_NONE, "Invalid client id to kick"},
	{/*GAMEMSG_VOTE_DENY_KICKSELF*/ DO_CHAT, PARA_NONE, "You can't kick yourself"},
	{/*GAMEMSG_VOTE_DENY_KICKADMIN*/ DO_CHAT, PARA_NONE, "You can't kick admins"},
	{/*GAMEMSG_VOTE_DENY_SPEC*/ DO_CHAT, PARA_NONE, "Server does not allow voting to move players to spectators"},
	{/*GAMEMSG_VOTE_DENY_SPECID*/ DO_CHAT, PARA_NONE, "Invalid client id to move"},
	{/*GAMEMSG_VOTE_DENY_SPECSELF*/ DO_CHAT, PARA_NONE, "You can't move yourself"},
	{/*GAMEMSG_SPEC_INVALIDID*/ DO_CHAT, PARA_NONE, "Invalid spectator id used"},
	{/*GAMEMSG_TEAM_SHUFFLE*/ DO_CHAT, PARA_NONE, "Teams were shuffled"},
	{/*GAMEMSG_TEAM_LOCK*/ DO_CHAT, PARA_NONE, "Teams were locked"},
	{/*GAMEMSG_TEAM_UNLOCK*/ DO_CHAT, PARA_NONE, "Teams were unlocked"},
	{/*GAMEMSG_TEAM_BALANCE*/ DO_CHAT, PARA_NONE, "Teams have been balanced"},
	{/*GAMEMSG_TEAM_DENY_LOCK*/ DO_BROADCAST, PARA_NONE, "Teams are locked"},
	{/*GAMEMSG_TEAM_DENY_BALANCE*/ DO_BROADCAST, PARA_NONE, "Teams must be balanced, please join other team"},
	{/*GAMEMSG_CTF_DROP*/ DO_SPECIAL, PARA_NONE, ""},	// special - play ctf drop sound
	{/*GAMEMSG_CTF_RETURN*/ DO_SPECIAL, PARA_NONE, ""},	// special - play ctf return sound

	{/*GAMEMSG_VOTE_DENY_WAIT*/ DO_CHAT, PARA_I, "You must wait %d seconds before making another vote"},
	{/*GAMEMSG_VOTE_DENY_KICKMIN*/ DO_CHAT, PARA_I, "Kick voting requires %d players on the server"},
	{/*GAMEMSG_TEAM_ALL*/ DO_SPECIAL, PARA_I, "All players were moved to the %s"},	// special - add team name
	{/*GAMEMSG_TEAM_DENY_MAX*/ DO_BROADCAST, PARA_I, "Only %d active players are allowed"},
	{/*GAMEMSG_TEAM_BALANCE_VICTIM*/ DO_SPECIAL, PARA_I, "You were moved to %s due to team balancing"},	// special - add team name
	{/*GAMEMSG_CTF_GRAB*/ DO_SPECIAL, PARA_I, ""},	// special - play ctf grab sound based on team

	{/*GAMEMSG_TEAM_DENY_WAIT*/ DO_BROADCAST, PARA_II, "Time to wait before changing team: %02d:%02d"},

	{/*GAMEMSG_CTF_CAPTURE*/ DO_SPECIAL, PARA_III, ""},	// special - play ctf capture sound + capture chat message

	{/*GAMEMSG_VOTE_DENY_INVALIDOP*/ DO_CHAT, PARA_S, "'%s' isn't an option on this server"},
	{/*GAMEMSG_VOTE_KICKYOU*/ DO_CHAT, PARA_S, "'%s' called for vote to kick you"},
	{/*GAMEMSG_VOTE_FORCE*/ DO_CHAT, PARA_S, "admin forced vote %s"},

	{/*GAMEMSG_VOTE_FORCEOP*/ DO_CHAT, PARA_SS, "admin forced server option '%s' (%s)"},
	{/*GAMEMSG_S2_VOTE_FORCESPEC*/ DO_CHAT, PARA_SS, "admin moved '%s' to spectator (%s)"},

	{/*GAMEMSG_VOTE_CALLOP*/ DO_CHAT, PARA_SSS, "'%s' called vote to change server option '%s' (%s)"},
	{/*GAMEMSG_VOTE_CALLKICK*/ DO_CHAT, PARA_SSS, "'%s' called for vote to kick '%s' (%s)"},
	{/*GAMEMSG_VOTE_CALLSPEC*/ DO_CHAT, PARA_SSS, "'%s' called for vote to move '%s' to spectators (%s)"}
};

void CGameClient::OnConsoleInit()
{
	m_pEngine = Kernel()->RequestInterface<IEngine>();
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pSound = Kernel()->RequestInterface<ISound>();
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pDemoPlayer = Kernel()->RequestInterface<IDemoPlayer>();
	m_pDemoRecorder = Kernel()->RequestInterface<IDemoRecorder>();
	m_pServerBrowser = Kernel()->RequestInterface<IServerBrowser>();
	m_pEditor = Kernel()->RequestInterface<IEditor>();
	m_pFriends = Kernel()->RequestInterface<IFriends>();

	// setup pointers
	m_pBinds = &::gs_Binds;
	m_pBroadcast = &::gs_Broadcast;
	m_pGameConsole = &::gs_GameConsole;
	m_pParticles = &::gs_Particles;
	m_pMenus = &::gs_Menus;
	m_pSkins = &::gs_Skins;
	m_pCountryFlags = &::gs_CountryFlags;
	m_pChat = &::gs_Chat;
	m_pFlow = &::gs_Flow;
	m_pCamera = &::gs_Camera;
	m_pControls = &::gs_Controls;
	m_pEffects = &::gs_Effects;
	m_pSounds = &::gs_Sounds;
	m_pMotd = &::gs_Motd;
	m_pDamageind = &::gsDamageInd;
	m_pMapimages = &::gs_MapImages;
	m_pVoting = &::gs_Voting;
	m_pScoreboard = &::gs_Scoreboard;
	m_pItems = &::gs_Items;
	m_pMapLayersBackGround = &::gs_MapLayersBackGround;
	m_pMapLayersForeGround = &::gs_MapLayersForeGround;

	// make a list of all the systems, make sure to add them in the corrent render order
	m_All.Add(m_pSkins);
	m_All.Add(m_pCountryFlags);
	m_All.Add(m_pMapimages);
	m_All.Add(m_pEffects); // doesn't render anything, just updates effects
	m_All.Add(m_pParticles); // doesn't render anything, just updates all the particles
	m_All.Add(m_pBinds);
	m_All.Add(m_pControls);
	m_All.Add(m_pCamera);
	m_All.Add(m_pSounds);
	m_All.Add(m_pVoting);

	m_All.Add(&gs_MapLayersBackGround); // first to render
	m_All.Add(&m_pParticles->m_RenderTrail);
	m_All.Add(m_pItems);
	m_All.Add(&gs_Players);
	m_All.Add(&gs_MapLayersForeGround);
	m_All.Add(&m_pParticles->m_RenderExplosions);
	m_All.Add(&gs_NamePlates);
	m_All.Add(&m_pParticles->m_RenderGeneral);
	m_All.Add(m_pDamageind);
	m_All.Add(&gs_Hud);
	m_All.Add(&gs_Spectator);
	m_All.Add(&gs_Emoticon);
	m_All.Add(&gs_KillMessages);
	m_All.Add(m_pChat);
	m_All.Add(&gs_Broadcast);
	m_All.Add(&gs_DebugHud);
	m_All.Add(&gs_Scoreboard);
	m_All.Add(m_pMotd);
	m_All.Add(m_pMenus);
	m_All.Add(m_pGameConsole);

	// build the input stack
	m_Input.Add(&m_pMenus->m_Binder); // this will take over all input when we want to bind a key
	m_Input.Add(&m_pBinds->m_SpecialBinds);
	m_Input.Add(m_pGameConsole);
	m_Input.Add(m_pChat); // chat has higher prio due to tha you can quit it by pressing esc
	m_Input.Add(m_pMotd); // for pressing esc to remove it
	m_Input.Add(m_pMenus);
	m_Input.Add(&gs_Spectator);
	m_Input.Add(&gs_Emoticon);
	m_Input.Add(m_pControls);
	m_Input.Add(m_pBinds);

	// add the some console commands
	Console()->Register("team", "i", CFGFLAG_CLIENT, ConTeam, this, "Switch team");
	Console()->Register("kill", "", CFGFLAG_CLIENT, ConKill, this, "Kill yourself");
	Console()->Register("ready_change", "", CFGFLAG_CLIENT, ConReadyChange, this, "Change ready state");

	Console()->Chain("add_friend", ConchainFriendUpdate, this);
	Console()->Chain("remove_friend", ConchainFriendUpdate, this);

	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->m_pClient = this;

	// let all the other components register their console commands
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnConsoleInit();

	//
	m_SuppressEvents = false;
}

void CGameClient::OnInit()
{
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();

	// propagate pointers
	m_UI.SetGraphics(Graphics(), TextRender());
	m_RenderTools.m_pGraphics = Graphics();
	m_RenderTools.m_pUI = UI();
	
	int64 Start = time_get();

	// set the language
	g_Localization.Load(g_Config.m_ClLanguagefile, Storage(), Console());

	// TODO: this should be different
	// setup item sizes
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Client()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	// load default font
	static CFont *pDefaultFont = 0;
	char aFilename[512];
	IOHANDLE File = Storage()->OpenFile("fonts/DejaVuSans.ttf", IOFLAG_READ, IStorage::TYPE_ALL, aFilename, sizeof(aFilename));
	if(File)
	{
		io_close(File);
		pDefaultFont = TextRender()->LoadFont(aFilename);
		TextRender()->SetDefaultFont(pDefaultFont);
	}
	if(!pDefaultFont)
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "gameclient", "failed to load font. filename='fonts/DejaVuSans.ttf'");

	// init all components
	for(int i = m_All.m_Num-1; i >= 0; --i)
		m_All.m_paComponents[i]->OnInit();

	// setup load amount// load textures
	for(int i = 0; i < g_pData->m_NumImages; i++)
	{
		g_pData->m_aImages[i].m_Id = Graphics()->LoadTexture(g_pData->m_aImages[i].m_pFilename, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		g_GameClient.m_pMenus->RenderLoading();
	}

	OnReset();

	int64 End = time_get();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "initialisation finished after %.2fms", ((End-Start)*1000)/(float)time_freq());
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "gameclient", aBuf);

	m_ServerMode = SERVERMODE_PURE;
}

void CGameClient::DispatchInput()
{
	// handle mouse movement
	float x = 0.0f, y = 0.0f;
	Input()->MouseRelative(&x, &y);
	if(x != 0.0f || y != 0.0f)
	{
		for(int h = 0; h < m_Input.m_Num; h++)
		{
			if(m_Input.m_paComponents[h]->OnMouseMove(x, y))
				break;
		}
	}

	// handle key presses
	for(int i = 0; i < Input()->NumEvents(); i++)
	{
		IInput::CEvent e = Input()->GetEvent(i);

		for(int h = 0; h < m_Input.m_Num; h++)
		{
			if(m_Input.m_paComponents[h]->OnInput(e))
			{
				//dbg_msg("", "%d char=%d key=%d flags=%d", h, e.ch, e.key, e.flags);
				break;
			}
		}
	}

	// clear all events for this frame
	Input()->ClearEvents();
}


int CGameClient::OnSnapInput(int *pData)
{
	return m_pControls->SnapInput(pData);
}

void CGameClient::OnConnected()
{
	m_Layers.Init(Kernel());
	m_Collision.Init(Layers());

	RenderTools()->RenderTilemapGenerateSkip(Layers());

	for(int i = 0; i < m_All.m_Num; i++)
	{
		m_All.m_paComponents[i]->OnMapLoad();
		m_All.m_paComponents[i]->OnReset();
	}

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	m_ServerMode = SERVERMODE_PURE;

	// send the inital info
	SendStartInfo();
}

void CGameClient::OnReset()
{
	// clear out the invalid pointers
	m_LastNewPredictedTick = -1;
	mem_zero(&g_GameClient.m_Snap, sizeof(g_GameClient.m_Snap));

	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aClients[i].Reset();

	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnReset();

	m_LocalClientID = -1;
	mem_zero(&m_GameInfo, sizeof(m_GameInfo));
	m_DemoSpecID = SPEC_FREEVIEW;
	m_Tuning = CTuningParams();
}


void CGameClient::UpdatePositions()
{
	// local character position
	if(g_Config.m_ClPredict && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(!m_Snap.m_pLocalCharacter ||
			(m_Snap.m_pGameData && m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_PAUSED|GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER)))
		{
			// don't use predicted
		}
		else
			m_LocalCharacterPos = mix(m_PredictedPrevChar.m_Pos, m_PredictedChar.m_Pos, Client()->PredIntraGameTick());
	}
	else if(m_Snap.m_pLocalCharacter && m_Snap.m_pLocalPrevCharacter)
	{
		m_LocalCharacterPos = mix(
			vec2(m_Snap.m_pLocalPrevCharacter->m_X, m_Snap.m_pLocalPrevCharacter->m_Y),
			vec2(m_Snap.m_pLocalCharacter->m_X, m_Snap.m_pLocalCharacter->m_Y), Client()->IntraGameTick());
	}

	// spectator position
	if(m_Snap.m_SpecInfo.m_Active)
	{
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK && DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER &&
			m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW)
		{
			m_Snap.m_SpecInfo.m_Position = mix(
				vec2(m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Prev.m_X, m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Prev.m_Y),
				vec2(m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Cur.m_X, m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Cur.m_Y),
				Client()->IntraGameTick());
			m_Snap.m_SpecInfo.m_UsePosition = true;
		}
		else if(m_Snap.m_pSpectatorInfo && (Client()->State() == IClient::STATE_DEMOPLAYBACK || m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW))
		{
			if(m_Snap.m_pPrevSpectatorInfo)
				m_Snap.m_SpecInfo.m_Position = mix(vec2(m_Snap.m_pPrevSpectatorInfo->m_X, m_Snap.m_pPrevSpectatorInfo->m_Y),
													vec2(m_Snap.m_pSpectatorInfo->m_X, m_Snap.m_pSpectatorInfo->m_Y), Client()->IntraGameTick());
			else
				m_Snap.m_SpecInfo.m_Position = vec2(m_Snap.m_pSpectatorInfo->m_X, m_Snap.m_pSpectatorInfo->m_Y);
			m_Snap.m_SpecInfo.m_UsePosition = true;
		}
	}
}


static void Evolve(CNetObj_Character *pCharacter, int Tick)
{
	CWorldCore TempWorld;
	CCharacterCore TempCore;
	mem_zero(&TempCore, sizeof(TempCore));
	TempCore.Init(&TempWorld, g_GameClient.Collision());
	TempCore.Read(pCharacter);

	while(pCharacter->m_Tick < Tick)
	{
		pCharacter->m_Tick++;
		TempCore.Tick(false);
		TempCore.Move();
		TempCore.Quantize();
	}

	TempCore.Write(pCharacter);
}


void CGameClient::OnRender()
{
	/*Graphics()->Clear(1,0,0);

	menus->render_background();
	return;*/
	/*
	Graphics()->Clear(1,0,0);
	Graphics()->MapScreen(0,0,100,100);

	Graphics()->QuadsBegin();
		Graphics()->SetColor(1,1,1,1);
		Graphics()->QuadsDraw(50, 50, 30, 30);
	Graphics()->QuadsEnd();

	return;*/

	// update the local character and spectate position
	UpdatePositions();

	// dispatch all input to systems
	DispatchInput();

	// render all systems
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnRender();

	// clear new tick flags
	m_NewTick = false;
	m_NewPredictedTick = false;
}

void CGameClient::OnRelease()
{
	// release all systems
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnRelease();
}

void CGameClient::OnMessage(int MsgId, CUnpacker *pUnpacker)
{
	Client()->RecordGameMessage(true);

	// special messages
	if(MsgId == NETMSGTYPE_SV_EXTRAPROJECTILE)
	{
		int Num = pUnpacker->GetInt();

		for(int k = 0; k < Num; k++)
		{
			CNetObj_Projectile Proj;
			for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
				((int *)&Proj)[i] = pUnpacker->GetInt();

			if(pUnpacker->Error())
				return;

			g_GameClient.m_pItems->AddExtraProjectile(&Proj);
		}

		return;
	}
	else if(MsgId == NETMSGTYPE_SV_TUNEPARAMS && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		Client()->RecordGameMessage(false);
		// unpack the new tuning
		CTuningParams NewTuning;
		int *pParams = (int *)&NewTuning;
		for(unsigned i = 0; i < sizeof(CTuningParams)/sizeof(int); i++)
			pParams[i] = pUnpacker->GetInt();

		// check for unpacking errors
		if(pUnpacker->Error())
			return;

		m_ServerMode = SERVERMODE_PURE;

		// apply new tuning
		m_Tuning = NewTuning;
		return;
	}
	else if(MsgId == NETMSGTYPE_SV_GAMEMSG)
	{
		int GameMsgID = pUnpacker->GetInt();

		int aParaI[3];
		int NumParaI = 0;
		const char *apParaS[3];
		int NumParaS = 0;

		// get paras
		switch(gs_GameMsgList[GameMsgID].m_ParaType)
		{
		case PARA_III:
			aParaI[NumParaI++] = pUnpacker->GetInt();
		case PARA_II:
			aParaI[NumParaI++] = pUnpacker->GetInt();
		case PARA_I:
			aParaI[NumParaI++] = pUnpacker->GetInt();
			break;
		case PARA_SSS:
			apParaS[NumParaS++] = pUnpacker->GetString();
		case PARA_SS:
			apParaS[NumParaS++] = pUnpacker->GetString();
		case PARA_S:
			apParaS[NumParaS++] = pUnpacker->GetString();
			break;
		}

		// check for unpacking errors
		if(pUnpacker->Error())
			return;

		// handle special messages
		static char aBuf[256];
		if(gs_GameMsgList[GameMsgID].m_Action == DO_SPECIAL)
		{
			switch(GameMsgID)
			{
			case GAMEMSG_CTF_DROP:
				if(m_SuppressEvents)
					return;
				m_pSounds->Enqueue(CSounds::CHN_GLOBAL, SOUND_CTF_DROP);
				break;
			case GAMEMSG_CTF_RETURN:
				if(m_SuppressEvents)
					return;
				m_pSounds->Enqueue(CSounds::CHN_GLOBAL, SOUND_CTF_RETURN);
				break;
			case GAMEMSG_TEAM_BALANCE_VICTIM:
				str_format(aBuf, sizeof(aBuf), Localize(gs_GameMsgList[GameMsgID].m_pText), GetTeamName(aParaI[0], m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS));
				m_pBroadcast->DoBroadcast(aBuf);
				break;
			case GAMEMSG_CTF_GRAB:
				if(m_SuppressEvents)
					return;
				if(m_LocalClientID != -1 && (m_aClients[m_LocalClientID].m_Team != aParaI[0] ||
					(m_Snap.m_SpecInfo.m_Active && m_Snap.m_SpecInfo.m_SpectatorID != -1 && m_aClients[m_Snap.m_SpecInfo.m_SpectatorID].m_Team != aParaI[0])))
					m_pSounds->Enqueue(CSounds::CHN_GLOBAL, SOUND_CTF_GRAB_PL);
				else
					m_pSounds->Enqueue(CSounds::CHN_GLOBAL, SOUND_CTF_GRAB_EN);
				break;
			case GAMEMSG_CTF_CAPTURE:
				m_pSounds->Enqueue(CSounds::CHN_GLOBAL, SOUND_CTF_CAPTURE);
				if(aParaI[2] <= 60*Client()->GameTickSpeed())
					str_format(aBuf, sizeof(aBuf), "The %s flag was captured by '%s' (%.2f seconds)", aParaI[0] ? "blue" : "red", m_aClients[clamp(aParaI[1], 0, MAX_CLIENTS-1)].m_aName, aParaI[2]/(float)Client()->GameTickSpeed());
				else
					str_format(aBuf, sizeof(aBuf), "The %s flag was captured by '%s'", aParaI[0] ? "blue" : "red", m_aClients[clamp(aParaI[1], 0, MAX_CLIENTS-1)].m_aName);
				m_pChat->AddLine(-1, 0, aBuf);
			}
			return;
		}

		// build message
		const char *pText = "";
		if(NumParaI == 0 && NumParaS == 0)
			pText = Localize(gs_GameMsgList[GameMsgID].m_pText);
		else
		{
			if(NumParaI == 1)
				str_format(aBuf, sizeof(aBuf), Localize(gs_GameMsgList[GameMsgID].m_pText), aParaI[0]);
			else if(NumParaI == 2)
				str_format(aBuf, sizeof(aBuf), Localize(gs_GameMsgList[GameMsgID].m_pText), aParaI[0], aParaI[1]);
			else if(NumParaI == 3)
				str_format(aBuf, sizeof(aBuf), Localize(gs_GameMsgList[GameMsgID].m_pText), aParaI[0], aParaI[1], aParaI[2]);
			else if(NumParaS == 1)
				str_format(aBuf, sizeof(aBuf), Localize(gs_GameMsgList[GameMsgID].m_pText), apParaS[0]);
			else if(NumParaS == 2)
				str_format(aBuf, sizeof(aBuf), Localize(gs_GameMsgList[GameMsgID].m_pText), apParaS[0], apParaS[1]);
			else if(NumParaS == 3)
				str_format(aBuf, sizeof(aBuf), Localize(gs_GameMsgList[GameMsgID].m_pText), apParaS[0], apParaS[1], apParaS[2]);
			pText = aBuf;
		}

		// handle message
		switch(gs_GameMsgList[GameMsgID].m_Action)
		{
		case DO_CHAT:
			m_pChat->AddLine(-1, 0, pText);
			break;
		case DO_BROADCAST:
			m_pBroadcast->DoBroadcast(pText);
		}
	}

	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgId, pUnpacker);
	if(!pRawMsg)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgId), MsgId, m_NetObjHandler.FailedMsgOn());
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
		return;
	}

	// TODO: this should be done smarter
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnMessage(MsgId, pRawMsg);

	if(MsgId == NETMSGTYPE_SV_CLIENTINFO && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		Client()->RecordGameMessage(false);
		CNetMsg_Sv_ClientInfo *pMsg = (CNetMsg_Sv_ClientInfo *)pRawMsg;

		if(pMsg->m_Local)
		{
			if(m_LocalClientID != -1)
			{
				if(g_Config.m_Debug)
					Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", "invalid local clientinfo");
				return;
			}
			m_LocalClientID = pMsg->m_ClientID;
		}
		else
		{
			if(m_aClients[pMsg->m_ClientID].m_Active)
			{
				if(g_Config.m_Debug)
					Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", "invalid clientinfo");
				return;
			}

			if(m_LocalClientID != -1)
			{
				DoEnterMessage(pMsg->m_pName, pMsg->m_Team);
				
				if(m_pDemoRecorder->IsRecording())
				{
					CNetMsg_De_ClientEnter Msg;
					Msg.m_pName = pMsg->m_pName;
					Msg.m_Team = pMsg->m_Team;
					Client()->SendPackMsg(&Msg, MSGFLAG_NOSEND|MSGFLAG_RECORD);
				}
			}
		}

		m_aClients[pMsg->m_ClientID].m_Active = true;
		m_aClients[pMsg->m_ClientID].m_Team  = pMsg->m_Team;
		str_copy(m_aClients[pMsg->m_ClientID].m_aName, pMsg->m_pName, sizeof(m_aClients[pMsg->m_ClientID].m_aName));
		str_copy(m_aClients[pMsg->m_ClientID].m_aClan, pMsg->m_pClan, sizeof(m_aClients[pMsg->m_ClientID].m_aClan));
		m_aClients[pMsg->m_ClientID].m_Country = pMsg->m_Country;
		for(int i = 0; i < 6; i++)
		{
			str_copy(m_aClients[pMsg->m_ClientID].m_aaSkinPartNames[i], pMsg->m_apSkinPartNames[i], 24);
			m_aClients[pMsg->m_ClientID].m_aUseCustomColors[i] = pMsg->m_aUseCustomColors[i];
			m_aClients[pMsg->m_ClientID].m_aSkinPartColors[i] = pMsg->m_aSkinPartColors[i];
		}

		// update friend state
		m_aClients[pMsg->m_ClientID].m_Friend = Friends()->IsFriend(m_aClients[pMsg->m_ClientID].m_aName, m_aClients[pMsg->m_ClientID].m_aClan, true);

		m_aClients[pMsg->m_ClientID].UpdateRenderInfo(true);

		m_GameInfo.m_NumPlayers++;
		// calculate team-balance
		if(m_aClients[pMsg->m_ClientID].m_Team != TEAM_SPECTATORS)
			m_GameInfo.m_aTeamSize[m_aClients[pMsg->m_ClientID].m_Team]++;
	}
	else if(MsgId == NETMSGTYPE_SV_CLIENTDROP && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		Client()->RecordGameMessage(false);
		CNetMsg_Sv_ClientDrop *pMsg = (CNetMsg_Sv_ClientDrop *)pRawMsg;

		if(m_LocalClientID == pMsg->m_ClientID || !m_aClients[pMsg->m_ClientID].m_Active)
		{
			if(g_Config.m_Debug)
				Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", "invalid clientdrop");
			return;
		}

		DoLeaveMessage(m_aClients[pMsg->m_ClientID].m_aName, pMsg->m_pReason);

		CNetMsg_De_ClientLeave Msg;
		Msg.m_pName = m_aClients[pMsg->m_ClientID].m_aName;
		Msg.m_pReason = pMsg->m_pReason;
		Client()->SendPackMsg(&Msg, MSGFLAG_NOSEND|MSGFLAG_RECORD);

		m_GameInfo.m_NumPlayers--;
		// calculate team-balance
		if(m_aClients[pMsg->m_ClientID].m_Team != TEAM_SPECTATORS)
			m_GameInfo.m_aTeamSize[m_aClients[pMsg->m_ClientID].m_Team]--;

		m_aClients[pMsg->m_ClientID].Reset();
	}
	else if(MsgId == NETMSGTYPE_SV_GAMEINFO && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		Client()->RecordGameMessage(false);
		CNetMsg_Sv_GameInfo *pMsg = (CNetMsg_Sv_GameInfo *)pRawMsg;

		m_GameInfo.m_GameFlags = pMsg->m_GameFlags;
		m_GameInfo.m_ScoreLimit = pMsg->m_ScoreLimit;
		m_GameInfo.m_TimeLimit = pMsg->m_TimeLimit;
		m_GameInfo.m_MatchNum = pMsg->m_MatchNum;
		m_GameInfo.m_MatchCurrent = pMsg->m_MatchCurrent;
	}
	else if(MsgId == NETMSGTYPE_SV_TEAM)
	{
		CNetMsg_Sv_Team *pMsg = (CNetMsg_Sv_Team *)pRawMsg;

		if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		{
			// calculate team-balance
			if(m_aClients[pMsg->m_ClientID].m_Team != TEAM_SPECTATORS)
				m_GameInfo.m_aTeamSize[m_aClients[pMsg->m_ClientID].m_Team]--;
			m_aClients[pMsg->m_ClientID].m_Team = pMsg->m_Team;
			if(m_aClients[pMsg->m_ClientID].m_Team != TEAM_SPECTATORS)
				m_GameInfo.m_aTeamSize[m_aClients[pMsg->m_ClientID].m_Team]++;

			m_aClients[pMsg->m_ClientID].UpdateRenderInfo(false);
		}

		if(pMsg->m_Silent == 0)
		{
			DoTeamChangeMessage(m_aClients[pMsg->m_ClientID].m_aName, pMsg->m_Team);
		}		
	}
	else if(MsgId == NETMSGTYPE_SV_READYTOENTER)
	{
		Client()->EnterGame();
	}
	else if (MsgId == NETMSGTYPE_SV_EMOTICON)
	{
		CNetMsg_Sv_Emoticon *pMsg = (CNetMsg_Sv_Emoticon *)pRawMsg;

		// apply
		m_aClients[pMsg->m_ClientID].m_Emoticon = pMsg->m_Emoticon;
		m_aClients[pMsg->m_ClientID].m_EmoticonStart = Client()->GameTick();
	}
	else if(MsgId == NETMSGTYPE_DE_SOUNDGLOBAL)
	{
		if(m_SuppressEvents)
			return;

		CNetMsg_De_SoundGlobal *pMsg = (CNetMsg_De_SoundGlobal *)pRawMsg;
		g_GameClient.m_pSounds->Play(CSounds::CHN_GLOBAL, pMsg->m_SoundID, 1.0f);
	}
	else if(MsgId == NETMSGTYPE_DE_CLIENTENTER && Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		CNetMsg_De_ClientEnter *pMsg = (CNetMsg_De_ClientEnter *)pRawMsg;
		DoEnterMessage(pMsg->m_pName, pMsg->m_Team);
	}
	else if(MsgId == NETMSGTYPE_DE_CLIENTLEAVE && Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		CNetMsg_De_ClientLeave *pMsg = (CNetMsg_De_ClientLeave *)pRawMsg;
		DoLeaveMessage(pMsg->m_pName, pMsg->m_pReason);
	}
}

void CGameClient::OnStateChange(int NewState, int OldState)
{
	// reset everything when not already connected (to keep gathered stuff)
	if(NewState < IClient::STATE_ONLINE)
		OnReset();

	// then change the state
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnStateChange(NewState, OldState);
}

void CGameClient::OnShutdown() {}
void CGameClient::OnEnterGame() {}

void CGameClient::OnGameOver()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK && g_Config.m_ClEditor == 0)
		Client()->AutoScreenshot_Start();
}

void CGameClient::OnStartGame()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Client()->DemoRecorder_HandleAutoStart();
}

void CGameClient::OnRconLine(const char *pLine)
{
	m_pGameConsole->PrintLine(CGameConsole::CONSOLETYPE_REMOTE, pLine);
}

void CGameClient::ProcessEvents()
{
	if(m_SuppressEvents)
		return;

	int SnapType = IClient::SNAP_CURRENT;
	int Num = Client()->SnapNumItems(SnapType);
	for(int Index = 0; Index < Num; Index++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(SnapType, Index, &Item);

		if(Item.m_Type == NETEVENTTYPE_DAMAGEIND)
		{
			CNetEvent_DamageInd *ev = (CNetEvent_DamageInd *)pData;
			g_GameClient.m_pEffects->DamageIndicator(vec2(ev->m_X, ev->m_Y), GetDirection(ev->m_Angle));
		}
		else if(Item.m_Type == NETEVENTTYPE_EXPLOSION)
		{
			CNetEvent_Explosion *ev = (CNetEvent_Explosion *)pData;
			g_GameClient.m_pEffects->Explosion(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_HAMMERHIT)
		{
			CNetEvent_HammerHit *ev = (CNetEvent_HammerHit *)pData;
			g_GameClient.m_pEffects->HammerHit(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_SPAWN)
		{
			CNetEvent_Spawn *ev = (CNetEvent_Spawn *)pData;
			g_GameClient.m_pEffects->PlayerSpawn(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_DEATH)
		{
			CNetEvent_Death *ev = (CNetEvent_Death *)pData;
			g_GameClient.m_pEffects->PlayerDeath(vec2(ev->m_X, ev->m_Y), ev->m_ClientID);
		}
		else if(Item.m_Type == NETEVENTTYPE_SOUNDWORLD)
		{
			CNetEvent_SoundWorld *ev = (CNetEvent_SoundWorld *)pData;
			g_GameClient.m_pSounds->PlayAt(CSounds::CHN_WORLD, ev->m_SoundID, 1.0f, vec2(ev->m_X, ev->m_Y));
		}
	}
}

void CGameClient::OnNewSnapshot()
{
	m_NewTick = true;

	// clear out the invalid pointers
	mem_zero(&g_GameClient.m_Snap, sizeof(g_GameClient.m_Snap));

	// secure snapshot
	{
		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int Index = 0; Index < Num; Index++)
		{
			IClient::CSnapItem Item;
			void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, Index, &Item);
			if(m_NetObjHandler.ValidateObj(Item.m_Type, pData, Item.m_DataSize) != 0)
			{
				if(g_Config.m_Debug)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "invalidated index=%d type=%d (%s) size=%d id=%d", Index, Item.m_Type, m_NetObjHandler.GetObjName(Item.m_Type), Item.m_DataSize, Item.m_ID);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
				}
				Client()->SnapInvalidateItem(IClient::SNAP_CURRENT, Index);
			}
		}
	}

	ProcessEvents();

	if(g_Config.m_DbgStress)
	{
		if((Client()->GameTick()%100) == 0)
		{
			char aMessage[64];
			int MsgLen = rand()%(sizeof(aMessage)-1);
			for(int i = 0; i < MsgLen; i++)
				aMessage[i] = 'a'+(rand()%('z'-'a'));
			aMessage[MsgLen] = 0;

			CNetMsg_Cl_Say Msg;
			Msg.m_Team = rand()&1;
			Msg.m_pMessage = aMessage;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		}
	}

	CTuningParams StandardTuning;
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_Tuning = StandardTuning;
		mem_zero(&m_GameInfo, sizeof(m_GameInfo));
	}

	// go trough all the items in the snapshot and gather the info we want
	{
		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int i = 0; i < Num; i++)
		{
			IClient::CSnapItem Item;
			const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

			// demo items
			if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
			{
				if(Item.m_Type == NETOBJTYPE_DE_CLIENTINFO)
				{
					const CNetObj_De_ClientInfo *pInfo = (const CNetObj_De_ClientInfo *)pData;
					int ClientID = Item.m_ID;
					CClientData *pClient = &m_aClients[ClientID];

					if(pInfo->m_Local)
						m_LocalClientID = ClientID;
					pClient->m_Active = true;
					pClient->m_Team  = pInfo->m_Team;
					IntsToStr(pInfo->m_aName, 4, pClient->m_aName);
					IntsToStr(pInfo->m_aClan, 3, pClient->m_aClan);
					pClient->m_Country = pInfo->m_Country;

					for(int p = 0; p < NUM_SKINPARTS; p++)
					{
						IntsToStr(pInfo->m_aaSkinPartNames[p], 6, pClient->m_aaSkinPartNames[p]);
						pClient->m_aUseCustomColors[p] = pInfo->m_aUseCustomColors[p];
						pClient->m_aSkinPartColors[p] = pInfo->m_aSkinPartColors[p];
					}

					m_GameInfo.m_NumPlayers++;
					// calculate team-balance
					if(pClient->m_Team != TEAM_SPECTATORS)
						m_GameInfo.m_aTeamSize[pClient->m_Team]++;
				}
				else if(Item.m_Type == NETOBJTYPE_DE_GAMEINFO)
				{
					const CNetObj_De_GameInfo *pInfo = (const CNetObj_De_GameInfo *)pData;

					m_GameInfo.m_GameFlags = pInfo->m_GameFlags;
					m_GameInfo.m_ScoreLimit = pInfo->m_ScoreLimit;
					m_GameInfo.m_TimeLimit = pInfo->m_TimeLimit;
					m_GameInfo.m_MatchNum = pInfo->m_MatchNum;
					m_GameInfo.m_MatchCurrent = pInfo->m_MatchCurrent;
				}
				else if(Item.m_Type == NETOBJTYPE_DE_TUNEPARAMS)
				{
					const CNetObj_De_TuneParams *pInfo = (const CNetObj_De_TuneParams *)pData;

					mem_copy(&m_Tuning, pInfo->m_aTuneParams, sizeof(m_Tuning));
					m_ServerMode = SERVERMODE_PURE;
				}
			}
			
			// network items
			if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
			{
				const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;
				int ClientID = Item.m_ID;
				if(m_aClients[ClientID].m_Active)
				{
					m_Snap.m_paPlayerInfos[ClientID] = pInfo;
					m_Snap.m_aInfoByScore[ClientID].m_pPlayerInfo = pInfo;
					m_Snap.m_aInfoByScore[ClientID].m_ClientID = ClientID;

					if(m_LocalClientID == ClientID)
					{
						m_Snap.m_pLocalInfo = pInfo;

						if(m_aClients[ClientID].m_Team == TEAM_SPECTATORS)
						{
							m_Snap.m_SpecInfo.m_Active = true;
							m_Snap.m_SpecInfo.m_SpectatorID = SPEC_FREEVIEW;
						}
					}
				}
			}
			else if(Item.m_Type == NETOBJTYPE_CHARACTER)
			{
				const void *pOld = Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_CHARACTER, Item.m_ID);
				m_Snap.m_aCharacters[Item.m_ID].m_Cur = *((const CNetObj_Character *)pData);

				// clamp ammo count for non ninja weapon
				if(m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_Weapon != WEAPON_NINJA)
					m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_AmmoCount = clamp(m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_AmmoCount, 0, 10);
				
				if(pOld)
				{
					m_Snap.m_aCharacters[Item.m_ID].m_Active = true;
					m_Snap.m_aCharacters[Item.m_ID].m_Prev = *((const CNetObj_Character *)pOld);

					if(m_Snap.m_aCharacters[Item.m_ID].m_Prev.m_Tick)
						Evolve(&m_Snap.m_aCharacters[Item.m_ID].m_Prev, Client()->PrevGameTick());
					if(m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_Tick)
						Evolve(&m_Snap.m_aCharacters[Item.m_ID].m_Cur, Client()->GameTick());
				}
			}
			else if(Item.m_Type == NETOBJTYPE_SPECTATORINFO)
			{
				m_Snap.m_pSpectatorInfo = (const CNetObj_SpectatorInfo *)pData;
				m_Snap.m_pPrevSpectatorInfo = (const CNetObj_SpectatorInfo *)Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_SPECTATORINFO, Item.m_ID);
				m_Snap.m_SpecInfo.m_Active = true;
				m_Snap.m_SpecInfo.m_SpectatorID = m_Snap.m_pSpectatorInfo->m_SpectatorID;
			}
			else if(Item.m_Type == NETOBJTYPE_GAMEDATA)
			{
				m_Snap.m_pGameData = (const CNetObj_GameData *)pData;

				static bool s_GameOver = 0;
				if(!s_GameOver && m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
					OnGameOver();
				else if(s_GameOver && !(m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER))
					OnStartGame();
				s_GameOver = m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER;
			}
			else if(Item.m_Type == NETOBJTYPE_GAMEDATATEAM)
			{
				m_Snap.m_pGameDataTeam = (const CNetObj_GameDataTeam *)pData;
			}
			else if(Item.m_Type == NETOBJTYPE_GAMEDATAFLAG)
			{
				m_Snap.m_pGameDataFlag = (const CNetObj_GameDataFlag *)pData;
				m_Snap.m_GameDataFlagSnapID = Item.m_ID;
			}
			else if(Item.m_Type == NETOBJTYPE_FLAG)
				m_Snap.m_paFlags[Item.m_ID%2] = (const CNetObj_Flag *)pData;
		}
	}

	// setup local pointers
	if(m_LocalClientID >= 0)
	{
		CSnapState::CCharacterInfo *c = &m_Snap.m_aCharacters[m_LocalClientID];
		if(c->m_Active)
		{
			m_Snap.m_pLocalCharacter = &c->m_Cur;
			m_Snap.m_pLocalPrevCharacter = &c->m_Prev;
			m_LocalCharacterPos = vec2(m_Snap.m_pLocalCharacter->m_X, m_Snap.m_pLocalCharacter->m_Y);
		}
		else if(Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_CHARACTER, m_LocalClientID))
		{
			// player died
			m_pControls->OnPlayerDeath();
		}
	}
	else
	{
		m_Snap.m_SpecInfo.m_Active = true;
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK && DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER &&
			m_DemoSpecID != SPEC_FREEVIEW && m_Snap.m_aCharacters[m_DemoSpecID].m_Active)
			m_Snap.m_SpecInfo.m_SpectatorID = m_DemoSpecID;
		else
			m_Snap.m_SpecInfo.m_SpectatorID = SPEC_FREEVIEW;
	}

	// sort player infos by score
	for(int k = 0; k < MAX_CLIENTS-1; k++) // ffs, bubblesort
	{
		for(int i = 0; i < MAX_CLIENTS-k-1; i++)
		{
			if(m_Snap.m_aInfoByScore[i+1].m_pPlayerInfo && (!m_Snap.m_aInfoByScore[i].m_pPlayerInfo ||
				m_Snap.m_aInfoByScore[i].m_pPlayerInfo->m_Score < m_Snap.m_aInfoByScore[i+1].m_pPlayerInfo->m_Score))
			{
				CPlayerInfoItem Tmp = m_Snap.m_aInfoByScore[i];
				m_Snap.m_aInfoByScore[i] = m_Snap.m_aInfoByScore[i+1];
				m_Snap.m_aInfoByScore[i+1] = Tmp;
			}
		}
	}

	// calc some player stats
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_Snap.m_paPlayerInfos[i])
			continue;

		// count not ready players
		if((m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_STARTCOUNTDOWN|GAMESTATEFLAG_PAUSED|GAMESTATEFLAG_WARMUP)) &&
			m_Snap.m_pGameData->m_GameStateEndTick == 0 && m_aClients[i].m_Team != TEAM_SPECTATORS && !(m_Snap.m_paPlayerInfos[i]->m_PlayerFlags&PLAYERFLAG_READY))
			m_Snap.m_NotReadyCount++;

		// count alive players per team
		if((m_GameInfo.m_GameFlags&GAMEFLAG_SURVIVAL) && m_aClients[i].m_Team != TEAM_SPECTATORS && !(m_Snap.m_paPlayerInfos[i]->m_PlayerFlags&PLAYERFLAG_DEAD))
			m_Snap.m_AliveCount[m_aClients[i].m_Team]++;
	}

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(m_aClients[i].m_Active)
				m_aClients[i].UpdateRenderInfo(true);
		}
	}

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	if(CurrentServerInfo.m_aGameType[0] != '0')
	{
		if(str_comp(CurrentServerInfo.m_aGameType, "DM") != 0 && str_comp(CurrentServerInfo.m_aGameType, "TDM") != 0 && str_comp(CurrentServerInfo.m_aGameType, "CTF") != 0 &&
			str_comp(CurrentServerInfo.m_aGameType, "LMS") != 0 && str_comp(CurrentServerInfo.m_aGameType, "SUR") != 0)
			m_ServerMode = SERVERMODE_MOD;
		else if(mem_comp(&StandardTuning, &m_Tuning, sizeof(CTuningParams)) == 0)
			m_ServerMode = SERVERMODE_PURE;
		else
			m_ServerMode = SERVERMODE_PUREMOD;
	}
}

void CGameClient::OnDemoRecSnap()
{
	// add client info
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_aClients[i].m_Active)
			continue;

		CNetObj_De_ClientInfo *pClientInfo = static_cast<CNetObj_De_ClientInfo *>(Client()->SnapNewItem(NETOBJTYPE_DE_CLIENTINFO, i, sizeof(CNetObj_De_ClientInfo)));
		if(!pClientInfo)
			return;

		pClientInfo->m_Local = i==m_LocalClientID ? 1 : 0;
		pClientInfo->m_Team = m_aClients[i].m_Team;
		StrToInts(pClientInfo->m_aName, 4, m_aClients[i].m_aName);
		StrToInts(pClientInfo->m_aClan, 3, m_aClients[i].m_aClan);
		pClientInfo->m_Country = m_aClients[i].m_Country;

		for(int p = 0; p < 6; p++)
		{
			StrToInts(pClientInfo->m_aaSkinPartNames[p], 6, m_aClients[i].m_aaSkinPartNames[p]);
			pClientInfo->m_aUseCustomColors[p] = m_aClients[i].m_aUseCustomColors[p];
			pClientInfo->m_aSkinPartColors[p] = m_aClients[i].m_aSkinPartColors[p];
		}
	}

	// add tuning
	CTuningParams StandardTuning;
	if(mem_comp(&StandardTuning, &m_Tuning, sizeof(CTuningParams)) != 0)
	{
		CNetObj_De_TuneParams *pTuneParams = static_cast<CNetObj_De_TuneParams *>(Client()->SnapNewItem(NETOBJTYPE_DE_TUNEPARAMS, 0, sizeof(CNetObj_De_TuneParams)));
		if(!pTuneParams)
			return;

		mem_copy(pTuneParams->m_aTuneParams, &m_Tuning, sizeof(pTuneParams->m_aTuneParams));
	}

	// add game info
	CNetObj_De_GameInfo *pGameInfo = static_cast<CNetObj_De_GameInfo *>(Client()->SnapNewItem(NETOBJTYPE_DE_GAMEINFO, 0, sizeof(CNetObj_De_GameInfo)));
	if(!pGameInfo)
		return;
	
	pGameInfo->m_GameFlags = m_GameInfo.m_GameFlags;
	pGameInfo->m_ScoreLimit = m_GameInfo.m_ScoreLimit;
	pGameInfo->m_TimeLimit = m_GameInfo.m_TimeLimit;
	pGameInfo->m_MatchNum = m_GameInfo.m_MatchNum;
	pGameInfo->m_MatchCurrent = m_GameInfo.m_MatchCurrent;
}

void CGameClient::OnPredict()
{
	// store the previous values so we can detect prediction errors
	CCharacterCore BeforePrevChar = m_PredictedPrevChar;
	CCharacterCore BeforeChar = m_PredictedChar;

	// we can't predict without our own id or own character
	if(m_LocalClientID == -1 || !m_Snap.m_aCharacters[m_LocalClientID].m_Active)
		return;

	// don't predict anything if we are paused or round/game is over
	if(m_Snap.m_pGameData && m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_PAUSED|GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER))
	{
		if(m_Snap.m_pLocalCharacter)
			m_PredictedChar.Read(m_Snap.m_pLocalCharacter);
		if(m_Snap.m_pLocalPrevCharacter)
			m_PredictedPrevChar.Read(m_Snap.m_pLocalPrevCharacter);
		return;
	}

	// repredict character
	CWorldCore World;
	World.m_Tuning = m_Tuning;

	// search for players
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_Snap.m_aCharacters[i].m_Active)
			continue;

		g_GameClient.m_aClients[i].m_Predicted.Init(&World, Collision());
		World.m_apCharacters[i] = &g_GameClient.m_aClients[i].m_Predicted;
		g_GameClient.m_aClients[i].m_Predicted.Read(&m_Snap.m_aCharacters[i].m_Cur);
	}

	// predict
	for(int Tick = Client()->GameTick()+1; Tick <= Client()->PredGameTick(); Tick++)
	{
		// fetch the local
		if(Tick == Client()->PredGameTick() && World.m_apCharacters[m_LocalClientID])
			m_PredictedPrevChar = *World.m_apCharacters[m_LocalClientID];

		// first calculate where everyone should move
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!World.m_apCharacters[c])
				continue;

			mem_zero(&World.m_apCharacters[c]->m_Input, sizeof(World.m_apCharacters[c]->m_Input));
			if(m_LocalClientID == c)
			{
				// apply player input
				int *pInput = Client()->GetInput(Tick);
				if(pInput)
					World.m_apCharacters[c]->m_Input = *((CNetObj_PlayerInput*)pInput);
				World.m_apCharacters[c]->Tick(true);
			}
			else
				World.m_apCharacters[c]->Tick(false);

		}

		// move all players and quantize their data
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!World.m_apCharacters[c])
				continue;

			World.m_apCharacters[c]->Move();
			World.m_apCharacters[c]->Quantize();
		}

		// check if we want to trigger effects
		if(Tick > m_LastNewPredictedTick)
		{
			m_LastNewPredictedTick = Tick;
			m_NewPredictedTick = true;

			if(m_LocalClientID != -1 && World.m_apCharacters[m_LocalClientID])
			{
				vec2 Pos = World.m_apCharacters[m_LocalClientID]->m_Pos;
				int Events = World.m_apCharacters[m_LocalClientID]->m_TriggeredEvents;
				if(Events&COREEVENT_GROUND_JUMP) g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, Pos);

				/*if(events&COREEVENT_AIR_JUMP)
				{
					GameClient.effects->air_jump(pos);
					GameClient.sounds->play_and_record(SOUNDS::CHN_WORLD, SOUND_PLAYER_AIRJUMP, 1.0f, pos);
				}*/

				//if(events&COREEVENT_HOOK_LAUNCH) snd_play_random(CHN_WORLD, SOUND_HOOK_LOOP, 1.0f, pos);
				//if(events&COREEVENT_HOOK_ATTACH_PLAYER) snd_play_random(CHN_WORLD, SOUND_HOOK_ATTACH_PLAYER, 1.0f, pos);
				if(Events&COREEVENT_HOOK_ATTACH_GROUND) g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_HOOK_ATTACH_GROUND, 1.0f, Pos);
				if(Events&COREEVENT_HOOK_HIT_NOHOOK) g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_HOOK_NOATTACH, 1.0f, Pos);
				//if(events&COREEVENT_HOOK_RETRACT) snd_play_random(CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
			}
		}

		if(Tick == Client()->PredGameTick() && World.m_apCharacters[m_LocalClientID])
			m_PredictedChar = *World.m_apCharacters[m_LocalClientID];
	}

	if(g_Config.m_Debug && g_Config.m_ClPredict && m_PredictedTick == Client()->PredGameTick())
	{
		CNetObj_CharacterCore Before = {0}, Now = {0}, BeforePrev = {0}, NowPrev = {0};
		BeforeChar.Write(&Before);
		BeforePrevChar.Write(&BeforePrev);
		m_PredictedChar.Write(&Now);
		m_PredictedPrevChar.Write(&NowPrev);

		if(mem_comp(&Before, &Now, sizeof(CNetObj_CharacterCore)) != 0)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "prediction error");
			for(unsigned i = 0; i < sizeof(CNetObj_CharacterCore)/sizeof(int); i++)
				if(((int *)&Before)[i] != ((int *)&Now)[i])
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "	%d %d %d (%d %d)", i, ((int *)&Before)[i], ((int *)&Now)[i], ((int *)&BeforePrev)[i], ((int *)&NowPrev)[i]);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
				}
		}
	}

	m_PredictedTick = Client()->PredGameTick();
}

void CGameClient::OnActivateEditor()
{
	OnRelease();
}

void CGameClient::CClientData::UpdateRenderInfo(bool UpdateSkinInfo)
{
	// update skin info
	if(UpdateSkinInfo)
	{
		m_SkinInfo.m_Size = 64;

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			if(m_aaSkinPartNames[p][0] == 'x' && m_aaSkinPartNames[p][1] == '_')
				str_copy(m_aaSkinPartNames[p], "default", 24);

			m_SkinPartIDs[p] = g_GameClient.m_pSkins->FindSkinPart(p, m_aaSkinPartNames[p]);
			if(m_SkinPartIDs[p] < 0)
			{
				m_SkinPartIDs[p] = g_GameClient.m_pSkins->Find("default");
				if(m_SkinPartIDs[p] < 0)
					m_SkinPartIDs[p] = 0;
			}

			const CSkins::CSkinPart *pSkinPart = g_GameClient.m_pSkins->GetSkinPart(p, m_SkinPartIDs[p]);
			if(m_aUseCustomColors[p])
			{
				m_SkinInfo.m_aTextures[p] = pSkinPart->m_ColorTexture;
				m_SkinInfo.m_aColors[p] = g_GameClient.m_pSkins->GetColorV4(m_aSkinPartColors[p], p==SKINPART_TATTOO);
			}
			else
			{
				m_SkinInfo.m_aTextures[p] = pSkinPart->m_OrgTexture;
				m_SkinInfo.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
	}

	m_RenderInfo = m_SkinInfo;

	// force team colors
	if(g_GameClient.m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
	{
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			m_RenderInfo.m_aTextures[p] = g_GameClient.m_pSkins->GetSkinPart(p, m_SkinPartIDs[p])->m_ColorTexture;
			int ColorVal = g_GameClient.m_pSkins->GetTeamColor(m_aUseCustomColors[p], m_aSkinPartColors[p], m_Team, p);
			m_RenderInfo.m_aColors[p] = g_GameClient.m_pSkins->GetColorV4(ColorVal, p==SKINPART_TATTOO);
		}
	}
}

void CGameClient::CClientData::Reset()
{
	m_aName[0] = 0;
	m_aClan[0] = 0;
	m_Country = -1;
	m_Team = 0;
	m_Angle = 0;
	m_Emoticon = 0;
	m_EmoticonStart = -1;
	m_Active = false;
	m_ChatIgnore = false;
	m_Friend = false;
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		m_SkinPartIDs[p] = 0;
		m_SkinInfo.m_aTextures[p] = g_GameClient.m_pSkins->GetSkinPart(p, 0)->m_ColorTexture;
		m_SkinInfo.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f , 1.0f);
	}
	UpdateRenderInfo(false);
}

void CGameClient::DoEnterMessage(const char *pName, int Team)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), Localize("'%s' entered and joined the %s"), pName, GetTeamName(Team, m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS));
	m_pChat->AddLine(-1, 0, aBuf);
}

void CGameClient::DoLeaveMessage(const char *pName, const char *pReason)
{
	char aBuf[128];
	if(pReason[0])
		str_format(aBuf, sizeof(aBuf), Localize("'%s' has left the game (%s)"), pName, pReason);
	else
		str_format(aBuf, sizeof(aBuf), Localize("'%s' has left the game"), pName);
	m_pChat->AddLine(-1, 0, aBuf);
}

void CGameClient::DoTeamChangeMessage(const char *pName, int Team)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), Localize("'%s' joined the %s"), pName, GetTeamName(Team, m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS));
	m_pChat->AddLine(-1, 0, aBuf);
}

void CGameClient::SendSwitchTeam(int Team)
{
	CNetMsg_Cl_SetTeam Msg;
	Msg.m_Team = Team;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

void CGameClient::SendStartInfo()
{
	CNetMsg_Cl_StartInfo Msg;
	Msg.m_pName = g_Config.m_PlayerName;
	Msg.m_pClan = g_Config.m_PlayerClan;
	Msg.m_Country = g_Config.m_PlayerCountry;
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		Msg.m_apSkinPartNames[p] = gs_apSkinVariables[p];
		Msg.m_aUseCustomColors[p] = *gs_apUCCVariables[p];
		Msg.m_aSkinPartColors[p] = *gs_apColorVariables[p];
	}
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

void CGameClient::SendKill()
{
	CNetMsg_Cl_Kill Msg;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

void CGameClient::SendReadyChange()
{
	CNetMsg_Cl_ReadyChange Msg;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

void CGameClient::ConTeam(IConsole::IResult *pResult, void *pUserData)
{
	((CGameClient*)pUserData)->SendSwitchTeam(pResult->GetInteger(0));
}

void CGameClient::ConKill(IConsole::IResult *pResult, void *pUserData)
{
	((CGameClient*)pUserData)->SendKill();
}

void CGameClient::ConReadyChange(IConsole::IResult *pResult, void *pUserData)
{
	((CGameClient*)pUserData)->SendReadyChange();
}

void CGameClient::ConchainFriendUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CGameClient *pClient = static_cast<CGameClient *>(pUserData);
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(pClient->m_aClients[i].m_Active)
			pClient->m_aClients[i].m_Friend = pClient->Friends()->IsFriend(pClient->m_aClients[i].m_aName, pClient->m_aClients[i].m_aClan, true);
	}
}

IGameClient *CreateGameClient()
{
	return &g_GameClient;
}
