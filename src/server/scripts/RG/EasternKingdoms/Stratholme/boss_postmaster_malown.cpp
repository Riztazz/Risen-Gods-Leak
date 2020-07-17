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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "stratholme.h"

enum Says
{
    SAY_KILL                    = 0,
    SAY_AGGRO                   = 1
};

enum Spells
{
    SPELL_WAILINGDEAD           = 7713,
    SPELL_BACKHAND              = 6253,
    SPELL_CURSEOFWEAKNESS       = 8552,
    SPELL_CURSEOFTONGUES        = 12889,
    SPELL_CALLOFTHEGRAVE        = 17831
};

enum Events
{
    EVENT_WAILINGDEAD           = 1,
    EVENT_BACKHAND              = 2,
    EVENT_CURSEOFWEAKNESS       = 3,
    EVENT_CURSEOFTONGUES        = 4,
    EVENT_CALLOFTHEGRAVE        = 5
};

class boss_postmaster_malown : public CreatureScript
{
    public: boss_postmaster_malown() : CreatureScript("boss_postmaster_malown") { }

        struct boss_postmaster_malownAI : public BossAI
        {
            boss_postmaster_malownAI(Creature* creature) : BossAI(creature, TYPE_MALOWN) { }

            void EnterCombat(Unit* /*who*/) override
            {
                Talk(SAY_AGGRO);
                events.ScheduleEvent(EVENT_WAILINGDEAD,     19 * IN_MILLISECONDS); // lasts 6 sec
                events.ScheduleEvent(EVENT_BACKHAND,         8 * IN_MILLISECONDS); // 2 sec stun
                events.ScheduleEvent(EVENT_CURSEOFWEAKNESS, 20 * IN_MILLISECONDS); // lasts 2 mins
                events.ScheduleEvent(EVENT_CURSEOFTONGUES,  22 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CALLOFTHEGRAVE,  25 * IN_MILLISECONDS);
            }

            void KilledUnit(Unit* /*victim*/) override
            {
                Talk(SAY_KILL);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_WAILINGDEAD:
                            if (rand32() % 100 < 65) //65% chance to cast
                                DoCastVictim(SPELL_WAILINGDEAD, true);
                            events.ScheduleEvent(EVENT_WAILINGDEAD, 19 * IN_MILLISECONDS);
                            break;
                        case EVENT_BACKHAND:
                            if (rand32() % 100 < 45) //45% chance to cast
                                DoCastVictim(SPELL_BACKHAND, true);
                            events.ScheduleEvent(EVENT_WAILINGDEAD, 8 * IN_MILLISECONDS);
                            break;
                        case EVENT_CURSEOFWEAKNESS:
                            if (rand32() % 100 < 3) //3% chance to cast
                                DoCastVictim(SPELL_CURSEOFWEAKNESS, true);
                            events.ScheduleEvent(EVENT_WAILINGDEAD, 20 * IN_MILLISECONDS);
                            break;
                        case EVENT_CURSEOFTONGUES:
                            if (rand32() % 100 < 3) //3% chance to cast
                                DoCastVictim(SPELL_CURSEOFTONGUES, true);
                            events.ScheduleEvent(EVENT_WAILINGDEAD, 22 * IN_MILLISECONDS);
                            break;
                        case EVENT_CALLOFTHEGRAVE:
                            if (rand32() % 100 < 5) //5% chance to cast
                                DoCastVictim(SPELL_CALLOFTHEGRAVE, true);
                            events.ScheduleEvent(EVENT_WAILINGDEAD, 25 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
                
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_postmaster_malownAI(creature);
        }
};

void AddSC_boss_postmaster_malown()
{
    new boss_postmaster_malown();
}
