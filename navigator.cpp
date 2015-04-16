#include <stdio.h>
#include <stdlib.h>

#define MAX_CROSSROADS 10
#define CROSSROADS_EPS 1.
#define D_INFTY (1e99)

struct Point
{
    double x;
    double y;
};

class Navigator
{
public:
    void initialize()
    {
        nCrossroads = 0;
        nExits = 0;
        prevDirection = -1;
    }

    int navigate(Point& position, int direction, double distance)
    {
        sprintf(buffer, "\nnavigate() called with position = [%lf, %lf], direction = %d, distance = %lf\n",
                position.x, position.y, direction, distance);
        Log(buffer);

        int maxi = -1;
        for(int i = 0; i < nCrossroads; i++)
        {
            if(sqdist(position, crossroads[i]) < CROSSROADS_EPS)
            {
                if(maxi == -1)
                {
                    maxi = i;
                }
                else
                {
                    sprintf(buffer, "WARNING!!! Crossroads #%d is also a candidate!\n", i);
                    Log(buffer);
                }
            }
        }

        if(maxi == -1)
        {
            sprintf(buffer, "considered a new crossroads #%d\n", nCrossroads);
            Log(buffer);

            maxi = nCrossroads++;
            crossroads[maxi] = position;
            nExits += 4;

            for(int i = 0; i < 4; i++)
            {
                fol[maxi][i] = -1;
            }
        }
        else
        {
            sprintf(buffer, "recognized as crossroads #%d\n", maxi);
            Log(buffer);

            position = crossroads[maxi];
        }

        if((prevDistance != -1) && (fol[prevCrossroads][prevDirection] == -1))
        {
            fol[prevCrossroads][prevDirection] = maxi;
            dist[prevCrossroads][prevDirection] = distance - prevDistance;

            fol[maxi][direction ^ 2] = prevCrossroads;
            dist[maxi][direction ^ 2] = distance - prevDistance;

            nExits -= 2;

            sprintf(buffer, "crossroads #%d exit %d connects to crossroads #%d exit %d, distance = %lf\n",
                    prevCrossroads, prevDirection, maxi, direction ^ 2, distance - prevDistance);
            Log(buffer);
        }

        if(!(nExits))
        {
            sprintf(buffer, "navigate() returns TERMINATE\n\n");
            Log(buffer);
            return -2;
        }

        int newDirection = getNewDirection(maxi, direction ^ 2);

        prevCrossroads = maxi;
        prevDirection = newDirection;
        prevDistance = distance;

        int turn = ((newDirection + (direction * 3) + 1) & 3) - 1;
        sprintf(buffer, "navigate() returns %d\n\n", turn);
        Log(buffer);

        return turn;
    }

private:
    char buffer[200];
    int nCrossroads;
    Point crossroads[MAX_CROSSROADS];

    int fol[MAX_CROSSROADS][4];
    double dist[MAX_CROSSROADS][4];

    int prevCrossroads;
    int prevDirection;
    double prevDistance;
    int nExits;

    double searchDist[MAX_CROSSROADS];
    int visited[MAX_CROSSROADS];

    double sqdist(const Point& p, const Point& q)
    {
        return ((p.x - q.x) * (p.x - q.x)) + ((p.y - q.y) * (p.y - q.y));
    }

    void Log(const char* Text)
    {
        printf("%s", Text);
    }

    int getNewDirection(int c, int fd)
    {
        int a[3], n = 0;
        for(int i = 0; i < 4; i++)
        {
            if((i != fd) && (fol[c][i] == -1))
            {
                a[n++] = i;
            }
        }

        if(n)
        {
            int rsl = a[rand() % n];
            sprintf(buffer, "selecting random undiscovedred direction %d\n", rsl);
            Log(buffer);
            return rsl;
        }

        double min = D_INFTY;
        int rsl = -1;

        for(int i = 0; i < 4; i++)
        {
            if(i == fd)
            {
                continue;
            }

            for(int j = 0; j < nCrossroads; j++)
            {
                visited[j] = 0;
                searchDist[j] = D_INFTY;
            }

            int t = fol[c][i];
            searchDist[t] = dist[c][i];

            int cur;
            while(1)
            {
                double mincur = D_INFTY;
                cur = -1;
                for(int j = 0; j < nCrossroads; j++)
                {
                    if((!(visited[j])) && (searchDist[j] < mincur))
                    {
                        mincur = searchDist[j];
                        cur = j;
                    }
                }

                visited[cur] = 1;

                for(int j = 0; j < 4; j++)
                {
                    if(fol[cur][j] == -1)
                    {
                        goto FOUND;
                    }
                }

                for(int j = 0; j < 4; j++)
                {
                    t = fol[cur][j];
                    if(searchDist[cur] + dist[cur][j] < searchDist[t])
                    {
                        searchDist[t] = searchDist[cur] + dist[cur][j];
                    }
                }
            }

            FOUND:
            sprintf(buffer, "noting exit %d towards undiscovered crossroads #%d at distance %lf", i, cur, searchDist[cur]);
            Log(buffer);

            if(searchDist[cur] < min)
            {
                min = searchDist[cur];
                rsl = i;
                Log(" (temporarily best)");
            }

            Log("\n");
        }

        return rsl;
    }
};

int main()
{
    Navigator n;
    n.initialize();

    Point p;
    int dir;
    double dist;
    while(scanf("%lf%lf%d%lf", &(p.x), &(p.y), &dir, &dist), n.navigate(p, dir, dist) != -2);

    return 0;
}
