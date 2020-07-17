﻿/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2010-2015 Rising Gods <http://www.rising-gods.de/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "Player.h"

/*######
## npc_captured_arkonarin
######*/

enum
{
    // Texts
    SAY_ESCORT_START             = 0,
    SAY_FIRST_STOP               = 1,
    SAY_SECOND_STOP              = 2,
    SAY_AGGRO                    = 2,
    SAY_FOUND_EQUIPMENT          = 3,
    SAY_ESCAPE_DEMONS            = 4,
    SAY_FRESH_AIR                = 5,
    SAY_TREY_BETRAYER            = 6,
    SAY_TREY                     = 7,
    SAY_TREY_ATTACK              = 8,
    SAY_ESCORT_COMPLETE          = 9,
    
    // Spells
    SPELL_STRENGHT_ARKONARIN     = 18163,
    //SPELL_MORTAL_STRIKE          = 16856,
    //SPELL_CLEAVE                 = 15496,
    
    // Quests
    QUEST_ID_RESCUE_JAEDENAR     = 5203,
    
    // Creatures
    NPC_JAEDENAR_LEGIONNAIRE     = 9862,
    NPC_SPIRT_TREY               = 11141,
    
    // Gameobjects
    GO_ARKONARIN_CHEST           = 176225,
    GO_ARKONARIN_CAGE            = 176306,
    
    // Factions
    FACTION_ESCORTEE_HORD        = 775,
    FACTION_ESCORTEE_ALLIANCE    = 774,
    DISPLAY_NORMAL               = 10407,
    DISPLAY_EQUIP                = 10402
};

class npc_captured_arkonarin : public CreatureScript
{
    public:
        npc_captured_arkonarin() : CreatureScript("npc_captured_arkonarin") { }
    
        struct npc_captured_arkonarinAI : public npc_escortAI
        {
            npc_captured_arkonarinAI(Creature* creature) : npc_escortAI(creature) {}
    
            void WaypointReached(uint32 waypointId) override
            {
                Player* player = GetPlayerForEscort();
                if (!player)
                    return;
    
                switch (waypointId)
                {
                    case 0:
                        Talk(SAY_ESCORT_START);
                        break;
                    case 14:
                        Talk(SAY_FIRST_STOP);
                        break;
                    case 34:
                        Talk(SAY_SECOND_STOP);
                        SetRun(true);
                        break;
                    case 38:
                        if (GameObject* Cage = me->FindNearestGameObject(GO_ARKONARIN_CHEST, 5.0f))
                            Cage->SetGoState(GO_STATE_ACTIVE);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
                        break;
                    case 39:
                        DoCastSelf(SPELL_STRENGHT_ARKONARIN, true);
                        break;
                    case 40:
                        if (Player* player = GetPlayerForEscort())
                            me->SetFacingToObject(player);
                        Talk(SAY_FOUND_EQUIPMENT);
                        me->SetDisplayId(DISPLAY_EQUIP);
                        break;
                    case 41:
                        Talk(SAY_ESCAPE_DEMONS);
                        me->SummonCreature(NPC_JAEDENAR_LEGIONNAIRE, 5082.068f, -490.084f, 296.856f, 5.15f, TEMPSUMMON_DEAD_DESPAWN, 60 * IN_MILLISECONDS);
                        me->SummonCreature(NPC_JAEDENAR_LEGIONNAIRE, 5084.135f, -489.187f, 296.832f, 5.15f, TEMPSUMMON_DEAD_DESPAWN, 60 * IN_MILLISECONDS);
                        me->SummonCreature(NPC_JAEDENAR_LEGIONNAIRE, 5085.676f, -488.518f, 296.824f, 5.15f, TEMPSUMMON_DEAD_DESPAWN, 60 * IN_MILLISECONDS);
                        break;
                    case 43:
                        SetRun(false);
                        break;
                    case 104:
                        Talk(SAY_FRESH_AIR);
                        break;
                    case 105:
                        me->SummonCreature(NPC_SPIRT_TREY, 4844.839f, -395.763f, 350.603f, 6.25f, TEMPSUMMON_DEAD_DESPAWN, 60 * IN_MILLISECONDS);
                        break;
                    case 106:
                        Talk(SAY_TREY);
                        break;
                    case 107:
                        if (Unit* target = me->FindNearestCreature(NPC_SPIRT_TREY, 1000.0f, true))
                            me->AI()->AttackStart(target);
                        break;
                    case 108:
                        Talk(SAY_ESCORT_COMPLETE);
                        break;
                    case 109:
                        player->GroupEventHappens(QUEST_ID_RESCUE_JAEDENAR, me);
                        break;
                    case 110:
                        me->SetImmuneToNPC(true);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);                    
                        me->setFaction(FACTION_FRIENDLY_TO_ALL);
                        me->SetDisplayId(DISPLAY_NORMAL);
                        break;
                }
            }
    
            void JustSummoned(Creature* summoned) override
            {
                if (summoned->GetEntry() == NPC_JAEDENAR_LEGIONNAIRE)
                    summoned->AI()->AttackStart(me);
                else if (summoned->GetEntry() == NPC_SPIRT_TREY)
                {
                    Talk(SAY_TREY_BETRAYER);
                }
            }
    
            void JustDied(Unit* /*killer*/) override
            {
                me->SetImmuneToNPC(true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);                    
                me->setFaction(FACTION_FRIENDLY_TO_ALL);           
                me->SetDisplayId(DISPLAY_NORMAL);
    
                if (Player* player = GetPlayerForEscort())
                    player->FailQuest(QUEST_ID_RESCUE_JAEDENAR);
            }
    
            void EnterCombat(Unit* who) override
            {
                if (who->GetEntry() == NPC_SPIRT_TREY)
                    Talk(SAY_TREY_ATTACK);
                else if (roll_chance_i(25))
                    Talk(SAY_AGGRO);
            }
    
            void Reset() override { }
        };
    
        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
        {
            if (quest->GetQuestId() == QUEST_ID_RESCUE_JAEDENAR)
            {
                if (GameObject* Cage = creature->FindNearestGameObject(GO_ARKONARIN_CAGE, 10.0f))
                    Cage->SetGoState(GO_STATE_ACTIVE);
    
                creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                creature->SetImmuneToNPC(false);
                creature->SetStandState(UNIT_STAND_STATE_STAND);
                if (npc_escortAI* pEscortAI = CAST_AI(npc_captured_arkonarin::npc_captured_arkonarinAI, creature->AI()))
                    pEscortAI->Start(true, false, player->GetGUID());     
    
                if (player->GetTeam() == HORDE)
                    creature->setFaction(FACTION_ESCORTEE_HORD);
    
                if (player->GetTeam() == ALLIANCE)
                    creature->setFaction(FACTION_ESCORTEE_ALLIANCE);
    
            }
            return true;
        }
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_captured_arkonarinAI(creature);
        }
};

enum CorruptedFlowerMisc
{
    // Gameobjects
    GO_CLEANSED_SONGFLOWER           = 164882,
    GO_CLEANSED_NIGHT_DRAGON         = 164881,
    GO_CLEANSED_WINDBLOSSOM          = 164884,
    GO_CLEANSED_WHIPPER_ROOT         = 174687,
    
    // Spells
    SPELL_SONGFLOWER_SERENADE        = 15366,
    PLANT_SPAWN_DURATION             = 60
};

class go_corrupted_plant : public GameObjectScript
{
    public:
        go_corrupted_plant() : GameObjectScript("go_corrupted_plant") { }
    
        bool OnQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 /*opt*/)
        {
            float fX, fY, fZ;
            go->GetPosition(fX, fY, fZ);
            go->SetLootState(GO_JUST_DEACTIVATED);
            go->SetRespawnTime(PLANT_SPAWN_DURATION);

            switch (quest->GetQuestId())
            {
                case 2278: // check Cleansed Songflower Quest IDs
                case 2523:
                case 3363:
                case 4113:
                case 4116:
                case 4118:
                case 4401:
                case 4464:
                case 4465:
                    player->SummonGameObject(GO_CLEANSED_SONGFLOWER, fX, fY, fZ, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, PLANT_SPAWN_DURATION);
                    break;
                case 4119: // check Cleansed Night Dragon Quest IDs
                case 4447:
                case 4448:
                case 4462:
                    player->SummonGameObject(GO_CLEANSED_NIGHT_DRAGON, fX, fY, fZ, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, PLANT_SPAWN_DURATION);
                    break;
                case 996: // check Cleansed Windblossom Quest IDs
                case 998:
                case 1514:
                case 4115:
                case 4221:
                case 4222:
                case 4343:
                case 4403:
                case 4466:
                case 4467:
                    player->SummonGameObject(GO_CLEANSED_WINDBLOSSOM, fX, fY, fZ, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, PLANT_SPAWN_DURATION);
                    break;
                case 4117: // check Cleansed Whipper Root Quest IDs
                case 4443:
                case 4444:
                case 4445:
                case 4446:
                case 4461:
                    player->SummonGameObject(GO_CLEANSED_WHIPPER_ROOT, fX, fY, fZ, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, PLANT_SPAWN_DURATION);
                    break;
                default:
                    break;
            }        

            return true;
        }
};

class go_cleansed_songflower : public GameObjectScript
{
    public:
        go_cleansed_songflower() : GameObjectScript("go_cleansed_songflower") { }

        bool OnGossipHello(Player* player, GameObject* /*go*/) override
        {
            player->CastSpell(player, SPELL_SONGFLOWER_SERENADE, true);
            return false;
        }
};

/*####
# npc_arei
####*/

enum AreiMisc
{
    // Texts
    SAY_AREI_ESCORT_START            = 0,
    SAY_ATTACK_IRONTREE              = 1,
    SAY_ATTACK_TOXIC_HORROR          = 2,
    SAY_EXIT_WOODS                   = 3,
    SAY_CLEAR_PATH                   = 4,
    SAY_ASHENVALE                    = 5,
    SAY_TRANSFORM                    = 6,
    SAY_LIFT_CURSE                   = 7,
    SAY_AREI_ESCORT_COMPLETE         = 8,

    // Spells
    SPELL_WITHER_STRIKE              = 5337,
    SPELL_AREI_TRANSFORM             = 14888,

    // Creatures
    NPC_AREI                         = 9598,
    NPC_TOXIC_HORROR                 = 7132,
    NPC_IRONTREE_WANDERER            = 7138,
    NPC_IRONTREE_STOMPER             = 7139,
    
    // Quests
    QUEST_ID_ANCIENT_SPIRIT          = 4261,

    // Events
    EVENT_WITHER_STRIKE              = 1,
    EVENT_DIALOGUE_STEP_1            = 2,
    EVENT_DIALOGUE_STEP_2            = 3,
    EVENT_DIALOGUE_STEP_3            = 4,
    EVENT_DIALOGUE_STEP_4            = 5,
    EVENT_DIALOGUE_STEP_5            = 6,
};

class npc_arei : public CreatureScript
{
    public:
        npc_arei() : CreatureScript("npc_arei") { }

        bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest) override
        {
            if (quest->GetQuestId() == QUEST_ID_ANCIENT_SPIRIT)
            {
                if (npc_areiAI* EscortAI = CAST_AI(npc_arei::npc_areiAI, creature->AI()))
                {
                    creature->setFaction(FACTION_ESCORT_N_NEUTRAL_ACTIVE);
                    creature->AI()->Talk(SAY_AREI_ESCORT_START, player);
                    EscortAI->Start(false, false, player->GetGUID());
                }
            }
            return true;
        }

        struct npc_areiAI : public npc_escortAI
        {
            npc_areiAI(Creature* creature) : npc_escortAI(creature)
            {
                m_bAggroIrontree = false;
                m_bAggroHorror = false;
                StartEndEvent = false;
                Reset();
                DeathCounter = 0;
            }

            void Reset() override
            {
                _events.Reset();
            }

            void EnterCombat(Unit* who) override
            {
                if ((who->GetEntry() == NPC_IRONTREE_WANDERER || who->GetEntry() == NPC_IRONTREE_STOMPER) && !m_bAggroIrontree)
                {
                    Talk(SAY_ATTACK_IRONTREE);
                    m_bAggroIrontree = true;
                }
                else if (who->GetEntry() == NPC_TOXIC_HORROR && !m_bAggroHorror)
                {
                    if (Player* player = GetPlayerForEscort())
                        Talk(SAY_ATTACK_TOXIC_HORROR, player);
                    m_bAggroHorror = true;
                }

                _events.RescheduleEvent(EVENT_WITHER_STRIKE, urand(1, 2) * IN_MILLISECONDS);
            }

            void JustSummoned(Creature* summoned) override
            {
                switch (summoned->GetEntry())
                {
                    case NPC_IRONTREE_STOMPER:
                        if (Player* player = GetPlayerForEscort())
                            Talk(SAY_EXIT_WOODS, player);
                        // no break;
                    case NPC_IRONTREE_WANDERER:
                        summoned->AI()->AttackStart(me);
                        break;
                }
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
            {
                if (summon->GetEntry() == NPC_IRONTREE_STOMPER || summon->GetEntry() == NPC_IRONTREE_WANDERER)
                    ++DeathCounter;
            }

            void WaypointReached(uint32 waypointId) override
            {
                Player* player = GetPlayerForEscort();
                if (!player)
                    return;

                switch (waypointId)
                {
                    case 36:
                        SetEscortPaused(true);
                        me->SummonCreature(NPC_IRONTREE_STOMPER, 6573.321f, -1195.213f, 442.489f, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 60 * IN_MILLISECONDS);
                        me->SummonCreature(NPC_IRONTREE_WANDERER, 6573.240f, -1213.475f, 443.643f, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 60 * IN_MILLISECONDS);
                        me->SummonCreature(NPC_IRONTREE_WANDERER, 6583.354f, -1209.811f, 444.769f, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 60 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (Player* player = GetPlayerForEscort())
                    player->FailQuest(QUEST_ID_ANCIENT_SPIRIT);
            }

            void UpdateAI(uint32 diff) override
            {
                npc_escortAI::UpdateAI(diff);
                _events.Update(diff);

                if (DeathCounter == 3 && !StartEndEvent)
                {
                    StartEndEvent = true;
                    _events.ScheduleEvent(EVENT_DIALOGUE_STEP_1, 1 * IN_MILLISECONDS);
                }

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DIALOGUE_STEP_1:
                            Talk(SAY_CLEAR_PATH);
                            _events.ScheduleEvent(EVENT_DIALOGUE_STEP_2, 4 * IN_MILLISECONDS);
                            break;
                        case EVENT_DIALOGUE_STEP_2:
                            if (Player* player = GetPlayerForEscort())
                                Talk(SAY_ASHENVALE, player);
                            _events.ScheduleEvent(EVENT_DIALOGUE_STEP_3, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_DIALOGUE_STEP_3:
                            Talk(SAY_TRANSFORM);
                            _events.ScheduleEvent(EVENT_DIALOGUE_STEP_4, 3 * IN_MILLISECONDS);
                            break;
                        case EVENT_DIALOGUE_STEP_4:
                            DoCastSelf(SPELL_AREI_TRANSFORM);
                            if (Player* player = GetPlayerForEscort())
                                Talk(SAY_LIFT_CURSE, player);
                            _events.ScheduleEvent(EVENT_DIALOGUE_STEP_5, 7 * IN_MILLISECONDS);
                            break;
                        case EVENT_DIALOGUE_STEP_5:
                            if (Player* player = GetPlayerForEscort())
                            {
                                Talk(SAY_AREI_ESCORT_COMPLETE, player);
                                player->GroupEventHappens(QUEST_ID_ANCIENT_SPIRIT, me);
                                me->DespawnOrUnsummon(10 * IN_MILLISECONDS);
                            }
                            break;
                        case EVENT_WITHER_STRIKE:
                            if(UpdateVictim())
                            {
                                DoCastVictim(SPELL_WITHER_STRIKE);
                                _events.ScheduleEvent(EVENT_WITHER_STRIKE, urand(3, 6) * IN_MILLISECONDS);
                            }
                            break;
                        default:
                            break;
                    }
                }

                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
            
        private:
            bool m_bAggroIrontree;
            bool m_bAggroHorror;
            bool StartEndEvent;
            uint8 DeathCounter;
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_areiAI(creature);
        }
};

void AddSC_felwood_rg()
{
    new npc_captured_arkonarin();
    new go_corrupted_plant();
    new go_cleansed_songflower();
    new npc_arei();
}
