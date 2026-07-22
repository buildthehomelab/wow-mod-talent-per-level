/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Copyright (C) Aldrynth / VenomekPL
 *
 * Talent curve: 1 point per level from 1–MaxLevel (default 80).
 * First-kill: +1 bonus talent per allowlisted outdoor/raid endboss, once per character.
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <algorithm>
#include <unordered_set>

namespace
{
    // Outdoor world bosses + canon raid endbosses (not every trash boss).
    std::unordered_set<uint32> const FirstKillBossEntries = {
        // Outdoor / world
        6109,   // Azuregos
        12397,  // Lord Kazzak
        14887,  // Ysondre
        14888,  // Lethon
        14889,  // Emeriss
        14890,  // Taerar
        17711,  // Doomwalker
        18728,  // Doom Lord Kazzak

        // Vanilla endbosses
        10184,  // Onyxia
        11502,  // Ragnaros
        11583,  // Nefarian
        14834,  // Hakkar
        15339,  // Ossirian
        15727,  // C'Thun
        15990,  // Kel'Thuzad

        // TBC endbosses
        19044,  // Gruul
        17257,  // Magtheridon
        21212,  // Lady Vashj
        19622,  // Kael'thas
        17968,  // Archimonde
        22917,  // Illidan
        23863,  // Zul'jin
        25315,  // Kil'jaeden

        // WotLK endbosses
        28859,  // Malygos
        28860,  // Sartharion
        33288,  // Yogg-Saron
        34564,  // Anub'arak
        36597,  // The Lich King
        39863,  // Halion
        32871,  // Algalon
    };

    bool IsFirstKillBoss(uint32 entry)
    {
        return FirstKillBossEntries.find(entry) != FirstKillBossEntries.end();
    }

    void TryAwardFirstKillTalent(Player* player, Creature* killed)
    {
        if (!player || !killed)
            return;

        if (!sConfigMgr->GetOption<bool>("TalentProgression.Enable", true))
            return;

        if (!sConfigMgr->GetOption<bool>("TalentProgression.FirstKillEnable", true))
            return;

        uint32 entry = killed->GetEntry();
        if (!IsFirstKillBoss(entry))
            return;

        uint32 guid = player->GetGUID().GetCounter();
        QueryResult existing = CharacterDatabase.Query(
            "SELECT 1 FROM aldr_first_kill_talent WHERE guid = {} AND creature_entry = {}",
            guid, entry);
        if (existing)
            return;

        CharacterDatabase.DirectExecute(
            "INSERT INTO aldr_first_kill_talent (guid, creature_entry) VALUES ({}, {})",
            guid, entry);

        player->RewardExtraBonusTalentPoints(1);
        player->InitTalentForLevel();

        if (sConfigMgr->GetOption<bool>("TalentProgression.FirstKillAnnounce", true))
        {
            std::string bossName = killed->GetName();
            ChatHandler(player->GetSession()).PSendSysMessage(
                "First kill of {} — you earned +1 talent point!", bossName);
        }

        LOG_INFO("module", "TalentProgression: {} earned first-kill talent for entry {}",
            player->GetName(), entry);
    }
}

class TalentProgression_Player : public PlayerScript
{
public:
    TalentProgression_Player() : PlayerScript("TalentProgression_Player", {
        PLAYERHOOK_ON_CALCULATE_TALENTS_POINTS,
        PLAYERHOOK_ON_CREATURE_KILL,
        PLAYERHOOK_ON_CREATURE_KILLED_BY_PET
    }) { }

    void OnPlayerCalculateTalentsPoints(Player const* player, uint32& talentPointsForLevel) override
    {
        if (!sConfigMgr->GetOption<bool>("TalentProgression.Enable", true))
            return;

        uint8 maxLevel = sConfigMgr->GetOption<uint8>("TalentProgression.MaxLevel", 80);
        uint8 level = std::min<uint8>(player->GetLevel(), maxLevel);

        // talentPointsForLevel currently == blizzBase + m_extraBonusTalentCount
        uint32 blizzBase = level < 10 ? 0 : uint32(level) - 9;
        uint32 bonus = talentPointsForLevel > blizzBase ? talentPointsForLevel - blizzBase : 0;
        talentPointsForLevel = uint32(level) + bonus;
    }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        TryAwardFirstKillTalent(killer, killed);
    }

    void OnPlayerCreatureKilledByPet(Player* petOwner, Creature* killed) override
    {
        TryAwardFirstKillTalent(petOwner, killed);
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

        if (sConfigMgr->GetOption<bool>("TalentProgression.Announce", false))
            LOG_INFO("server.loading",
                "TalentProgression: 1 talent/level through {} + first-kill boss bonuses",
                sConfigMgr->GetOption<uint8>("TalentProgression.MaxLevel", 80));
        else
            LOG_INFO("server.loading", "TalentProgression: module present");
    }
};

void AddTalentProgressionScripts()
{
    new TalentProgression_Player();
    new TalentProgression_World();
}
