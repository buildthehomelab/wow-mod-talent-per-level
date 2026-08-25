/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Copyright (C) Aldrynth / VenomekPL
 *
 * Talent curve: 1 point per level from 1–MaxLevel (default 80).
 * First-kill / retro: +1 bonus talent per enabled boss credit (CharDB),
 * with login reconcile for config clawback and achievement retro.
 * Individual Progression hidden quests are not kill proof — IP fills skipped
 * ladder states (e.g. Onyxia also stamps "MC complete").
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Tokenize.h"
#include "UnitScript.h"
#include "WorldSession.h"
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    struct BossCreditDef
    {
        char const* id;
        char const* displayName;
        uint32 const* creatureEntries;
        uint8 creatureCount;
        uint32 const* achievementIds;
        uint8 achievementCount;
    };

    // --- creature entry tables ---
    uint32 const Ent_Azuregos[]        = { 6109 };
    uint32 const Ent_Kazzak[]          = { 12397 };
    uint32 const Ent_Ysondre[]         = { 14887 };
    uint32 const Ent_Lethon[]          = { 14888 };
    uint32 const Ent_Emeriss[]         = { 14889 };
    uint32 const Ent_Taerar[]          = { 14890 };
    uint32 const Ent_Doomwalker[]      = { 17711 };
    uint32 const Ent_DoomlordKazzak[]  = { 18728 };
    uint32 const Ent_Onyxia[]          = { 10184, 301000 }; // WotLK + IP classic
    uint32 const Ent_Ragnaros[]        = { 11502 };
    uint32 const Ent_Nefarian[]        = { 11583 };
    uint32 const Ent_Hakkar[]          = { 14834 };
    uint32 const Ent_Ossirian[]        = { 15339 };
    uint32 const Ent_Cthun[]           = { 15727 };
    uint32 const Ent_KtClassic[]       = { 351019 }; // IP Naxx40
    uint32 const Ent_KtWotlk[]         = { 15990 };
    uint32 const Ent_Gruul[]           = { 19044 };
    uint32 const Ent_Magtheridon[]     = { 17257 };
    uint32 const Ent_Vashj[]           = { 21212 };
    uint32 const Ent_Kaelthas[]        = { 19622 };
    uint32 const Ent_Archimonde[]      = { 17968 };
    uint32 const Ent_Illidan[]         = { 22917 };
    uint32 const Ent_Zuljin[]          = { 23863 };
    uint32 const Ent_Kiljaeden[]       = { 25315 };
    uint32 const Ent_Malygos[]         = { 28859 };
    uint32 const Ent_Sartharion[]      = { 28860 };
    uint32 const Ent_Yogg[]            = { 33288 };
    uint32 const Ent_Anubarak[]        = { 34564 };
    uint32 const Ent_LichKing[]        = { 36597 };
    uint32 const Ent_Halion[]          = { 39863 };
    uint32 const Ent_Algalon[]         = { 32871 };

    // --- achievement tables (any match = proven) ---
    uint32 const Ach_Onyxia[]          = { 684 };
    uint32 const Ach_Ragnaros[]        = { 686 };
    uint32 const Ach_Nefarian[]        = { 685 };
    uint32 const Ach_Hakkar[]          = { 688 };
    uint32 const Ach_Ossirian[]        = { 689 };
    uint32 const Ach_Cthun[]           = { 687 };
    uint32 const Ach_KtClassic[]       = { 533 };          // IP custom
    uint32 const Ach_KtWotlk[]         = { 574, 575 };
    uint32 const Ach_Gruul[]           = { 692 };
    uint32 const Ach_Magtheridon[]     = { 693 };
    uint32 const Ach_Vashj[]           = { 694 };
    uint32 const Ach_Kaelthas[]        = { 696 };
    uint32 const Ach_Archimonde[]      = { 695 };
    uint32 const Ach_Illidan[]         = { 697 };
    uint32 const Ach_Zuljin[]          = { 691 };
    uint32 const Ach_Kiljaeden[]       = { 698 };
    uint32 const Ach_Malygos[]         = { 622, 623 };
    uint32 const Ach_Sartharion[]      = { 1876, 625 };
    uint32 const Ach_Yogg[]            = { 2892, 2893 };
    uint32 const Ach_Anubarak[]        = { 3916, 3917 };
    uint32 const Ach_LichKing[]        = { 4530, 4597 };
    uint32 const Ach_Halion[]          = { 4815, 4817 };
    uint32 const Ach_Algalon[]         = { 3036, 3037 };

#define TP_ARR(a) (a), uint8(sizeof(a) / sizeof((a)[0]))
#define TP_NONE   nullptr, uint8(0)

    BossCreditDef const Catalog[] = {
        { "azuregos",        "Azuregos",                 TP_ARR(Ent_Azuregos),       TP_NONE },
        { "kazzak",          "Lord Kazzak",              TP_ARR(Ent_Kazzak),         TP_NONE },
        { "ysondre",         "Ysondre",                  TP_ARR(Ent_Ysondre),        TP_NONE },
        { "lethon",          "Lethon",                   TP_ARR(Ent_Lethon),         TP_NONE },
        { "emeriss",         "Emeriss",                  TP_ARR(Ent_Emeriss),        TP_NONE },
        { "taerar",          "Taerar",                   TP_ARR(Ent_Taerar),         TP_NONE },
        { "doomwalker",      "Doomwalker",               TP_ARR(Ent_Doomwalker),     TP_NONE },
        { "doomlord_kazzak", "Doom Lord Kazzak",         TP_ARR(Ent_DoomlordKazzak), TP_NONE },

        { "onyxia",          "Onyxia",                   TP_ARR(Ent_Onyxia),         TP_ARR(Ach_Onyxia) },
        { "ragnaros",        "Ragnaros",                 TP_ARR(Ent_Ragnaros),       TP_ARR(Ach_Ragnaros) },
        { "nefarian",        "Nefarian",                 TP_ARR(Ent_Nefarian),       TP_ARR(Ach_Nefarian) },
        { "hakkar",          "Hakkar",                   TP_ARR(Ent_Hakkar),         TP_ARR(Ach_Hakkar) },
        { "ossirian",        "Ossirian the Unscarred",   TP_ARR(Ent_Ossirian),       TP_ARR(Ach_Ossirian) },
        { "cthun",           "C'Thun",                   TP_ARR(Ent_Cthun),          TP_ARR(Ach_Cthun) },
        { "kt_classic",      "Kel'Thuzad (Classic)",     TP_ARR(Ent_KtClassic),      TP_ARR(Ach_KtClassic) },
        { "kt_wotlk",        "Kel'Thuzad (WotLK)",       TP_ARR(Ent_KtWotlk),        TP_ARR(Ach_KtWotlk) },

        { "gruul",           "Gruul the Dragonkiller",   TP_ARR(Ent_Gruul),          TP_ARR(Ach_Gruul) },
        { "magtheridon",     "Magtheridon",              TP_ARR(Ent_Magtheridon),    TP_ARR(Ach_Magtheridon) },
        { "vashj",           "Lady Vashj",               TP_ARR(Ent_Vashj),          TP_ARR(Ach_Vashj) },
        { "kaelthas",        "Kael'thas Sunstrider",     TP_ARR(Ent_Kaelthas),       TP_ARR(Ach_Kaelthas) },
        { "archimonde",      "Archimonde",               TP_ARR(Ent_Archimonde),     TP_ARR(Ach_Archimonde) },
        { "illidan",         "Illidan Stormrage",        TP_ARR(Ent_Illidan),        TP_ARR(Ach_Illidan) },
        { "zuljin",          "Zul'jin",                  TP_ARR(Ent_Zuljin),         TP_ARR(Ach_Zuljin) },
        { "kiljaeden",       "Kil'jaeden",               TP_ARR(Ent_Kiljaeden),      TP_ARR(Ach_Kiljaeden) },

        { "malygos",         "Malygos",                  TP_ARR(Ent_Malygos),        TP_ARR(Ach_Malygos) },
        { "sartharion",      "Sartharion",               TP_ARR(Ent_Sartharion),     TP_ARR(Ach_Sartharion) },
        { "yogg_saron",      "Yogg-Saron",               TP_ARR(Ent_Yogg),           TP_ARR(Ach_Yogg) },
        { "anubarak",        "Anub'arak",                TP_ARR(Ent_Anubarak),       TP_ARR(Ach_Anubarak) },
        { "lich_king",       "The Lich King",            TP_ARR(Ent_LichKing),       TP_ARR(Ach_LichKing) },
        { "halion",          "Halion",                   TP_ARR(Ent_Halion),         TP_ARR(Ach_Halion) },
        { "algalon",         "Algalon the Observer",     TP_ARR(Ent_Algalon),        TP_ARR(Ach_Algalon) },
    };

#undef TP_ARR
#undef TP_NONE

    std::unordered_set<std::string> EnabledCredits;
    std::unordered_map<uint32, std::string> EntryToCredit;
    std::unordered_map<std::string, BossCreditDef const*> CreditById;

    std::string SanitizeCreditId(std::string const& creditId)
    {
        std::string out;
        out.reserve(creditId.size());
        for (char ch : creditId)
        {
            if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')
                out.push_back(ch);
        }
        return out;
    }

    void RebuildRuntimeMaps()
    {
        EnabledCredits.clear();
        EntryToCredit.clear();
        CreditById.clear();

        for (BossCreditDef const& def : Catalog)
            CreditById.emplace(def.id, &def);

        std::string raw = sConfigMgr->GetOption<std::string>("TalentProgression.EnabledCredits", "");
        if (raw.size() >= 2 && ((raw.front() == '"' && raw.back() == '"') || (raw.front() == '\'' && raw.back() == '\'')))
            raw = raw.substr(1, raw.size() - 2);

        if (raw.empty())
        {
            for (BossCreditDef const& def : Catalog)
                EnabledCredits.insert(def.id);
        }
        else
        {
            for (std::string_view token : Acore::Tokenize(raw, ',', false))
            {
                while (!token.empty() && (token.front() == ' ' || token.front() == '\t'))
                    token.remove_prefix(1);
                while (!token.empty() && (token.back() == ' ' || token.back() == '\t'))
                    token.remove_suffix(1);
                if (token.empty())
                    continue;

                std::string id(token);
                if (CreditById.find(id) == CreditById.end())
                {
                    LOG_WARN("module", "TalentProgression: unknown credit '{}' in EnabledCredits — ignored", id);
                    continue;
                }
                EnabledCredits.insert(id);
            }
        }

        for (std::string const& id : EnabledCredits)
        {
            BossCreditDef const* def = CreditById[id];
            for (uint8 i = 0; i < def->creatureCount; ++i)
                EntryToCredit[def->creatureEntries[i]] = id;
        }

        LOG_INFO("server.loading", "TalentProgression: {} boss credits enabled", EnabledCredits.size());
    }

    bool IsCreditEnabled(std::string const& creditId)
    {
        return EnabledCredits.find(creditId) != EnabledCredits.end();
    }

    bool PlayerHasCredit(uint32 guid, std::string const& creditId)
    {
        std::string safe = SanitizeCreditId(creditId);
        if (safe.empty())
            return false;

        QueryResult result = CharacterDatabase.Query(
            "SELECT 1 FROM aldr_first_kill_talent WHERE guid = {} AND credit_id = '{}'",
            guid, safe);
        return result != nullptr;
    }

    std::unordered_set<std::string> LoadPlayerCredits(uint32 guid)
    {
        std::unordered_set<std::string> have;
        QueryResult result = CharacterDatabase.Query(
            "SELECT credit_id FROM aldr_first_kill_talent WHERE guid = {}", guid);
        if (!result)
            return have;

        do
        {
            have.insert(SanitizeCreditId((*result)[0].Get<std::string>()));
        } while (result->NextRow());

        return have;
    }

    void SafeRemoveBonusTalent(Player* player, uint32 count)
    {
        if (!player || !count)
            return;

        uint32 cur = player->GetBonusTalentCount();
        if (cur <= count)
            player->SetBonusTalentCount(0);
        else
            player->RemoveBonusTalent(count);
    }

    bool HasProvenCredit(Player* player, BossCreditDef const& def)
    {
        // Achievements only. Individual Progression hidden quests (66000+N) mean
        // "tier reached", and IP fills skipped states when a later boss is killed.
        for (uint8 i = 0; i < def.achievementCount; ++i)
            if (def.achievementIds[i] && player->HasAchieved(def.achievementIds[i]))
                return true;

        return false;
    }

    bool InsertCreditRow(uint32 guid, std::string const& creditId, uint32 sourceEntry)
    {
        std::string safe = SanitizeCreditId(creditId);
        if (safe.empty())
            return false;

        if (PlayerHasCredit(guid, safe))
            return false;

        CharacterDatabase.DirectExecute(
            "INSERT IGNORE INTO aldr_first_kill_talent (guid, credit_id, source_entry) VALUES ({}, '{}', {})",
            guid, safe, sourceEntry);

        return true;
    }

    void DeleteCreditRow(uint32 guid, std::string const& creditId)
    {
        std::string safe = SanitizeCreditId(creditId);
        if (safe.empty())
            return;

        CharacterDatabase.DirectExecute(
            "DELETE FROM aldr_first_kill_talent WHERE guid = {} AND credit_id = '{}'",
            guid, safe);
    }

    void AwardCredit(Player* player, BossCreditDef const& def, uint32 sourceEntry, char const* reason, bool refreshTalents)
    {
        if (!player)
            return;

        uint32 guid = player->GetGUID().GetCounter();
        if (!InsertCreditRow(guid, def.id, sourceEntry))
            return;

        player->RewardExtraBonusTalentPoints(1);
        if (refreshTalents)
            player->InitTalentForLevel();

        if (sConfigMgr->GetOption<bool>("TalentProgression.FirstKillAnnounce", true))
            ChatHandler(player->GetSession()).PSendSysMessage(
                "Talent credit: {} — +1 talent point ({})", def.displayName, reason);

        LOG_INFO("module", "TalentProgression: {} awarded '{}' via {} (entry {})",
            player->GetName(), def.id, reason, sourceEntry);
    }

    void RevokeCredit(Player* player, std::string const& creditId, bool whisper, bool refreshTalents)
    {
        if (!player)
            return;

        uint32 guid = player->GetGUID().GetCounter();
        DeleteCreditRow(guid, creditId);
        SafeRemoveBonusTalent(player, 1);

        char const* name = creditId.c_str();
        if (auto it = CreditById.find(creditId); it != CreditById.end())
            name = it->second->displayName;

        if (whisper && sConfigMgr->GetOption<bool>("TalentProgression.FirstKillAnnounce", true))
            ChatHandler(player->GetSession()).PSendSysMessage(
                "Talent credit removed: {} — -1 talent point (no longer on the allowlist)", name);

        if (refreshTalents)
            player->InitTalentForLevel();

        LOG_INFO("module", "TalentProgression: {} revoked '{}'", player->GetName(), creditId);
    }

    void TryAwardKillCredit(Player* player, Creature* killed)
    {
        if (!player || !killed)
            return;

        if (!sConfigMgr->GetOption<bool>("TalentProgression.Enable", true))
            return;

        if (!sConfigMgr->GetOption<bool>("TalentProgression.FirstKillEnable", true))
            return;

        auto it = EntryToCredit.find(killed->GetEntry());
        if (it == EntryToCredit.end())
            return;

        auto defIt = CreditById.find(it->second);
        if (defIt == CreditById.end() || !IsCreditEnabled(defIt->second->id))
            return;

        AwardCredit(player, *defIt->second, killed->GetEntry(), "first kill", true);
    }

    Player* ResolveCreditPlayer(Unit* unit)
    {
        if (!unit)
            return nullptr;

        if (Player* player = unit->ToPlayer())
            return player;

        return unit->GetCharmerOrOwnerPlayerOrPlayerItself();
    }

    bool IsRealPlayer(Player* player)
    {
        return player && player->GetSession() && !player->GetSession()->IsBot();
    }

    void AwardGroupMembers(Group* group, Creature* killed, Unit* killer, std::function<void(Player*)> const& consider)
    {
        if (!group)
            return;

        for (GroupReference* itr = group->GetFirstMember(); itr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member)
                continue;

            if (killer == member || member->IsAtGroupRewardDistance(killed))
                consider(member);
        }
    }

    // Last-hit is not enough: playerbots and Taerar shades often land the killing blow.
    // Credit every real player who would share XP / loot for this kill.
    void AwardKillCreditToParticipants(Creature* killed, Unit* killer)
    {
        if (!killed)
            return;

        if (EntryToCredit.find(killed->GetEntry()) == EntryToCredit.end())
            return;

        std::unordered_set<uint32> seen;
        auto consider = [&](Player* player)
        {
            if (!IsRealPlayer(player))
                return;

            if (!seen.insert(player->GetGUID().GetCounter()).second)
                return;

            TryAwardKillCredit(player, killed);
        };

        if (Player* killerPlayer = ResolveCreditPlayer(killer))
        {
            consider(killerPlayer);
            AwardGroupMembers(killerPlayer->GetGroup(), killed, killer, consider);
        }

        if (Player* loot = killed->GetLootRecipient())
        {
            consider(loot);
            Group* lootGroup = killed->GetLootRecipientGroup();
            AwardGroupMembers(lootGroup ? lootGroup : loot->GetGroup(), killed, killer, consider);
        }
        else
            AwardGroupMembers(killed->GetLootRecipientGroup(), killed, killer, consider);

        for (ThreatReference const* ref : killed->GetThreatMgr().GetUnsortedThreatList())
        {
            if (!ref)
                continue;

            if (Player* tagged = ResolveCreditPlayer(ref->GetVictim()))
                if (killer == tagged || tagged->IsAtGroupRewardDistance(killed))
                    consider(tagged);
        }
    }

    void ReconcilePlayerCredits(Player* player)
    {
        if (!player)
            return;

        if (!sConfigMgr->GetOption<bool>("TalentProgression.Enable", true))
            return;

        if (!sConfigMgr->GetOption<bool>("TalentProgression.FirstKillEnable", true))
            return;

        if (!sConfigMgr->GetOption<bool>("TalentProgression.ReconcileOnLogin", true))
            return;

        uint32 guid = player->GetGUID().GetCounter();
        std::unordered_set<std::string> have = LoadPlayerCredits(guid);
        bool changed = false;

        for (std::string const& creditId : have)
        {
            if (creditId.empty() || IsCreditEnabled(creditId))
                continue;

            RevokeCredit(player, creditId, true, false);
            changed = true;
        }

        if (sConfigMgr->GetOption<bool>("TalentProgression.RetroEnable", true))
        {
            for (std::string const& creditId : EnabledCredits)
            {
                if (PlayerHasCredit(guid, creditId))
                    continue;

                auto defIt = CreditById.find(creditId);
                if (defIt == CreditById.end() || !HasProvenCredit(player, *defIt->second))
                    continue;

                AwardCredit(player, *defIt->second, 0, "retro", false);
                changed = true;
            }
        }

        // Rows can exist without a bonus point (missed last-hit, GM insert). Catch up on login.
        have = LoadPlayerCredits(guid);
        uint32 enabledCreditCount = 0;
        for (std::string const& creditId : have)
            if (!creditId.empty() && IsCreditEnabled(creditId))
                ++enabledCreditCount;

        uint32 bonus = player->GetBonusTalentCount();
        if (enabledCreditCount > bonus)
        {
            uint32 missing = enabledCreditCount - bonus;
            player->RewardExtraBonusTalentPoints(missing);
            changed = true;
            LOG_INFO("module", "TalentProgression: {} synced +{} bonus talent(s) to {} credit(s)",
                player->GetName(), missing, enabledCreditCount);
        }

        if (changed)
            player->InitTalentForLevel();
    }
}

class TalentProgression_Player : public PlayerScript
{
public:
    TalentProgression_Player() : PlayerScript("TalentProgression_Player", {
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_CALCULATE_TALENTS_POINTS
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        ReconcilePlayerCredits(player);
    }

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

class TalentProgression_Unit : public UnitScript
{
public:
    TalentProgression_Unit() : UnitScript("TalentProgression_Unit", true, {
        UNITHOOK_ON_UNIT_DEATH
    }) { }

    void OnUnitDeath(Unit* unit, Unit* killer) override
    {
        if (!unit || !unit->IsCreature())
            return;

        AwardKillCreditToParticipants(unit->ToCreature(), killer);
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

        RebuildRuntimeMaps();

        if (sConfigMgr->GetOption<bool>("TalentProgression.Announce", false))
            LOG_INFO("server.loading",
                "TalentProgression: 1 talent/level through {} + {} first-kill credits (reconcile/retro)",
                sConfigMgr->GetOption<uint8>("TalentProgression.MaxLevel", 80),
                EnabledCredits.size());
        else
            LOG_INFO("server.loading", "TalentProgression: module present ({} credits enabled)",
                EnabledCredits.size());
    }
};

void AddTalentProgressionScripts()
{
    new TalentProgression_Player();
    new TalentProgression_Unit();
    new TalentProgression_World();
}
