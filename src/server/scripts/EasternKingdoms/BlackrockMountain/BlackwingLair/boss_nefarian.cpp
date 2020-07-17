/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_Nefarian
SD%Complete: 80
SDComment: Some issues with class calls effecting more than one class
SDCategory: Blackwing Lair
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackwing_lair.h"

enum Events
{
    // Victor Nefarius
    EVENT_SPAWN_ADD     = 1,
    EVENT_SHADOW_BOLT,
    EVENT_FEAR,
    EVENT_MIND_CONTROL,

    // Nefarian
    EVENT_SHADOWFLAME,
  //EVENT_FEAR,
    EVENT_VEILOFSHADOW,
    EVENT_CLEAVE,
    EVENT_TAILLASH,
    EVENT_CLASSCALL
};

enum Says
{
    // Victor Nefarius
    SAY_GAMESBEGIN_1        = 0,
    SAY_GAMESBEGIN_2        = 1,
    //SAY_VAEL_INTRO          = 2, Not used - when he corrupts Vaelastrasz

    // Nefarian
    SAY_RANDOM              = 0,
    SAY_RAISE_SKELETONS     = 1,
    SAY_SLAY                = 2,
    SAY_DEATH               = 3,

    SAY_MAGE                = 4,
    SAY_WARRIOR             = 5,
    SAY_DRUID               = 6,
    SAY_PRIEST              = 7,
    SAY_PALADIN             = 8,
    SAY_SHAMAN              = 9,
    SAY_WARLOCK             = 10,
    SAY_HUNTER              = 11,
    SAY_ROGUE               = 12,
};

enum Creatures
{
    NPC_BRONZE_DRAKANOID       = 14263,
    NPCE_BLUE_DRAKANOID        = 14261,
    NPC_RED_DRAKANOID          = 14264,
    NPC_GREEN_DRAKANOID        = 14262,
    NPCE_BLACK_DRAKANOID       = 14265,
    NPC_CHROMATIC_DRAKANOID    = 14302,

    BONE_CONSTRUCT             = 14605,
};

enum Spells
{
    // Victor Nefarius
    SPELL_SHADOWBOLT            = 22677,
    SPELL_SHADOWBOLT_VOLLEY     = 22665,
    SPELL_SHADOW_COMMAND        = 22667,
    SPELL_FEAR                  = 22678,

    SPELL_NEFARIANS_BARRIER     = 22663,

    // Nefarian
    SPELL_SHADOWFLAME_INITIAL   = 22992,
    SPELL_SHADOWFLAME           = 22539,
    SPELL_BELLOWINGROAR         = 22686,
    SPELL_VEILOFSHADOW          = 7068,
    SPELL_CLEAVE                = 20691,
    SPELL_TAILLASH              = 23364,

    SPELL_MAGE                  = 23410,     // wild magic
    SPELL_WARRIOR               = 23397,     // beserk
    SPELL_DRUID                 = 23398,     // cat form
    SPELL_PRIEST                = 23401,     // corrupted healing
    SPELL_PALADIN               = 23418,     // syphon blessing
    SPELL_SHAMAN                = 23425,     // totems
    SPELL_WARLOCK               = 23427,     // infernals
    SPELL_HUNTER                = 23436,     // bow broke
    SPELL_ROGUE                 = 23414      // Paralise
};

#define GOSSIP_ITEM_1           "I've made no mistakes."
#define GOSSIP_ITEM_2           "You have lost your mind, Nefarius. You speak in riddles."
#define GOSSIP_ITEM_3           "Please do."

Position const DrakeSpawnLoc[2] = // drakonid
{
    {-7591.151855f, -1204.051880f, 476.800476f, 3.0f},
    {-7514.598633f, -1150.448853f, 476.796570f, 3.0f}
};

Position const NefarianLoc[2] =
{
    {-7495.615723f, -1337.156738f, 507.922516f, 3.0f}, // nefarian spawn old: {-7449.763672f, -1387.816040f, 526.783691f, 3.0f}
    {-7535.456543f, -1279.562500f, 476.798706f, 3.0f}  // nefarian move
};

uint32 const Entry[5] = {NPC_BRONZE_DRAKANOID, NPCE_BLUE_DRAKANOID, NPC_RED_DRAKANOID, NPC_GREEN_DRAKANOID, NPCE_BLACK_DRAKANOID};

class boss_victor_nefarius : public CreatureScript
{
public:
    boss_victor_nefarius() : CreatureScript("boss_victor_nefarius") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                player->SEND_GOSSIP_MENU(7198, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                player->SEND_GOSSIP_MENU(7199, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                player->CLOSE_GOSSIP_MENU();
                creature->AI()->Talk(SAY_GAMESBEGIN_1);
                CAST_AI(boss_victor_nefarius::boss_victor_nefariusAI, creature->AI())->BeginEvent(player);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
            if (instance->GetBossState(BOSS_CHOMAGGUS) != DONE || instance->GetBossState(BOSS_NEFARIAN) == DONE)
                return false;

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        player->SEND_GOSSIP_MENU(7134, creature->GetGUID());
        return true;
    }

    struct boss_victor_nefariusAI : public BossAI
    {
        boss_victor_nefariusAI(Creature* creature) : BossAI(creature, BOSS_NEFARIAN) { }

        void Reset()
        {
            SpawnedAdds = 0;

            if (!me->FindNearestCreature(NPC_NEFARIAN, 1000.0f, true))
                _Reset();
            SpawnedAdds = 0;

            me->SetVisible(true);
            me->SetPhaseMask(1, true);
            me->SetUInt32Value(UNIT_NPC_FLAGS, 1);
            me->setFaction(35);
            me->SetStandState(UNIT_STAND_STATE_SIT_HIGH_CHAIR);
            me->RemoveAura(SPELL_NEFARIANS_BARRIER);
        }

        void JustReachedHome()
        {
            Reset();
        }

        void BeginEvent(Player* target)
        {
            _EnterCombat();

            Talk(SAY_GAMESBEGIN_2);

            me->setFaction(103);
            me->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
            DoCast(me, SPELL_NEFARIANS_BARRIER);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            AttackStart(target);
            events.ScheduleEvent(EVENT_SHADOW_BOLT, urand(3, 10)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FEAR, urand(10, 20)*IN_MILLISECONDS);
            //events.ScheduleEvent(EVENT_MIND_CONTROL, urand(30, 35)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SPAWN_ADD, 10*IN_MILLISECONDS);
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
        {
            if (summon->GetEntry() != NPC_NEFARIAN)
            {
                summon->UpdateEntry(BONE_CONSTRUCT);
                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                summon->SetReactState(REACT_PASSIVE);
                summon->SetStandState(UNIT_STAND_STATE_DEAD);
            }
        }

        void JustSummoned(Creature* /*summon*/) {}

        void UpdateAI(uint32 diff)
        {
            // Only do this if we haven't spawned nefarian yet
            if (UpdateVictim() && SpawnedAdds <= 42)
            {
                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHADOW_BOLT:
                            switch (urand(0, 1))
                            {
                                case 0:
                                    DoCastVictim(SPELL_SHADOWBOLT_VOLLEY);
                                    break;
                                case 1:
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                                        DoCast(target, SPELL_SHADOWBOLT);
                                    break;
                            }
                            DoResetThreat();
                            events.ScheduleEvent(EVENT_SHADOW_BOLT, urand(3, 10)*IN_MILLISECONDS);
                            break;
                        case EVENT_FEAR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                                DoCast(target, SPELL_FEAR);
                            events.ScheduleEvent(EVENT_FEAR, urand(10, 20)*IN_MILLISECONDS);
                            break;
                        case EVENT_MIND_CONTROL:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                                DoCast(target, SPELL_SHADOW_COMMAND);
                            events.ScheduleEvent(EVENT_MIND_CONTROL, urand(30, 35)*IN_MILLISECONDS);
                            break;
                        case EVENT_SPAWN_ADD:
                            for (uint8 i=0; i<2; ++i)
                            {
                                uint32 CreatureID;
                                if (urand(0, 2) == 0)
                                    CreatureID = NPC_CHROMATIC_DRAKANOID;
                                else
                                    CreatureID = Entry[urand(0, 4)];
                                if (Creature* dragon = me->SummonCreature(CreatureID, DrakeSpawnLoc[i]))
                                {
                                    dragon->setFaction(103);
                                    dragon->AI()->AttackStart(me->GetVictim());
                                }

                                if (++SpawnedAdds >= 42)
                                {
                                    if (Creature* nefarian = me->SummonCreature(NPC_NEFARIAN, NefarianLoc[0]))
                                    {
                                        nefarian->setActive(true);
                                        nefarian->SetCanFly(true);
                                        nefarian->SetDisableGravity(true);
                                        nefarian->AI()->DoCastAOE(SPELL_SHADOWFLAME_INITIAL);
                                        nefarian->GetMotionMaster()->MovePoint(1, NefarianLoc[1]);
                                    }
                                    events.CancelEvent(EVENT_MIND_CONTROL);
                                    events.CancelEvent(EVENT_FEAR);
                                    events.CancelEvent(EVENT_SHADOW_BOLT);
                                    me->SetVisible(false);
                                    return;
                                }
                            }
                            events.ScheduleEvent(EVENT_SPAWN_ADD, 4*IN_MILLISECONDS);
                            break;
                    }

                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;
                }
            }
        }

    private:
        uint32 SpawnedAdds;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<boss_victor_nefariusAI>(creature);
    }
};

class boss_nefarian : public CreatureScript
{
public:
    boss_nefarian() : CreatureScript("boss_nefarian") { }

    struct boss_nefarianAI : public BossAI
    {
        boss_nefarianAI(Creature* creature) : BossAI(creature, BOSS_NEFARIAN) { }

        void Reset()
        {
            Phase3 = false;
            canDespawn = false;
            DespawnTimer = 30*IN_MILLISECONDS;
        }

        void JustReachedHome()
        {
            canDespawn = true;
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_SHADOWFLAME, 12000);
            events.ScheduleEvent(EVENT_FEAR, urand(25000, 35000));
            events.ScheduleEvent(EVENT_VEILOFSHADOW, urand(25000, 35000));
            events.ScheduleEvent(EVENT_CLEAVE, 7000);
            //events.ScheduleEvent(EVENT_TAILLASH, 10000);
            events.ScheduleEvent(EVENT_CLASSCALL, urand(30000, 35000));
            Talk(SAY_RANDOM);
        }

        void JustDied(Unit* /*Killer*/)
        {
            _JustDied();
            Talk(SAY_DEATH);
        }

        void KilledUnit(Unit* victim)
        {
            if (rand32() % 5)
                return;

            Talk(SAY_SLAY, victim);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 1)
            {
                me->SetInCombatWithZone();
                if (me->GetVictim())
                    AttackStart(me->GetVictim());
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (canDespawn && DespawnTimer <= diff)
            {
                instance->SetBossState(BOSS_NEFARIAN, FAIL);

                std::list<Creature*> constructList;
                me->GetCreatureListWithEntryInGrid(constructList, BONE_CONSTRUCT, 500.0f);
                for (std::list<Creature*>::const_iterator itr = constructList.begin(); itr != constructList.end(); ++itr)
                    (*itr)->DespawnOrUnsummon();
            } else DespawnTimer -= diff;

            if (!UpdateVictim())
                return;

            if (canDespawn)
                canDespawn = false;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_SHADOWFLAME:
                        DoCastVictim(SPELL_SHADOWFLAME);
                        events.ScheduleEvent(EVENT_SHADOWFLAME, 12000);
                        break;
                    case EVENT_FEAR:
                        DoCastVictim(SPELL_BELLOWINGROAR);
                        events.ScheduleEvent(EVENT_FEAR, urand(25000, 35000));
                        break;
                    case EVENT_VEILOFSHADOW:
                        DoCastVictim(SPELL_VEILOFSHADOW);
                        events.ScheduleEvent(EVENT_VEILOFSHADOW, urand(25000, 35000));
                        break;
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, 7000);
                        break;
                    case EVENT_TAILLASH:
                        // Cast NYI since we need a better check for behind target
                        DoCastVictim(SPELL_TAILLASH);
                        events.ScheduleEvent(EVENT_TAILLASH, 10000);
                        break;
                    case EVENT_CLASSCALL:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            switch (target->getClass())
                        {
                            case CLASS_MAGE:
                                Talk(SAY_MAGE);
                                DoCast(me, SPELL_MAGE);
                                break;
                            case CLASS_WARRIOR:
                                Talk(SAY_WARRIOR);
                                DoCast(me, SPELL_WARRIOR);
                                break;
                            case CLASS_DRUID:
                                Talk(SAY_DRUID);
                                DoCast(target, SPELL_DRUID);
                                break;
                            case CLASS_PRIEST:
                                Talk(SAY_PRIEST);
                                DoCast(me, SPELL_PRIEST);
                                break;
                            case CLASS_PALADIN:
                                Talk(SAY_PALADIN);
                                DoCast(me, SPELL_PALADIN);
                                break;
                            case CLASS_SHAMAN:
                                Talk(SAY_SHAMAN);
                                DoCast(me, SPELL_SHAMAN);
                                break;
                            case CLASS_WARLOCK:
                                Talk(SAY_WARLOCK);
                                DoCast(me, SPELL_WARLOCK);
                                break;
                            case CLASS_HUNTER:
                                Talk(SAY_HUNTER);
                                DoCast(me, SPELL_HUNTER);
                                break;
                            case CLASS_ROGUE:
                                Talk(SAY_ROGUE);
                                DoCast(me, SPELL_ROGUE);
                                break;
                        }
                        events.ScheduleEvent(EVENT_CLASSCALL, urand(30000, 35000));
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            // Phase3 begins when health below 20 pct
            if (!Phase3 && HealthBelowPct(20))
            {
                std::list<Creature*> constructList;
                me->GetCreatureListWithEntryInGrid(constructList, BONE_CONSTRUCT, 500.0f);
                for (std::list<Creature*>::const_iterator itr = constructList.begin(); itr != constructList.end(); ++itr)
                    if ((*itr) && !(*itr)->IsAlive())
                    {
                        (*itr)->Respawn();
                        (*itr)->SetInCombatWithZone();
                        (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        (*itr)->SetReactState(REACT_AGGRESSIVE);
                        (*itr)->SetStandState(UNIT_STAND_STATE_STAND);
                    }

                Phase3 = true;
                Talk(SAY_RAISE_SKELETONS);
            }

            DoMeleeAttackIfReady();
        }

    private:
        bool canDespawn;
        uint32 DespawnTimer;
        bool Phase3;

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<boss_nefarianAI>(creature);
    }
};

void AddSC_boss_nefarian()
{
    new boss_victor_nefarius();
    new boss_nefarian();
}
