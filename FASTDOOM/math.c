#include "doomtype.h"

int GetClosestColor(byte *colors, int num_colors, int r1, int g1, int b1)
{
    int i;

    int result;

    int distance;
    int best_difference = MAXINT;

    for (i = 0; i < num_colors; i++)
    {
        int r2, g2, b2;
        int cR, cG, cB;
        int pos = i * 3;

        r2 = (int)colors[pos];
        cR = (r2 - r1) * (r2 - r1);

        g2 = (int)colors[pos + 1];
        cG = (g2 - g1) * (g2 - g1);

        b2 = (int)colors[pos + 2];
        cB = (b2 - b1) * (b2 - b1);

        distance = cR + cG + cB;

        if (distance == 0)
        {
            return i;
        }
        else
        {
            if (best_difference > distance)
            {
                best_difference = distance;
                result = i;
            }
        }
    }

    return result;
}
