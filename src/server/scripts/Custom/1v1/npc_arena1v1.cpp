#include "ScriptMgr.h"
#include "RatedMgr.h"
#include "Common.h"
#include "DisableMgr.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "RatedInfo.h"
#include "Language.h"
#include "npc_arena1v1.h"


class npc_1v1arena : public CreatureScript
{
public:
	npc_1v1arena() : CreatureScript("npc_1v1arena")
	{
	}


	bool JoinQueueArena(Player* player, Creature* me, bool isRated)
	{
		if (!player || !me)
			return false;

		if (sWorld->getIntConfig(CONFIG_ARENA_1V1_MIN_LEVEL) > player->getLevel())
			return false;

		ObjectGuid guid = player->GetGUID();
		uint8 arenaslot = RatedInfo::GetRatedSlotByType(RATED_TYPE_5v5);
		RatedType ratedType = RATED_TYPE_5v5;
		uint32 arenaRating = 0;
		uint32 matchmakerRating = 0;

		// ignore if we already in BG or BG queue
		if (player->InBattleground())
			return false;

		//check existance
		Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
		if (!bg)
		{
			TC_LOG_ERROR("Arena", "Battleground: template bg (all arenas) not found");
			return false;
		}

		if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, BATTLEGROUND_AA, NULL))
		{
			ChatHandler(player->GetSession()).PSendSysMessage(LANG_ARENA_DISABLED);
			return false;
		}

		BattlegroundTypeId bgTypeId = bg->GetTypeID();
		BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, ratedType);
		PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), player->getLevel());
		if (!bracketEntry)
			return false;

		GroupJoinBattlegroundResult err = ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS;

		// check if already in queue
		if (player->GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
			//player is already in this queue
			return false;
		// check if has free queue slots
		if (!player->HasFreeBattlegroundQueueId())
			return false;

		uint32 ateamId = 0;

		if (isRated)
		{
			ateamId = player->GetArenaTeamId(arenaslot);
			RatedInfo* at = sRatedMgr->GetRatedInfo(ateamId);

			// get the team rating for queueing
			arenaRating = at->GetMatchMakerRating();
			matchmakerRating = arenaRating;
			// the arenateam id must match for everyone in the group

			if (arenaRating <= 0)
				arenaRating = 1;
		}

		BattlegroundQueue &bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
		bg->SetRated(isRated);

		GroupQueueInfo* ginfo = bgQueue.AddGroup(player, NULL, bgTypeId, bracketEntry, ratedType, isRated, false, arenaRating, matchmakerRating);
		uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
		uint32 queueSlot = player->AddBattlegroundQueueId(bgQueueTypeId);
		

		WorldPacket data;
		// send status packet (in queue)
		sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, player, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0, ratedType);
		player->GetSession()->SendPacket(&data);

		sBattlegroundMgr->ScheduleQueueUpdate(matchmakerRating, ratedType, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());

		return true;
	}


	bool OnGossipHello(Player* player, Creature* me)
	{
		if (!player || !me)
			return true;

		if (sWorld->getBoolConfig(CONFIG_ARENA_1V1_ENABLE) == false)
		{
			ChatHandler(player->GetSession()).SendSysMessage("1v1 disabled!");
			return true;
		}

		if (player->InBattlegroundQueueForBattlegroundQueueType(BATTLEGROUND_QUEUE_5v5))
			player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Leave queue 1v1 Arena", GOSSIP_SENDER_MAIN, 3, "Are you sure?", 0, false);
		else
		{
			if (player->InBattlegroundQueueForBattlegroundQueueType(BATTLEGROUND_QUEUE_5v5) == false)
			{
				player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Sign up 1v1 Arena (rated)", GOSSIP_SENDER_MAIN, 2);
			}
		}
		player->SEND_GOSSIP_MENU(68, me->GetGUID());
		return true;
	}



	bool OnGossipSelect(Player* player, Creature* me, uint32 /*uiSender*/, uint32 uiAction)
	{
		if (!player || !me)
			return true;

		player->PlayerTalkClass->ClearMenus();

		switch (uiAction)
		{

		case 2: // Join Queue Arena (rated)
		{
			JoinQueueArena(player, me, true);
				ChatHandler(player->GetSession()).SendSysMessage("Something went wrong while join queue.");

			player->CLOSE_GOSSIP_MENU();
			return true;
		}
		break;

		case 3: // Leave Queue
		{
			uint8 ratedType;
			WorldPacket Data;
			Data << (uint8)0x1 << (uint8)0x0 << (uint32)BATTLEGROUND_AA << (uint16)0x0 << (uint8)0x0;
			player->GetSession()->HandleBattleFieldPortOpcode(Data);
			player->CLOSE_GOSSIP_MENU();
			return true;
		}
		break;
		}
	}
};


void AddSC_npc_1v1arena()
{
	new npc_1v1arena();
}
