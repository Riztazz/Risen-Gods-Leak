/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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
Name: cast_commandscript
%Complete: 100
Comment: All cast related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Creature.h"
#include "Language.h"
#include "Player.h"

namespace chat { namespace command { namespace handler {

        bool HandleCastCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Unit* target = handler->getSelectedUnit();
        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId)
            return false;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
        {
            handler->PSendSysMessage(LANG_COMMAND_NOSPELLFOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer()))
        {
            handler->PSendSysMessage(LANG_COMMAND_SPELL_BROKEN, spellId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* triggeredStr = strtok(NULL, " ");
        if (triggeredStr)
        {
            int l = strlen(triggeredStr);
            if (strncmp(triggeredStr, "triggered", l) != 0)
                return false;
        }

        bool triggered = (triggeredStr != NULL);

        handler->GetSession()->GetPlayer()->CastSpell(target, spellId, triggered);

        return true;
    }

        bool HandleCastBackCommand(ChatHandler* handler, char const* args)
    {
        Creature* caster = handler->getSelectedCreature();
        if (!caster)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r
        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId || !sSpellMgr->GetSpellInfo(spellId))
        {
            handler->PSendSysMessage(LANG_COMMAND_NOSPELLFOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* triggeredStr = strtok(NULL, " ");
        if (triggeredStr)
        {
            int l = strlen(triggeredStr);
            if (strncmp(triggeredStr, "triggered", l) != 0)
                return false;
        }

        bool triggered = (triggeredStr != NULL);

        caster->CastSpell(handler->GetSession()->GetPlayer(), spellId, triggered);

        return true;
    }

        bool HandleCastDistCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId)
            return false;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
        {
            handler->PSendSysMessage(LANG_COMMAND_NOSPELLFOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer()))
        {
            handler->PSendSysMessage(LANG_COMMAND_SPELL_BROKEN, spellId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* distStr = strtok(NULL, " ");

        float dist = 0;

        if (distStr)
            sscanf(distStr, "%f", &dist);

        char* triggeredStr = strtok(NULL, " ");
        if (triggeredStr)
        {
            int l = strlen(triggeredStr);
            if (strncmp(triggeredStr, "triggered", l) != 0)
                return false;
        }

        bool triggered = (triggeredStr != NULL);

        float x, y, z;
        handler->GetSession()->GetPlayer()->GetClosePoint(x, y, z, dist);

        handler->GetSession()->GetPlayer()->CastSpell(x, y, z, spellId, triggered);

        return true;
    }

        bool HandleCastSelfCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Unit* target = handler->getSelectedUnit();
        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId)
            return false;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            return false;

        if (!SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer()))
        {
            handler->PSendSysMessage(LANG_COMMAND_SPELL_BROKEN, spellId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        target->CastSpell(target, spellId, false);

        return true;
    }

        bool HandleCastTargetCommad(ChatHandler* handler, char const* args)
    {
        Creature* caster = handler->getSelectedCreature();
        if (!caster)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!caster->GetVictim())
        {
            handler->SendSysMessage(LANG_SELECTED_TARGET_NOT_HAVE_VICTIM);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId || !sSpellMgr->GetSpellInfo(spellId))
        {
            handler->PSendSysMessage(LANG_COMMAND_NOSPELLFOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* triggeredStr = strtok(NULL, " ");
        if (triggeredStr)
        {
            int l = strlen(triggeredStr);
            if (strncmp(triggeredStr, "triggered", l) != 0)
                return false;
        }

        bool triggered = (triggeredStr != NULL);

        caster->CastSpell(caster->GetVictim(), spellId, triggered);

        return true;
    }

        bool HandleCastDestCommand(ChatHandler* handler, char const* args)
    {
        Unit* caster = handler->getSelectedUnit();
        if (!caster)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId || !sSpellMgr->GetSpellInfo(spellId))
        {
            handler->PSendSysMessage(LANG_COMMAND_NOSPELLFOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* posX = strtok(NULL, " ");
        char* posY = strtok(NULL, " ");
        char* posZ = strtok(NULL, " ");

        if (!posX || !posY || !posZ)
            return false;

        float x = float(atof(posX));
        float y = float(atof(posY));
        float z = float(atof(posZ));

        char* triggeredStr = strtok(NULL, " ");
        if (triggeredStr)
        {
            int l = strlen(triggeredStr);
            if (strncmp(triggeredStr, "triggered", l) != 0)
                return false;
        }

        bool triggered = (triggeredStr != NULL);

        caster->CastSpell(x, y, z, spellId, triggered);

        return true;
    }

}}}

void AddSC_cast_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_cast",         HandleCastCommand,      false);
    new LegacyCommandScript("cmd_cast_back",    HandleCastBackCommand,  false);
    new LegacyCommandScript("cmd_cast_dist",    HandleCastDistCommand,  false);
    new LegacyCommandScript("cmd_cast_self",    HandleCastSelfCommand,  false);
    new LegacyCommandScript("cmd_cast_target",  HandleCastTargetCommad, false);
    new LegacyCommandScript("cmd_cast_dest",    HandleCastDestCommand,  false);
}
