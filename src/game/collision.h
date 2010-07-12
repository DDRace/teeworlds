#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	bool IsTileSolid(int x, int y);
	int GetTile(int x, int y);

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
	};

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(round(x), round(y)); }
	bool CheckPoint(vec2 p) { return CheckPoint(p.x, p.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round(x), round(y)); }
	int GetWidth() { return m_Width; }
	int GetHeight() { return m_Height; }
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *Bpounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);
	
	// race
	int GetIndex(int x, int y);
	int IsTeleport(int x, int y);
	int IsCheckpoint(int x, int y);
	int IsSpeedup(int x, int y);
	void GetSpeedup(int x, int y, vec2 *Dir, int *Force);
	
	class CTeleTile *m_pTele;
	class CSpeedupTile *m_pSpeedup;
	
	class CLayers *Layers() { return m_pLayers; }
};

#endif
