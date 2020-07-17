/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

/* ScriptData
SDName: Boss_Ebonroc
SD%Complete: 50
SDComment: Shadow of Ebonroc needs core support
SDCategory: Blackwing Lair
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackwing_lair.h"
#include "SpellInfo.h"

enum Spells
{
    SPELL_SHADOWFLAME           = 22539,
    SPELL_WINGBUFFET            = 23339,
    SPELL_SHADOWOFEBONROC       = 23340
};

enum Events
{
    EVENT_SHADOWFLAME           = 1,
    EVENT_WINGBUFFET            = 2,
    EVENT_SHADOWOFEBONROC       = 3
};

class boss_ebonroc : public CreatureScript
{
    public:
        boss_ebonroc() : CreatureScript("boss_ebonroc") { }
    
        struct boss_ebonrocAI : public BossAI
        {
            boss_ebonrocAI(Creature* creature) : BossAI(creature, BOSS_EBONROC) { }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
    
                events.ScheduleEvent(EVENT_SHADOWFLAME,    urand(10, 20) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WINGBUFFET,                30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SHADOWOFEBONROC, urand(8, 10) * IN_MILLISECONDS);
            }


            void SpellHitTarget(Unit* target, SpellInfo const* spellInfo)
            {
                if (spellInfo->Id == SPELL_WINGBUFFET)
                    if (GetThreat(target))
                        ModifyThreatByPercent(target, -75);
            }
    
            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);
    
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
    
                if (!UpdateVictim())
                    return;
    
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_SHADOWFLAME:
                            DoCastVictim(SPELL_SHADOWFLAME);
                            events.ScheduleEvent(EVENT_SHADOWFLAME, urand(10, 20) * IN_MILLISECONDS);
                            break;
                        case EVENT_WINGBUFFET:
                            DoCastVictim(SPELL_WINGBUFFET);
                            events.ScheduleEvent(EVENT_WINGBUFFET, 30 * IN_MILLISECONDS);
                            break;
                        case EVENT_SHADOWOFEBONROC:
                            DoCastVictim(SPELL_SHADOWOFEBONROC);
                            events.ScheduleEvent(EVENT_SHADOWOFEBONROC, urand(8, 10) * IN_MILLISECONDS);
                            break;
                    }
                }
    
                DoMeleeAttackIfReady();
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_ebonrocAI>(creature);
        }
};

void AddSC_boss_ebonroc()
{
    new boss_ebonroc();
}
