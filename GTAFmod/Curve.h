#pragma once
#include "plugin.h"

class Curve
{
	public:
		CVector2D (&points)[];

		Curve(CVector2D (&newPoints)[]) : points(newPoints)
		{

		}
		float Evaluate(float value);
};