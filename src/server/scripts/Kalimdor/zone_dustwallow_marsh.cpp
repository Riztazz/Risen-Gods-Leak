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
SDName: Dustwallow_Marsh
SD%Complete: 95
SDComment: Quest support: 11180, 558, 11126, 11142, 11174, Vendor Nat Pagle
SDCategory: Dustwallow Marsh
EndScriptData */

/* ContentData
npc_risen_husk_spirit
npc_nat_pagle
npc_private_hendel
npc_cassa_crimsonwing - handled by npc_taxi
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "Player.h"
#include "WorldSession.h"

/*######
## npc_risen_husk_spirit
######*/

enum HauntingWitchHill
{
    // Quest
    QUEST_WHATS_HAUNTING_WITCH_HILL     = 11180,

    // General spells
    SPELL_SUMMON_RESTLESS_APPARITION    = 42511,
    SPELL_WITCH_HILL_INFORMATION_CREDIT = 42512,

    // Risen Husk specific
    SPELL_CONSUME_FLESH                 = 37933,
    NPC_RISEN_HUSK                      = 23555,

    // Risen Spirit specific
    SPELL_INTANGIBLE_PRESENCE           = 43127,
    NPC_RISEN_SPIRIT                    = 23554,

    // Events
    EVENT_CONSUME_FLESH                 = 1,
    EVENT_INTANGIBLE_PRESENCE           = 2,
};

class npc_risen_husk_spirit : public CreatureScript
{
    public:
        npc_risen_husk_spirit() : CreatureScript("npc_risen_husk_spirit") { }

        struct npc_risen_husk_spiritAI : public ScriptedAI
        {
            npc_risen_husk_spiritAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                events.Reset();
                if (me->GetEntry() == NPC_RISEN_HUSK)
                    events.ScheduleEvent(EVENT_CONSUME_FLESH, 5000);
                else if (me->GetEntry() == NPC_RISEN_SPIRIT)
                    events.ScheduleEvent(EVENT_INTANGIBLE_PRESENCE, 5000);
            }

            void JustDied(Unit* killer) override
            {
                if (killer->GetTypeId() == TYPEID_PLAYER)
                {
                    if (killer->ToPlayer()->GetQuestStatus(QUEST_WHATS_HAUNTING_WITCH_HILL) == QUEST_STATUS_INCOMPLETE)
                    {
                        DoCastSelf(SPELL_SUMMON_RESTLESS_APPARITION, true);
                        DoCast(killer, SPELL_WITCH_HILL_INFORMATION_CREDIT, true);
                    }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CONSUME_FLESH:
                            DoCastVictim(SPELL_CONSUME_FLESH);
                            events.ScheduleEvent(EVENT_CONSUME_FLESH, 15000);
                            break;
                        case EVENT_INTANGIBLE_PRESENCE:
                            DoCastVictim(SPELL_INTANGIBLE_PRESENCE);
                            events.ScheduleEvent(EVENT_INTANGIBLE_PRESENCE, 15000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_risen_husk_spiritAI(creature);
        }
};

/*######
## npc_nat_pagle
######*/

enum NatPagle
{
    QUEST_NATS_MEASURING_TAPE = 8227
};

class npc_nat_pagle : public CreatureScript
{
public:
    npc_nat_pagle() : CreatureScript("npc_nat_pagle") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (creature->IsVendor() && player->GetQuestRewardStatus(QUEST_NATS_MEASURING_TAPE))
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, player->GetOptionTextWithEntry(GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_TEXT_BROWSE_GOODS_ID), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
            player->SEND_GOSSIP_MENU(7640, creature->GetGUID());
        }
        else
            player->SEND_GOSSIP_MENU(7638, creature->GetGUID());

        return true;
    }

};

/*######
## npc_private_hendel
######*/

enum Hendel
{
    SAY_PROGRESS_1_TER          = 0,
    SAY_PROGRESS_2_HEN          = 1,
    SAY_PROGRESS_3_TER          = 2,
    SAY_PROGRESS_4_TER          = 3,
    EMOTE_SURRENDER             = 4,

    QUEST_MISSING_DIPLO_PT16    = 1324,
    FACTION_ENEMY               = 168,                      //guessed, may be different

    NPC_SENTRY                  = 5184,                     //helps hendel
    NPC_JAINA                   = 4968,                     //appears once hendel gives up
    NPC_TERVOSH                 = 4967
};

/// @todo develop this further, end event not created
class npc_private_hendel : public CreatureScript
{
public:
    npc_private_hendel() : CreatureScript("npc_private_hendel") { }

    bool OnQuestAccept(Player* /*player*/, Creature* creature, const Quest* quest) override
    {
        if (quest->GetQuestId() == QUEST_MISSING_DIPLO_PT16)
            creature->setFaction(FACTION_ENEMY);

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_private_hendelAI(creature);
    }

    struct npc_private_hendelAI : public ScriptedAI
    {
        npc_private_hendelAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->RestoreFaction();
        }

        void AttackedBy(Unit* pAttacker) override
        {
            if (me->GetVictim())
                return;

            if (me->IsFriendlyTo(pAttacker))
                return;

            AttackStart(pAttacker);
        }

        void DamageTaken(Unit* pDoneBy, uint32 &Damage) override
        {
            if (Damage > me->GetHealth() || me->HealthBelowPctDamaged(20, Damage))
            {
                Damage = 0;

                if (Player* player = pDoneBy->GetCharmerOrOwnerPlayerOrPlayerItself())
                    player->GroupEventHappens(QUEST_MISSING_DIPLO_PT16, me);

                Talk(EMOTE_SURRENDER);
                EnterEvadeMode();
            }
        }
    };

};

/*######
## npc_zelfrax
######*/

Position const MovePosition = {-2967.030f, -3872.1799f, 35.620f, 0.0f};

enum Zelfrax
{
    SAY_ZELFRAX1     = 0,
    SAY_ZELFRAX2     = 1
};

class npc_zelfrax : public CreatureScript
{
public:
    npc_zelfrax() : CreatureScript("npc_zelfrax") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_zelfraxAI(creature);
    }

    struct npc_zelfraxAI : public ScriptedAI
    {
        npc_zelfraxAI(Creature* creature) : ScriptedAI(creature)
        {
            MoveToDock();
        }

        void AttackStart(Unit* who) override
        {
            if (!who)
                return;

            if (me->Attack(who, true))
            {
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);

                if (IsCombatMovementAllowed())
                    me->GetMotionMaster()->MoveChase(who);
            }
        }

        void MovementInform(uint32 Type, uint32 /*Id*/) override
        {
            if (Type != POINT_MOTION_TYPE)
                return;

            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            me->SetImmuneToPC(false);
            SetCombatMovement(true);

            if (me->IsInCombat())
                if (Unit* unit = me->GetVictim())
                    me->GetMotionMaster()->MoveChase(unit);
        }

        void MoveToDock()
        {
            SetCombatMovement(false);
            me->GetMotionMaster()->MovePoint(0, MovePosition);
            Talk(SAY_ZELFRAX1);
            Talk(SAY_ZELFRAX2);
        }

        void UpdateAI(uint32 /*Diff*/) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

};


enum SpellScripts
{
    SPELL_OOZE_ZAP              = 42489,
    SPELL_OOZE_ZAP_CHANNEL_END  = 42485,
    SPELL_OOZE_CHANNEL_CREDIT   = 42486,
    SPELL_ENERGIZED             = 42492,
};

class spell_ooze_zap : public SpellScriptLoader
{
    public:
        spell_ooze_zap() : SpellScriptLoader("spell_ooze_zap") { }

        class spell_ooze_zap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ooze_zap_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_OOZE_ZAP))
                    return false;
                return true;
            }

            SpellCastResult CheckRequirement()
            {
                if (!GetCaster()->HasAura(GetSpellInfo()->Effects[EFFECT_1].CalcValue()))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW; // This is actually correct

                if (!GetExplTargetUnit())
                    return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (GetHitUnit())
                    GetCaster()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_ooze_zap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_ooze_zap_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_ooze_zap_SpellScript();
        }
};

class spell_ooze_zap_channel_end : public SpellScriptLoader
{
    public:
        spell_ooze_zap_channel_end() : SpellScriptLoader("spell_ooze_zap_channel_end") { }

        class spell_ooze_zap_channel_end_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ooze_zap_channel_end_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_OOZE_ZAP_CHANNEL_END))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Player* player = GetCaster()->ToPlayer())
                    GetHitUnit()->CastSpell(player, SPELL_OOZE_CHANNEL_CREDIT, true);
                GetHitUnit()->Kill(GetHitUnit());                
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_ooze_zap_channel_end_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_ooze_zap_channel_end_SpellScript();
        }
};

class spell_energize_aoe : public SpellScriptLoader
{
    public:
        spell_energize_aoe() : SpellScriptLoader("spell_energize_aoe") { }

        class spell_energize_aoe_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_energize_aoe_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_ENERGIZED))
                    return false;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end();)
                {
                    if ((*itr)->GetTypeId() == TYPEID_PLAYER && (*itr)->ToPlayer()->GetQuestStatus(GetSpellInfo()->Effects[EFFECT_1].CalcValue()) == QUEST_STATUS_INCOMPLETE)
                        ++itr;
                    else
                        targets.erase(itr++);
                }
                targets.push_back(GetCaster());
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetCaster()->CastSpell(GetCaster(), uint32(GetEffectValue()), true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_energize_aoe_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_energize_aoe_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_energize_aoe_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_energize_aoe_SpellScript();
        }
};

void AddSC_dustwallow_marsh()
{
    new npc_risen_husk_spirit();
    new npc_nat_pagle();
    new npc_private_hendel();
    new npc_zelfrax();
    new spell_ooze_zap();
    new spell_ooze_zap_channel_end();
    new spell_energize_aoe();
}
