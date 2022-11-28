#include "doomtype.h"

int SQRT(int x)
{
    int start, end, ans;

    if (x == 0 || x == 1)
        return x;

    start = 1;
    end = x / 2;

    while (start <= end)
    {
        int mid = (start + end) / 2;

        if (mid * mid == x)
            return mid;

        if (mid <= x / mid)
        {
            start = mid + 1;
            ans = mid;
        }
        else
            end = mid - 1;
    }
    return ans;
}

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

        distance = SQRT(cR + cG + cB);

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
