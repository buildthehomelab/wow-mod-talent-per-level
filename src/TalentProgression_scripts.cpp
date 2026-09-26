/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Copyright (C) Aldrynth / VenomekPL
 *
 * Talent curve: 1 point per level from 1–MaxLevel (default 80) instead of
 * Blizzard's level 10 start. Bonus talents (extraBonusTalentCount) stack on top.
 */

#include "Config.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <algorithm>

class TalentProgression_Player : public PlayerScript
{
public:
    TalentProgression_Player() : PlayerScript("TalentProgression_Player", {
        PLAYERHOOK_ON_CALCULATE_TALENTS_POINTS
    }) { }

    void OnPlayerCalculateTalentsPoints(Player const* player, uint32& talentPointsForLevel) override
    {
        if (!sConfigMgr->GetOption<bool>("TalentProgression.Enable", true))
            return;

        uint8 maxLevel = sConfigMgr->GetOption<uint8>("TalentProgression.MaxLevel", 80);
        uint8 level = std::min<uint8>(player->GetLevel(), maxLevel);

        uint32 blizzBase = level < 10 ? 0 : uint32(level) - 9;
        uint32 bonus = talentPointsForLevel > blizzBase ? talentPointsForLevel - blizzBase : 0;
        talentPointsForLevel = uint32(level) + bonus;
    }
};

class TalentProgression_World : public WorldScript
{
public:
    TalentProgression_World() : WorldScript("TalentProgression_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        if (!sConfigMgr->GetOption<bool>("TalentProgression.Enable", true))
            return;

        LOG_INFO("server.loading", "TalentProgression: 1 talent/level through level {}",
            sConfigMgr->GetOption<uint8>("TalentProgression.MaxLevel", 80));
    }
};

void AddTalentProgressionScripts()
{
    new TalentProgression_Player();
    new TalentProgression_World();
}
