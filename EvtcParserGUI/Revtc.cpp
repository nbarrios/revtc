#include "Revtc.h"
#include <algorithm>

namespace Revtc {

	inline bool operator<(const Player& lhs, const Player& rhs) {
		return lhs.subgroup < rhs.subgroup;
	}

	Parser::Parser(const unsigned char * buf, size_t len)
		: buf(buf)
		, buf_len(len)
		, boss_addr(0)
		, last_event_time(UINT64_MAX)
	{
	}

	Parser::~Parser()
	{
	}

	Log Parser::parse()
	{
		Log log;
		log.area_id = (BossID) 0;
		uint32_t index = 0;

		/* Header */
		// EVTC + Version - 12 bytes
		std::string evtc = std::string((char*)&buf[0], 4);
		if (evtc != "EVTC") {
			log.error = "Corrupted or otherwise invalid EVTC file.";
			return log;
		}
		log.version = std::string((char *)&buf[4], 8);
		log.area_id = (BossID) *(uint16_t*)&buf[13];
		log.encounter_name = encounterName(log.area_id);
		log.reward_at = 0.f;

		//Agent
		uint32_t agent_count = *(uint32_t*)&buf[16];
		index = 16 + sizeof(uint32_t);
		for (unsigned int i = 0; i < agent_count; ++i) {
			Agent agent{};
			agent.last_aware = UINT64_MAX;
			agent.addr = *(uint64_t*)&buf[index]; index += sizeof(uint64_t);
			uint16_t lhf = *(uint16_t*)&buf[index];
			uint16_t uhf = *(uint16_t*)&buf[index + sizeof(uint16_t)];
			agent.prof = *(uint32_t*)&buf[index]; index += sizeof(uint32_t);
			agent.is_elite = *(uint32_t*)&buf[index]; index += sizeof(uint32_t);
			agent.toughness = *(int16_t*)&buf[index]; index += sizeof(int16_t);
			agent.concentration = *(int16_t*)&buf[index]; index += sizeof(int16_t);
			agent.healing = *(int16_t*)&buf[index]; index += sizeof(int16_t);
			agent.pad1 = *(int16_t*)&buf[index]; index += sizeof(int16_t);
			agent.condition = *(int16_t*)&buf[index]; index += sizeof(int16_t);
			agent.pad2 = *(int16_t*)&buf[index]; index += sizeof(int16_t);
			char *name_buf = (char *)&buf[index];
			agent.name = std::string(name_buf); index += 64;
			index += 4; //align padding

			//Check for player and extract info
			if (agent.is_elite != 0xFFFFFFFF) {
				agent.is_player = true;

				Player player{};
				player.addr = agent.addr;
				//Character Name
				size_t len = strlen(name_buf);
				player.name = std::string(name_buf, len);
				name_buf += len + 2; //Skip two since we don't want the colon
				//Account Name
				len = strlen(name_buf);
				player.account = std::string(name_buf, len);
				name_buf += len + 1;
				//Subgroup
				std::string sub(name_buf, 1);
				player.subgroup = std::stoi(sub);
				//Profession
				player.profession = agent.prof;
				const auto& prof = professionName(player.profession);
				player.profession_name = prof.first;
				player.profession_name_short = prof.second;
				player.elite_spec = agent.is_elite;
				const auto& elite = eliteSpecName(player.elite_spec);
				player.elite_spec_name = elite.first;
				player.elite_spec_name_short = elite.second;
				player.might_last_event = UINT64_MAX;

				players.emplace(player.addr, player);
			}
			else {
				if (uhf == 0xFFFF) {
					agent.is_gadget = true;
				}
				else {
					agent.is_character = true;
				}
				agent.id = lhf;
				if (agent.id == (uint16_t)log.area_id) {
					boss_addr = agent.addr;
				}
			}

			agents.emplace(agent.addr, agent);
		}

		//Skills
		uint32_t skill_count = *(uint32_t*)&buf[index]; index += sizeof(uint32_t);
		for (unsigned int i = 0; i < skill_count; ++i) {
			Skill skill;

			skill.id = *(int32_t*)&buf[index]; index += sizeof(int32_t);
			skill.name = std::string((char *)&buf[index]); index += 64;

			skills.emplace(skill.id, skill);
		}

		//Events
		//First iteration - create CombatEvents
		uint64_t first_event_time = 0;
		while (index < buf_len) {
			CombatEvent event = *(CombatEvent*)&buf[index];
			index += sizeof(CombatEvent);

			if (event.is_statechange == CBTS_LOGSTART) {
				first_event_time = event.time;
			}
			else if (event.is_statechange == CBTS_LOGEND) {
				last_event_time = event.time - first_event_time;
			}
			event.time -= first_event_time;

			//Assign times and instance ids
			if (agents.count(event.src_agent)) {
				Agent& agent = agents.at(event.src_agent);
				if (agent.first_aware == 0) {
					agent.first_aware = event.time;
				}
				agent.last_aware = event.time;
				if (!event.is_statechange) {
					agent.instance_id = event.src_instid;
					agent_addrs.emplace(agent.instance_id, agent.addr);
				}
			}

			events.push_back(event);
		}
		
		//Copy times to players
		for (auto& player : players) {
			const Agent& agent = agents.at(player.second.addr);
			player.second.first_aware = agent.first_aware;
			player.second.last_aware = agent.last_aware;
		}

		// Second iteration
		//Map master instance ids to agents by addr
		for (const auto& event : events) {
			if (event.src_master_instid != 0) {
				if (agent_addrs.count(event.src_master_instid) && agents.count(event.src_agent)) {
					Agent& slave = agents.at(event.src_agent);
					uint64_t master_addr = agent_addrs.at(event.src_master_instid);
					if (agents.count(master_addr)) {
						Agent& master = agents.at(master_addr);
						uint64_t time_rel = event.time;
						if (time_rel > master.first_aware && time_rel < master.last_aware) {
							slave.master_addr = master.addr;
						}
					}
				}
			}
		}

		// Third iteration - could parallelize this, seems plenty fast anyways
		//Extract data
		for (const auto& event : events) {
			Agent *src = nullptr;
			Agent *dst = nullptr;
			Agent *master = nullptr;
			if (agents.count(event.src_agent)) {
				src = &agents.at(event.src_agent);
			}
			if (agents.count(event.dst_agent)) {
				dst = &agents.at(event.dst_agent);
				dst->hits++;
			}
			if (src && src->master_addr) {
				//Just attribute all slave damage to the master for now
				src = &agents.at(src->master_addr);
			}

			if (event.is_statechange) {
				if (event.is_statechange == CBTS_REWARD) {
					log.reward_at = event.time / 1000.f;
				}
			}
			else if (event.is_activation) {

			}
			else if (event.is_buffremove) {
				if (src && src->is_player) {
					Player& player = players.at(src->addr);
					if (event.skillid == 0x2E4) { //Might
						if (event.is_buffremove == CBTB_MANUAL) {
							player.might_current--;
							player.might_current = std::max(0u, player.might_current);
							player.might_applied += player.might_current;
							player.might_events++;
						}
						player.might_last_event = event.time;
					}
				}
			}
			else {
				if (event.buff) { //Buff
					if (event.buff_dmg) {
						if (src && src->is_player) {
							Player& player = players.at(src->addr);
							player.condi_damage += event.buff_dmg;
							if (dst && dst->id == (uint16_t)log.area_id) {
								player.boss_condi_damage += event.buff_dmg;
							}
							//Special Handling for Deimos (who turns into a gadget at 10%)
							if (dst && log.area_id == BossID::DEIMOS && dst->id == 0x2113) {
								player.boss_condi_damage += event.buff_dmg;
							}
						}
					}
					else if (event.value) { //Buff Application
						if (dst && dst->is_player) {
							Player& player = players.at(dst->addr);
							if (event.skillid == 0x2E4) { //Might
								if (event.overstack_value > 0) {
									player.might_current = 25;
								}
								else {
									player.might_current++;
									player.might_current = std::min(25u, player.might_current);
								}

								player.might_applied += player.might_current;
								player.might_events++;
								player.might_last_event = event.time;
							}
						}
					}
				}
				else { //Physical
					if (src && src->is_player) {
						Player& player = players.at(src->addr);
						player.physical_damage += event.value;
						if (dst) {
							if (dst->id == (uint16_t)log.area_id) {
								player.boss_physical_damage += event.value;
							}

							//Special Handling for Deimos (who turns into a gadget at 10%)
							if (log.area_id == BossID::DEIMOS && dst->id == DEIMOS_STRUCTURE) {
								player.boss_physical_damage += event.value;
							}
							//Low-tech orb pusher detect on Keep Construct
							else if (log.area_id == BossID::KEEP_CONSTRUCT && dst->id == KC_CONSTRUCT_CORE) {
								player.note_counter++;
							}
						}
					}
					else if (src && dst && dst->is_player) {
						if (log.area_id == BossID::DEIMOS && src->id == DEIMOS_HANDS) {
							Player& player = players.at(dst->addr);
							player.note_counter++;
						}
					}
				}
			}
		}

		const Agent& boss = agents.at(boss_addr);
		log.boss_lifetime = (float)(boss.last_aware - boss.first_aware) / 1000.f;

		std::string tracked_player_name;
		uint32_t tracked_count = 0;

		for (auto& player_pair : players) {
			auto& player = player_pair.second;
			player.dps = roundf((float)(player.physical_damage + player.condi_damage) / (log.reward_at ? log.reward_at : log.boss_lifetime));
			player.boss_dps = roundf((float)(player.boss_physical_damage + player.boss_condi_damage) / (log.reward_at ? log.reward_at : log.boss_lifetime));
			player.might_avg = (float)player.might_applied / (float)player.might_events;
			player.might_avg *= (float)(player.might_last_event / 1000.f) / (float)(log.reward_at ? log.reward_at : log.boss_lifetime);

			//Notes
			if (log.area_id == BossID::KEEP_CONSTRUCT || log.area_id == BossID::DEIMOS) {
				if (player.note_counter > tracked_count) {
					tracked_player_name = player.name;
					tracked_count = player.note_counter;
				}
			}
		}
		for (auto& player_pair : players) {
			auto& player = player_pair.second;

			//Notes
			if (log.area_id == BossID::KEEP_CONSTRUCT) {
				if (player.name == tracked_player_name) {
					player.note = "Orb Pusher";
				}
			}
			else if (log.area_id == BossID::DEIMOS) {
				if (player.name == tracked_player_name) {
					player.note = "Hand Kiter";
				}
			}

			log.players.push_back(player);
		}
		std::sort(log.players.begin(), log.players.end(), std::less<Player>());
		return log;
	}

	std::string Parser::encounterName(BossID area_id)
	{
		//TODO: Check if this would be faster as a hash table (likely not)
		switch (area_id)
		{
		case BossID::VALE_GUARDIAN:
			return "Vale Guardian";
		case BossID::GORSEVAL:
			return "Gorseval";
		case BossID::SABETHA:
			return "Sabetha";
		case BossID::SLOTHASOR:
			return "Slothasor";
		case BossID::MATTHIAS:
			return "Matthias Gabriel";
		case BossID::KEEP_CONSTRUCT:
			return "Keep Construct";
		case BossID::XERA:
			return "Xera";
		case BossID::CAIRN:
			return "Cairn";
		case BossID::MO:
			return "Mursaat Overseer";
		case BossID::SAMAROG:
			return "Samarog";
		case BossID::DEIMOS:
			return "Deimos";
		case BossID::SOULLESS_HORROR:
			return "Soulless Horror";
		case BossID::DHUUM:
			return "Dhuum";
		case BossID::MAMA:
			return "M.A.M.A";
		case BossID::SIAX:
			return "Siax";
		case BossID::ENSOLYSS:
			return "Ensolyss";
		case BossID::SKORVALD:
			return "Skorvald";
		case BossID::ARTSARIIV:
			return "Artsariiv";
		case BossID::ARKK:
			return "Arkk";
		case BossID::STANDARD_GOLEM:
			return "Standard Kitty Golem";
		case BossID::MEDIUM_GOLEM:
			return "Medium Kitty Golem";
		case BossID::LARGE_GOLEM:
			return "Large Kitty Golem";
		case BossID::MASSIVE_GOLEM:
			return "Massive Kitty Golem";
		case BossID::AVERAGE_GOLEM:
			return "Average Kitty Golem";
		case BossID::VITAL_GOLEM:
			return "Vital Kitty Golem";
		default:
			return "Unknown";
			break;
		}
	}

	std::pair<std::string, std::string> Parser::professionName(uint32_t prof)
	{
		switch (prof)
		{
		case 1:
			return std::make_pair("Guardian", "Grdn");
		case 2:
			return std::make_pair("Warrior", "Warr");
		case 3:
			return std::make_pair("Engineer", "Engi");
		case 4:
			return std::make_pair("Ranger", "Rngr");
		case 5:
			return std::make_pair("Thief", "Thf");
		case 6:
			return std::make_pair("Elementalist", "Ele");
		case 7:
			return std::make_pair("Mesmer", "Mes");
		case 8:
			return std::make_pair("Necromancer", "Necr");
		case 9:
			return std::make_pair("Revenant", "Rev");
		default:
			return std::make_pair("Unknown", "Unk");
			break;
		}
	}

	std::pair<std::string, std::string> Parser::eliteSpecName(uint32_t elite)
	{
		switch (elite)
		{
		case 5:
			return std::make_pair("Druid", "Dru");
		case 7:
			return std::make_pair("Daredevil", "DD");
		case 18:
			return std::make_pair("Berserker", "Brsk");
		case 27:
			return std::make_pair("Dragonhunter", "DH");
		case 34:
			return std::make_pair("Reaper", "Rpr");
		case 40:
			return std::make_pair("Chronomancer", "Chrn");
		case 43:
			return std::make_pair("Scrapper", "Scrp");
		case 48:
			return std::make_pair("Tempest", "Temp");
		case 52:
			return std::make_pair("Herald", "Hrld");
		case 55:
			return std::make_pair("Soulbeast", "Slb");
		case 56:
			return std::make_pair("Weaver", "Weav");
		case 57:
			return std::make_pair("Holosmith", "Holo");
		case 58:
			return std::make_pair("Deadeye", "Deye");
		case 59:
			return std::make_pair("Mirage", "Mir");
		case 60:
			return std::make_pair("Scourge", "Scrg");
		case 61:
			return std::make_pair("Spellbreaker", "Spbr");
		case 62:
			return std::make_pair("Firebrand", "Fbrd");
		case 63:
			return std::make_pair("Renegade", "Ren");
		default:
			return std::make_pair("Unknown", "Unk");
			break;
		}
	}

}