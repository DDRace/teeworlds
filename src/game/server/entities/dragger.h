/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_ENTITY_DRAGGER_H
#define GAME_SERVER_ENTITY_DRAGGER_H

#include <game/server/entity.h>
class CCharacter;

class CDragger : public CEntity
{
	vec2 m_Core;
	float m_Strength;
	int m_EvalTick;
	void Move();
	void Drag();
	CCharacter * m_Target;
	bool m_NW;
	int m_CatchedTeam;
public:


	CDragger(CGameWorld *pGameWorld, vec2 Pos, float Strength, bool NW, int CatchedTeam, int Layer = 0, int Number = 0);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int snapping_client);
};

class CDraggerTeam {
	CDragger * m_Draggers[MAX_CLIENTS];
	
public:
	
	CDraggerTeam(CGameWorld *pGameWorld, vec2 Pos, float Strength, bool NW=false, int Layer = 0, int Number = 0);
	//~CDraggerTeam();
};

#endif
