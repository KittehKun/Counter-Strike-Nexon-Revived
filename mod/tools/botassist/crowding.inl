// botassist: bots in each other's way, the decisions on their own (no game calls, so a test builds this file alone).
// Included by botassist.cpp before the navigation code; the game side (who is on a ladder, who blocks whom) is there.
//
//  - ladders: one bot climbs at a time each way. A second one follows a bot going the same way once it is LADDER_GAP
//    past their end; one coming the other way (or stopped on it) waits till it's off. Bots walking up to the ladder
//    take turns by distance to it (within LADDER_TIE: by key), so two arriving together don't both go.
//  - holdout places: a bot takes the place none of its group stands on (the fewest, the lowest first), so a bot that
//    joins doesn't walk onto one that's taken when an earlier one was infected; the places are the spot's areas that
//    SpreadPoints keeps apart (HOLD_PLACE_GAP).
//  - in each other's way: of two bots, the one with the lower key keeps going and the other steps aside; a person in
//    the way always gets the way.
//  - blocked links: a link between two areas that stuck bots ran into a wall on, LINK_BLOCK_HITS times, is one every
//    bot's path goes around (NavBlockCost in botassist.cpp) till it's kept no longer.

// a player near a ladder: on it (dir 1 up, -1 down, 0 not moving; feetZ), or walking up to it (dist from the ladder)
struct LadderOther
{
    bool on;
    int dir;
    float feetZ;
    float dist;
    long long key;
};
static const float LADDER_GAP = 80.0f, LADDER_TIE = 8.0f;

// myDir: the way this bot climbs (1 up, -1 down); entryZ: the height of the end it gets on at; myDist: how far it is from
// the ladder; myKey: its turn when as near as another
static bool LadderMustWait(int myDir, float myDist, long long myKey, float entryZ, const std::vector<LadderOther>& others)
{
    for (const LadderOther& o : others)
    {
        if (o.on)
        {
            if (o.dir != myDir || fabsf(o.feetZ - entryZ) <= LADDER_GAP)
                return true; // coming the other way, stopped, or still at this end
            continue;
        }
        if (o.dist + LADDER_TIE < myDist || (fabsf(o.dist - myDist) <= LADDER_TIE && o.key < myKey))
            return true; // nearer the ladder: its turn first
    }
    return false;
}

// the place (0 .. places-1) the fewest of taken stand on, the lowest of those
static int LeastUsedPlace(const std::vector<int>& taken, int places)
{
    if (places <= 0)
        return 0;
    std::vector<int> used(places, 0);
    for (int t : taken)
        if (t >= 0 && t < places)
            used[t]++;
    int best = 0;
    for (int i = 1; i < places; i++)
        if (used[i] < used[best])
            best = i;
    return best;
}

// in order: the points at least gap from every point kept before them (their indices)
static std::vector<int> SpreadPoints(const std::vector<std::pair<float, float>>& pts, float gap)
{
    std::vector<int> kept;
    for (int i = 0; i < (int)pts.size(); i++)
    {
        bool apart = true;
        for (int k : kept)
        {
            float dx = pts[i].first - pts[k].first, dy = pts[i].second - pts[k].second;
            apart = apart && dx * dx + dy * dy >= gap * gap;
        }
        if (apart)
            kept.push_back(i);
    }
    return kept;
}

// this bot steps aside for the one in its way: always for a person, for a bot with a lower key
static bool GivesWay(bool otherIsBot, long long myKey, long long otherKey)
{
    return !otherIsBot || otherKey < myKey;
}

// a link between two nav areas that a bot couldn't walk (a wall or door the mesh doesn't know about): from LINK_BLOCK_HITS
// stuck reports on, every bot's path goes another way while there is one. ent/cls: what was in the way (only the game side
// reads them: once that's gone, a broken breakable, the link is open again)
struct BlockedLink
{
    int from, to, hits;
    float last, until;
    void* ent;
    int cls;
};
static const int LINK_BLOCK_HITS = 2, LINK_BLOCK_MAX = 64;

// one more stuck report on from -> to: counted, and kept keep s from now. A full list drops the one ending first.
static void NoteBlockedLink(std::vector<BlockedLink>& links, int from, int to, float now, float keep, void* ent, int cls)
{
    for (BlockedLink& l : links)
        if (l.from == from && l.to == to)
        {
            l.hits++;
            l.last = now;
            l.until = now + keep;
            l.ent = ent;
            l.cls = cls;
            return;
        }
    if ((int)links.size() >= LINK_BLOCK_MAX)
    {
        size_t first = 0;
        for (size_t i = 1; i < links.size(); i++)
            if (links[i].until < links[first].until)
                first = i;
        links.erase(links.begin() + first);
    }
    links.push_back({ from, to, 1, now, now + keep, ent, cls });
}

// from -> to blocked for everyone now (a clock that went back, a new map, doesn't count)
static bool LinkBlocked(const std::vector<BlockedLink>& links, int from, int to, float now)
{
    for (const BlockedLink& l : links)
        if (l.from == from && l.to == to)
            return l.hits >= LINK_BLOCK_HITS && now >= l.last && now < l.until;
    return false;
}
