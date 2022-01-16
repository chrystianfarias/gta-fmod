#pragma once
#include "plugin.h"
#include "RenderWare.h"
#include "CHud.h"

#define POINTS 5

class Curve
{
	public:
		CVector2D (&points)[];
		float maxX;
		float maxY;
		//RwIm2DVertex lineQuadVerts[POINTS];
		//RwIm2DVertex lineVerts[POINTS];

		Curve(CVector2D (&newPoints)[]) : points(newPoints)
		{
			maxX = newPoints[0].x;
			maxY = newPoints[0].y;
			for (int i = 0; i < 3; ++i)
			{
				if (maxX < newPoints[i].x)
					maxX = newPoints[i].x;

				if (maxY < newPoints[i].y)
					maxY = newPoints[i].y;
			}
		}
		float Evaluate(float value);
		//void Draw(CRect rect, float lineWidth);
		//void Render();
};

static void Setup2dVertex(RwIm2DVertex& vertex, float x, float y, RwUInt32 color) {
	vertex.x = x;
	vertex.y = y;
	vertex.u = vertex.v = 0.0f;
	vertex.z = CSprite2d::NearScreenZ + 0.0001f;
	vertex.rhw = CSprite2d::RecipNearClip;
	vertex.emissiveColor = color;
}