/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <game/layers.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>

#include "controls.h"
#include "camera.h"
#include "hud.h"
#include "voting.h"
#include "binds.h"

CHud::CHud()
{
	// won't work if zero
	m_AverageFPS = 1.0f;
	OnReset();
}
	
void CHud::OnReset()
{
	m_CheckpointDiff = 0.0f;
	m_DDRaceTime = 0;
	m_LastReceivedTimeTick = 0;
	m_CheckpointTick = 0;
	m_DDRaceTick = 0;
	m_FinishTime = false;
	m_ServerRecord = -1.0f;
	m_PlayerRecord = -1.0f;
	m_DDRaceTimeReceived = false;
}

void CHud::RenderGameTimer()
{
	float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;
	Graphics()->MapScreen(0, 0, 300.0f*Graphics()->ScreenAspect(), 300.0f);
	
	if(!m_pClient->m_Snap.m_pGameobj->m_SuddenDeath)
	{
		char Buf[32];
		int Time = 0;
		if(m_pClient->m_Snap.m_pGameobj->m_TimeLimit)
		{
			Time = m_pClient->m_Snap.m_pGameobj->m_TimeLimit*60 - ((Client()->GameTick()-m_pClient->m_Snap.m_pGameobj->m_RoundStartTick)/Client()->GameTickSpeed());

			if(m_pClient->m_Snap.m_pGameobj->m_GameOver)
				Time  = 0;
		}
		else
			Time = (Client()->GameTick()-m_pClient->m_Snap.m_pGameobj->m_RoundStartTick)/Client()->GameTickSpeed();

		if(Time <= 0)
			str_format(Buf, sizeof(Buf), "00:00.0");
		else
			str_format(Buf, sizeof(Buf), "%02d:%02d.%d", Time/60, Time%60, m_DDRaceTick/10);
		float FontSize = 10.0f;
		float w = TextRender()->TextWidth(0, 12,"00:00.0",-1);
		TextRender()->Text(0, Half-w/2, 2, FontSize, Buf, -1);
	}
}

void CHud::RenderSuddenDeath()
{
	if(m_pClient->m_Snap.m_pGameobj->m_SuddenDeath)
	{
		float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;
		const char *pText = Localize("Sudden Death");
		float FontSize = 12.0f;
		float w = TextRender()->TextWidth(0, FontSize, pText, -1);
		TextRender()->Text(0, Half-w/2, 2, FontSize, pText, -1);
	}
}

void CHud::RenderScoreHud()
{
	int GameFlags = m_pClient->m_Snap.m_pGameobj->m_Flags;
	float Whole = 300*Graphics()->ScreenAspect();
	
	// render small score hud
	if(!(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver) && (GameFlags&GAMEFLAG_TEAMS))
	{
		char aScoreTeam[2][32];
		str_format(aScoreTeam[0], sizeof(aScoreTeam)/2, "%d", m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed);
		str_format(aScoreTeam[1], sizeof(aScoreTeam)/2, "%d", m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue);
		float aScoreTeamWidth[2] = {TextRender()->TextWidth(0, 14.0f, aScoreTeam[0], -1), TextRender()->TextWidth(0, 14.0f, aScoreTeam[1], -1)};
		float ScoreWidthMax = max(max(aScoreTeamWidth[0], aScoreTeamWidth[1]), TextRender()->TextWidth(0, 14.0f, "100", -1));
		float Split = 3.0f;
		float ImageSize = GameFlags&GAMEFLAG_FLAGS ? 16.0f : Split;
		
		for(int t = 0; t < 2; t++)
		{
			// draw box
			Graphics()->BlendNormal();
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			if(t == 0)
				Graphics()->SetColor(1.0f, 0.0f, 0.0f, 0.25f);
			else
				Graphics()->SetColor(0.0f, 0.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRectExt(Whole-ScoreWidthMax-ImageSize-2*Split, 245.0f+t*20, ScoreWidthMax+ImageSize+2*Split, 18.0f, 5.0f, CUI::CORNER_L);
			Graphics()->QuadsEnd();

			// draw score
			TextRender()->Text(0, Whole-ScoreWidthMax+(ScoreWidthMax-aScoreTeamWidth[t])/2-Split, 245.0f+t*20, 14.0f, aScoreTeam[t], -1);

			if(GameFlags&GAMEFLAG_FLAGS && m_pClient->m_Snap.m_paFlags[t])
			{
				if(m_pClient->m_Snap.m_paFlags[t]->m_CarriedBy == -2 || (m_pClient->m_Snap.m_paFlags[t]->m_CarriedBy == -1 && ((Client()->GameTick()/10)&1)))
				{
					// draw flag
					Graphics()->BlendNormal();
					Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
					Graphics()->QuadsBegin();
					RenderTools()->SelectSprite(t==0?SPRITE_FLAG_RED:SPRITE_FLAG_BLUE);
					IGraphics::CQuadItem QuadItem(Whole-ScoreWidthMax-ImageSize, 246.0f+t*20, ImageSize/2, ImageSize);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
				else if(m_pClient->m_Snap.m_paFlags[t]->m_CarriedBy >= 0)
				{
					// draw name of the flag holder
					int Id = m_pClient->m_Snap.m_paFlags[t]->m_CarriedBy%MAX_CLIENTS;
					const char *pName = m_pClient->m_aClients[Id].m_aName;
					float w = TextRender()->TextWidth(0, 10.0f, pName, -1);
					TextRender()->Text(0, Whole-ScoreWidthMax-ImageSize-3*Split-w, 247.0f+t*20, 10.0f, pName, -1);

					// draw tee of the flag holder
					CTeeRenderInfo Info = m_pClient->m_aClients[Id].m_RenderInfo;
					Info.m_Size = 18.0f;
					RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0),
						vec2(Whole-ScoreWidthMax-Info.m_Size/2-Split, 246.0f+Info.m_Size/2+t*20));
				}
			}
		}
	}
}

void CHud::RenderWarmupTimer()
{
	// render warmup timer
	if(m_pClient->m_Snap.m_pGameobj->m_Warmup)
	{
		char Buf[256];
		float FontSize = 20.0f;
		float w = TextRender()->TextWidth(0, FontSize, Localize("Warmup"), -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()+-w/2, 50, FontSize, Localize("Warmup"), -1);

		int Seconds = m_pClient->m_Snap.m_pGameobj->m_Warmup/SERVER_TICK_SPEED;
		if(Seconds < 5)
			str_format(Buf, sizeof(Buf), "%d.%d", Seconds, (m_pClient->m_Snap.m_pGameobj->m_Warmup*10/SERVER_TICK_SPEED)%10);
		else
			str_format(Buf, sizeof(Buf), "%d", Seconds);
		w = TextRender()->TextWidth(0, FontSize, Buf, -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()+-w/2, 75, FontSize, Buf, -1);
	}	
}

void CHud::MapscreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup)
{
	float Points[4];
	RenderTools()->MapscreenToWorld(CenterX, CenterY, pGroup->m_ParallaxX/100.0f, pGroup->m_ParallaxY/100.0f,
		pGroup->m_OffsetX, pGroup->m_OffsetY, Graphics()->ScreenAspect(), 1.0f, Points);
	Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}

void CHud::RenderFps()
{
	if(g_Config.m_ClShowfps)
	{
		// calculate avg. fps
		float FPS = 1.0f / Client()->FrameTime();
		m_AverageFPS = (m_AverageFPS*(1.0f-(1.0f/m_AverageFPS))) + (FPS*(1.0f/m_AverageFPS));
		char Buf[512];
		str_format(Buf, sizeof(Buf), "%d", (int)m_AverageFPS);
		TextRender()->Text(0, m_Width-10-TextRender()->TextWidth(0,12,Buf,-1), 5, 12, Buf, -1);
	}
}

void CHud::RenderConnectionWarning()
{
	if(Client()->ConnectionProblems())
	{
		const char *pText = Localize("Connection Problems...");
		float w = TextRender()->TextWidth(0, 24, pText, -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()-w/2, 50, 24, pText, -1);
	}
}

void CHud::RenderTeambalanceWarning()
{
	// render prompt about team-balance
	bool Flash = time_get()/(time_freq()/2)%2 == 0;
	if (m_pClient->m_Snap.m_pGameobj && (m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS) != 0)
	{	
		int TeamDiff = m_pClient->m_Snap.m_aTeamSize[0]-m_pClient->m_Snap.m_aTeamSize[1];
		if (g_Config.m_ClWarningTeambalance && (TeamDiff >= 2 || TeamDiff <= -2))
		{
			const char *pText = Localize("Please balance teams!");
			if(Flash)
				TextRender()->TextColor(1,1,0.5f,1);
			else
				TextRender()->TextColor(0.7f,0.7f,0.2f,1.0f);
			TextRender()->Text(0x0, 5, 50, 6, pText, -1);
			TextRender()->TextColor(1,1,1,1);
		}
	}
}


void CHud::RenderVoting()
{
	if(!m_pClient->m_pVoting->IsVoting() || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;
	
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.40f);
	RenderTools()->DrawRoundRect(-10, 60-2, 100+10+4+5, 28, 5.0f);
	Graphics()->QuadsEnd();

	TextRender()->TextColor(1,1,1,1);

	char Buf[512];
	str_format(Buf, sizeof(Buf), Localize("%ds left"), m_pClient->m_pVoting->SecondsLeft());
	float tw = TextRender()->TextWidth(0x0, 6, Buf, -1);

	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, 5.0f, 60.0f, 6.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
	Cursor.m_LineWidth = 100-tw;
	TextRender()->TextEx(&Cursor, m_pClient->m_pVoting->VoteDescription(), -1);

	TextRender()->Text(0x0, 5+100-tw, 60, 6, Buf, -1);
	

	CUIRect Base = {5, 70, 100, 4};
	m_pClient->m_pVoting->RenderBars(Base, false);
	
	const char *pYesKey = m_pClient->m_pBinds->GetKey("vote yes");
	const char *pNoKey = m_pClient->m_pBinds->GetKey("vote no");
	str_format(Buf, sizeof(Buf), "%s - %s", pYesKey, Localize("Vote yes"));
	Base.y += Base.h+1;
	UI()->DoLabel(&Base, Buf, 6.0f, -1);

	str_format(Buf, sizeof(Buf), "%s - %s", Localize("Vote no"), pNoKey);
	UI()->DoLabel(&Base, Buf, 6.0f, 1);
}

void CHud::RenderCursor()
{
	if(!m_pClient->m_Snap.m_pLocalCharacter || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;
		
	MapscreenToGroup(m_pClient->m_pCamera->m_Center.x, m_pClient->m_pCamera->m_Center.y, Layers()->GameGroup());
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	// render cursor
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteCursor);
	float CursorSize = 64;
	RenderTools()->DrawSprite(m_pClient->m_pControls->m_TargetPos.x, m_pClient->m_pControls->m_TargetPos.y, CursorSize);
	Graphics()->QuadsEnd();
}

void CHud::RenderHealthAndAmmo()
{
	//mapscreen_to_group(gacenter_x, center_y, layers_game_group());

	float x = 5;
	float y = 5;

	// render ammo count
	// render gui stuff

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->MapScreen(0,0,m_Width,300);
	
	Graphics()->QuadsBegin();
	
	// if weaponstage is active, put a "glow" around the stage ammo
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteProj);
	IGraphics::CQuadItem Array[10];
	int i;
	for (i = 0; i < min(m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount, 10); i++)
		Array[i] = IGraphics::CQuadItem(x+i*12,y+24,10,10);
	Graphics()->QuadsDrawTL(Array, i);
	Graphics()->QuadsEnd();

	Graphics()->QuadsBegin();
	int h = 0;

	// render health
	RenderTools()->SelectSprite(SPRITE_HEALTH_FULL);
	for(; h < min(m_pClient->m_Snap.m_pLocalCharacter->m_Health, 10); h++)
		Array[h] = IGraphics::CQuadItem(x+h*12,y,10,10);
	Graphics()->QuadsDrawTL(Array, h);

	i = 0;
	RenderTools()->SelectSprite(SPRITE_HEALTH_EMPTY);
	for(; h < 10; h++)
		Array[i++] = IGraphics::CQuadItem(x+h*12,y,10,10);
	Graphics()->QuadsDrawTL(Array, i);

	// render armor meter
	h = 0;
	RenderTools()->SelectSprite(SPRITE_ARMOR_FULL);
	for(; h < min(m_pClient->m_Snap.m_pLocalCharacter->m_Armor, 10); h++)
		Array[h] = IGraphics::CQuadItem(x+h*12,y+12,10,10);
	Graphics()->QuadsDrawTL(Array, h);

	i = 0;
	RenderTools()->SelectSprite(SPRITE_ARMOR_EMPTY);
	for(; h < 10; h++)
		Array[i++] = IGraphics::CQuadItem(x+h*12,y+12,10,10);
	Graphics()->QuadsDrawTL(Array, i);
	Graphics()->QuadsEnd();
}

void CHud::RenderDDRaceEffects()
{
	// check racestate
	if(m_FinishTime && m_LastReceivedTimeTick + Client()->GameTickSpeed()*2 < Client()->GameTick())
	{
		m_FinishTime = false;
		m_DDRaceTimeReceived = false;
		return;
	}
	
	if(m_DDRaceTime)
	{
		char aBuf[64];
		if(m_FinishTime)
		{
			str_format(aBuf, sizeof(aBuf), "Finish time: %02d:%02d.%02d", m_DDRaceTime/6000, m_DDRaceTime/100-m_DDRaceTime/6000 * 60, m_DDRaceTime % 100);
			TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0,12,aBuf,-1)/2, 20, 12, aBuf, -1);
		}
		else if(m_CheckpointTick + Client()->GameTickSpeed()*6 > Client()->GameTick())
		{
			str_format(aBuf, sizeof(aBuf), "%+5.2f", m_CheckpointDiff);
			
			// calculate alpha (4 sec 1 than get lower the next 2 sec)
			float a = 1.0f;
			if(m_CheckpointTick+Client()->GameTickSpeed()*4 < Client()->GameTick() && m_CheckpointTick+Client()->GameTickSpeed()*6 > Client()->GameTick())
			{
				// lower the alpha slowly to blend text out
				a = ((float)(m_CheckpointTick+Client()->GameTickSpeed()*6) - (float)Client()->GameTick()) / (float)(Client()->GameTickSpeed()*2);
			}
			
			if(m_CheckpointDiff > 0)
				TextRender()->TextColor(1.0f,0.5f,0.5f,a); // red
			else if(m_CheckpointDiff < 0)
				TextRender()->TextColor(0.5f,1.0f,0.5f,a); // green
			else if(!m_CheckpointDiff)
				TextRender()->TextColor(1,1,1,a); // white
			TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0, 10, aBuf, -1)/2, 20, 10, aBuf, -1);
			
			TextRender()->TextColor(1,1,1,1);
		}
	}
		/*else if(m_DDRaceTimeReceived)
		{
			str_format(aBuf, sizeof(aBuf), "%02d:%02d.%d", m_DDRaceTime/60, m_DDRaceTime%60, m_DDRaceTick/10);
			TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0, 12,"00:00.0",-1)/2, 20, 12, aBuf, -1); // use fixed value for text width so its not shaky
		}*/


	
	static int LastChangeTick = 0;
	if(LastChangeTick != Client()->PredGameTick())
	{	
		m_DDRaceTick += 100/Client()->GameTickSpeed();
		LastChangeTick = Client()->PredGameTick();
	}
	
	if(m_DDRaceTick >= 100)
		m_DDRaceTick = 0;
}

void CHud::RenderRecord()
{	
	if(m_ServerRecord > 0 )
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Server best:");
		TextRender()->Text(0, 5, 40, 6, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "%02d:%05.2f", (int)m_ServerRecord/60, m_ServerRecord-((int)m_ServerRecord/60*60));
		TextRender()->Text(0, 53, 40, 6, aBuf, -1);
	}
	
	if(m_PlayerRecord > 0 )
	{				
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Personal best:");
		TextRender()->Text(0, 5, 47, 6, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "%02d:%05.2f", (int)m_PlayerRecord/60, m_PlayerRecord-((int)m_PlayerRecord/60*60));
		TextRender()->Text(0, 53, 47, 6, aBuf, -1);
	}
}

void CHud::OnRender()
{
	if(!m_pClient->m_Snap.m_pGameobj)
		return;
		
	m_Width = 300*Graphics()->ScreenAspect();

	bool Spectate = false;
	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team == -1)
		Spectate = true;
	
	if(m_pClient->m_Snap.m_pLocalCharacter && !Spectate && !(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver)) {
		RenderHealthAndAmmo();
		RenderDDRaceEffects();
	}
		

	RenderGameTimer();
	RenderSuddenDeath();
	RenderScoreHud();
	RenderWarmupTimer();
	RenderFps();
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		RenderConnectionWarning();
	RenderTeambalanceWarning();
	RenderVoting();
	RenderRecord();
	RenderCursor();
	
}

void CHud::OnMessage(int MsgType, void *pRawMsg)
{

	if(MsgType == NETMSGTYPE_SV_DDRACETIME)
	{
		m_DDRaceTimeReceived = true;
		
		CNetMsg_Sv_DDRaceTime *pMsg = (CNetMsg_Sv_DDRaceTime *)pRawMsg;

		m_DDRaceTime = pMsg->m_Time;
		m_DDRaceTick = 0;
		
		m_LastReceivedTimeTick = Client()->GameTick();
		
		m_FinishTime = pMsg->m_Finish ? true : false;
		
		if(pMsg->m_Check)
		{
			m_CheckpointDiff = (float)pMsg->m_Check/100;
			m_CheckpointTick = Client()->GameTick();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalCid)
		{
			m_CheckpointTick = 0;
			m_DDRaceTime = 0;
		}
	}
	else if(MsgType == NETMSGTYPE_SV_RECORD)
	{
		CNetMsg_Sv_Record *pMsg = (CNetMsg_Sv_Record *)pRawMsg;
		m_ServerRecord = (float)pMsg->m_ServerTimeBest/100;
		m_PlayerRecord = (float)pMsg->m_PlayerTimeBest/100;
	}
}
