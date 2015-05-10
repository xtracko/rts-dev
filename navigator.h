#define MAX_CROSSROADS 100
#define INFTY (1 << 30)

struct Point
{
    int x;
    int y;

    int operator == (const Point& p) const
    {
        return (x == p.x) && (y == p.y);
    }
};

struct OutEdge
{
    int to;
    int dist;
};

class Navigator
{
public:
    void initialize()
    //call this function at the very beginning
    {
        position.x = 0;
        position.y = 0;
        direction = 0;
        distance = 0;

        nCrossroads = 0;
        nExits = 0;
        prevDirection = -1;
    }

    void turnMet(int turn, int dist)
    //turn: 1 = right turn, -1 = left turn
    //dist: number of tiles traveled since the previous call of turnMet() or crossroadsMet()
    //      (is ignored at the first call)
    {
        if(turn)
        //(this function is also called internally with turn = 0)
        {
            sprintf(buffer, "turnMet() called with turn = %d, dist = %d\n", turn, dist);
            Log(buffer);
        }

        distance += dist;

        switch(direction)
        {
            case 0: position.y -= dist; break;
            case 1: position.x += dist; break;
            case 2: position.y += dist; break;
            case 3: position.x -= dist; break;
        }

        direction += turn;
        direction &= 3;

        sprintf(buffer, "current position is [%d, %d], direction %d\n", position.x, position.y, direction);
        Log(buffer);
    }

    int crossroadsMet(int dist)
    //dist: number of tiles traveled since the previous call of turnMet() or crossroadsMet()
    //      (is ignored at the first call)
    //return value: -2 = stop, -1 = turn left, 0 = go straight, 1 = turn right
    {
        sprintf(buffer, "\ncrossroadsMet() called with d = %d\n", dist);
        Log(buffer);

        turnMet(0, dist);

        int c = -1;
        for(int i = 0; i < nCrossroads; i++)
        {
            if(position == crossroads[i])
            {
                c = i;
                break;
            }
        }

        if(c == -1)
        {
            sprintf(buffer, "considered a new crossroads #%d\n", nCrossroads);
            Log(buffer);

            c = nCrossroads++;
            crossroads[c] = position;
            nExits += 4;

            for(int i = 0; i < 4; i++)
            {
                edge[c][i].to = -1;
            }
        }
        else
        {
            sprintf(buffer, "recognized as crossroads #%d\n", c);
            Log(buffer);
        }

        if((prevDirection != -1) && (edge[prevCrossroads][prevDirection].to == -1))
        {
            edge[prevCrossroads][prevDirection] = (OutEdge) {c, distance};
            edge[c][direction ^ 2] = (OutEdge) {prevCrossroads, distance};

            nExits -= 2;

            sprintf(buffer, "crossroads #%d exit %d connects to crossroads #%d exit %d, distance = %d\n",
                    prevCrossroads, prevDirection, c, direction ^ 2, distance);
            Log(buffer);
        }

        if(!(nExits))
        {
            sprintf(buffer, "crossroadsMet() returns TERMINATE\n\n");
            Log(buffer);
            return -2;
        }

        int newDirection = getNewDirection(c, direction ^ 2);

        prevCrossroads = c;
        prevDirection = newDirection;
        distance = 0;

        int turn = ((newDirection + (direction * 3) + 1) & 3) - 1;
        sprintf(buffer, "crossroadsMet() returns %d\n\n", turn);
        Log(buffer);

        return turn;
    }

private:
    Point position;
    int direction;
    int distance;

    char buffer[200];
    int nCrossroads;
    Point crossroads[MAX_CROSSROADS];

    OutEdge edge[MAX_CROSSROADS][4];

    int prevCrossroads;
    int prevDirection;
    int nExits;

    int searchDist[MAX_CROSSROADS];
    int visited[MAX_CROSSROADS];

    void Log(const char* Text)
    {
        printf("%s", Text);
    }

    int getNewDirection(int c, int fd)
    {
        int a[3], n = 0;
        for(int i = 0; i < 4; i++)
        {
            if((i != fd) && (edge[c][i].to == -1))
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

        int min = INFTY;
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
                searchDist[j] = INFTY;
            }

            int t = edge[c][i].to;
            searchDist[t] = edge[c][i].dist;

            int cur;
            while(1)
            {
                int mincur = INFTY;
                cur = -1;
                for(int j = 0; j < nCrossroads; j++)
                {
                    if((!(visited[j])) && (searchDist[j] < mincur))
                    {
                        mincur = searchDist[j];
                        cur = j;
                    }
                }

                if(cur == -1)
                {
                    sprintf(buffer, "FATAL ERROR!!! No undiscovered crossroads encountered when trying exit %d\n", i);
                    Log(buffer);
                    return fd ^ 2;
                }

                visited[cur] = 1;

                for(int j = 0; j < 4; j++)
                {
                    if(edge[cur][j].to == -1)
                    {
                        goto FOUND;
                    }
                }

                for(int j = 0; j < 4; j++)
                {
                    t = edge[cur][j].to;
                    if(searchDist[cur] + edge[cur][j].dist < searchDist[t])
                    {
                        searchDist[t] = searchDist[cur] + edge[cur][j].dist;
                    }
                }
            }

            FOUND:
            sprintf(buffer, "noting exit %d towards undiscovered crossroads #%d at distance %d", i, cur, searchDist[cur]);
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
