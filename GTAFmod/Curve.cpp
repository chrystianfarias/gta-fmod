#include "Curve.h"
#include "RenderWare.h"
#include "CHud.h"

#define MAX_RPM 8000
#define MAX_TORQUE 200
#define POINTS 5

float Curve::Evaluate(float x)
{
	int arrSize = 3;
	float y = 0;

	for (int i = 0; i < arrSize; ++i)
	{
		float p = 1;
		for (int j = 0; j < arrSize; ++j)
		{
			if (i == j)
			{
				continue;
			}

			p *= (x - points[j].x) / (points[i].x - points[j].x);
		}
		y += points[i].y * p;
	}
	return y;
}
/*
void Curve::Draw(CRect rect, float lineWidth)
{
	//Quad
	Setup2dVertex(lineQuadVerts[0], rect.left, rect.top, RWRGBALONG(255, 255, 255, 255));
	Setup2dVertex(lineQuadVerts[1], rect.left + lineWidth, rect.top, RWRGBALONG(255, 255, 255, 255));

	Setup2dVertex(lineQuadVerts[2], rect.left, rect.top + rect.bottom, RWRGBALONG(255, 255, 255, 255));
	Setup2dVertex(lineQuadVerts[3], rect.left + lineWidth, rect.top + rect.bottom, RWRGBALONG(255, 255, 255, 255));

	Setup2dVertex(lineQuadVerts[4], rect.left + rect.right + lineWidth, rect.top + rect.bottom + lineWidth, RWRGBALONG(255, 255, 255, 255));
	Setup2dVertex(lineQuadVerts[5], rect.left + rect.right, rect.top + rect.bottom, RWRGBALONG(255, 255, 255, 255));

	//Curve
	for (int i = 0; i < POINTS; ++i)
	{
		float lerp = i / POINTS * MAX_RPM;
		RwIm2DVertex lineVerts[4];
		float torque = Evaluate(i);

		int x = rect.left + (lerp * rect.right);
		int y = rect.top + ((torque / MAX_TORQUE) * rect.bottom) + rect.top;

		Setup2dVertex(lineVerts[0], x, y, RWRGBALONG(255, 0, 0, 255));
		Setup2dVertex(lineVerts[2], x, y + lineWidth, RWRGBALONG(255, 0, 0, 255));
		Setup2dVertex(lineVerts[3], x + lineWidth, y + lineWidth, RWRGBALONG(255, 0, 0, 255));
		Setup2dVertex(lineVerts[1], x + lineWidth, y, RWRGBALONG(255, 0, 0, 255));
	}
}

void Curve::Render()
{
	RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, lineQuadVerts, 6);

	for (int i = 0; i < POINTS; ++i)
	{
		RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, &lineVerts[i], 4);
	}
}*/