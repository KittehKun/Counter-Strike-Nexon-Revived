// botassist: how bunched up players are (the decisions on their own: no game calls, so a test builds this file alone).
// Build A of the before/after bot test: only the measuring part of fix/bot-improvement's crowding.inl.

// how bunched up some players are (botassist.log's "crowd:" lines): pairs closer than touch, and the most within group
// of one of them (itself counted; biggestAt its index, the first of the biggest; -1 for none)
struct CrowdPt
{
    float x, y, z;
};
struct CrowdCount
{
    int pairs, biggest, biggestAt;
};
static CrowdCount CountCrowd(const std::vector<CrowdPt>& pts, float touch, float group)
{
    CrowdCount c = { 0, 0, -1 };
    for (size_t i = 0; i < pts.size(); i++)
    {
        int around = 1;
        for (size_t k = 0; k < pts.size(); k++)
        {
            if (k == i)
                continue;
            float dx = pts[i].x - pts[k].x, dy = pts[i].y - pts[k].y, dz = pts[i].z - pts[k].z;
            float d2 = dx * dx + dy * dy + dz * dz;
            around += d2 < group * group;
            c.pairs += k > i && d2 < touch * touch;
        }
        if (around > c.biggest)
        {
            c.biggest = around;
            c.biggestAt = (int)i;
        }
    }
    return c;
}
