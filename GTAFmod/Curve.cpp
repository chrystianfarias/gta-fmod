#include "Curve.h"

float Curve::Evaluate(float x)
{
	int arrSize = 4;
	float total = 0;

	for (int i = 0; i < arrSize; ++i)
	{
		float totalTop = 0;
		float totalButton = 0;
		for (int j = 0; j < arrSize; ++j)
		{
			if (i != j)
			{
				if (i == 0)
				{
					totalTop = (x - points[j].y);
					totalButton = (points[i].y - points[j].y);
				}
				else
				{
					totalTop *= (x - points[j].y);
					totalButton *= (points[i].y - points[j].y);
				}
			}
		}
		total += points[i].x * (totalTop / totalButton);
	}
	return total;
}