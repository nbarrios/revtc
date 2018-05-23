#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Revtc {

#define DEIMOS_STRUCTURE 0x2113
#define DEIMOS_HANDS 0x4345
#define KC_CONSTRUCT_CORE 0x3F85

	enum class BossID : uint16_t {
		VALE_GUARDIAN = 15438,
		GORSEVAL = 15429,
		SABETHA = 15375,
		SLOTHASOR = 16123,
		MATTHIAS = 16115,
		KEEP_CONSTRUCT = 16235,
		XERA = 16246,
		CAIRN = 17194,
		MO = 17172,
		SAMAROG = 17188,
		DEIMOS = 17154,
		SOULLESS_HORROR = 19767,
		DHUUM = 19450,
		MAMA = 17021,
		SIAX = 17028,
		ENSOLYSS = 16948,
		SKORVALD = 17632,
		ARTSARIIV = 17949,
		ARKK = 17759,
		STANDARD_GOLEM = 16199,
		MEDIUM_GOLEM = 19645,
		LARGE_GOLEM = 19676,
		MASSIVE_GOLEM = 16202,
		AVERAGE_GOLEM = 16177,
		VITAL_GOLEM = 16198
	};

	//From deltaconnected's EVTC README.txt
	/* combat state change */
	enum cbtstatechange {
		CBTS_NONE, // not used - not this kind of event
		CBTS_ENTERCOMBAT, // src_agent entered combat, dst_agent is subgroup
		CBTS_EXITCOMBAT, // src_agent left combat
		CBTS_CHANGEUP, // src_agent is now alive
		CBTS_CHANGEDEAD, // src_agent is now dead
		CBTS_CHANGEDOWN, // src_agent is now downed
		CBTS_SPAWN, // src_agent is now in game tracking range
		CBTS_DESPAWN, // src_agent is no longer being tracked
		CBTS_HEALTHUPDATE, // src_agent has reached a health marker. dst_agent = percent * 10000 (eg. 99.5% will be 9950)
		CBTS_LOGSTART, // log start. value = server unix timestamp **uint32**. buff_dmg = local unix timestamp. src_agent = 0x637261 (arcdps id)
		CBTS_LOGEND, // log end. value = server unix timestamp **uint32**. buff_dmg = local unix timestamp. src_agent = 0x637261 (arcdps id)
		CBTS_WEAPSWAP, // src_agent swapped weapon set. dst_agent = current set id (0/1 water, 4/5 land)
		CBTS_MAXHEALTHUPDATE, // src_agent has had it's maximum health changed. dst_agent = new max health
		CBTS_POINTOFVIEW, // src_agent will be agent of "recording" player
		CBTS_LANGUAGE, // src_agent will be text language
		CBTS_GWBUILD, // src_agent will be game build
		CBTS_SHARDID, // src_agent will be sever shard id
		CBTS_REWARD, // src_agent is self, dst_agent is reward id, value is reward type. these are the wiggly boxes that you get
		CBTS_BUFFINITIAL // combat event that will appear once per buff per agent on logging start (zero duration, buff==18)
	};

	/* combat buff remove type */
	enum cbtbuffremove {
		CBTB_NONE, // not used - not this kind of event
		CBTB_ALL, // all stacks removed
		CBTB_SINGLE, // single stack removed. disabled on server trigger, will happen for each stack on cleanse
		CBTB_MANUAL, // autoremoved by ooc or allstack (ignore for strip/cleanse calc, use for in/out volume)
	};

	struct CombatEvent {
		uint64_t time; /* timegettime() at time of event */
		uint64_t src_agent; /* unique identifier */
		uint64_t dst_agent; /* unique identifier */
		int32_t value; /* event-specific */
		int32_t buff_dmg; /* estimated buff damage. zero on application event */
		uint16_t overstack_value; /* estimated overwritten stack duration for buff application */
		uint16_t skillid; /* skill id */
		uint16_t src_instid; /* agent map instance id */
		uint16_t dst_instid; /* agent map instance id */
		uint16_t src_master_instid; /* master source agent map instance id if source is a minion/pet */
		uint8_t iss_offset; /* internal tracking. garbage */
		uint8_t iss_offset_target; /* internal tracking. garbage */
		uint8_t iss_bd_offset; /* internal tracking. garbage */
		uint8_t iss_bd_offset_target; /* internal tracking. garbage */
		uint8_t iss_alt_offset; /* internal tracking. garbage */
		uint8_t iss_alt_offset_target; /* internal tracking. garbage */
		uint8_t skar; /* internal tracking. garbage */
		uint8_t skar_alt; /* internal tracking. garbage */
		uint8_t skar_use_alt; /* internal tracking. garbage */
		uint8_t iff; /* from iff enum */
		uint8_t buff; /* buff application, removal, or damage event */
		uint8_t result; /* from cbtresult enum */
		uint8_t is_activation; /* from cbtactivation enum */
		uint8_t is_buffremove; /* buff removed. src=relevant, dst=caused it (for strips/cleanses). from cbtr enum */
		uint8_t is_ninety; /* source agent health was over 90% */
		uint8_t is_fifty; /* target agent health was under 50% */
		uint8_t is_moving; /* source agent was moving */
		uint8_t is_statechange; /* from cbtstatechange enum */
		uint8_t is_flanking; /* target agent was not facing source */
		uint8_t is_shields; /* all or part damage was vs barrier/shield */
		uint8_t result_local; /* internal tracking. garbage */
		uint8_t ident_local; /* internal tracking. garbage */
	};

	struct Agent {
		uint64_t addr;
		uint32_t prof;
		uint32_t is_elite;
		int16_t toughness;
		int16_t concentration;
		int16_t healing;
		int16_t pad1;
		int16_t condition;
		int16_t pad2;
		std::string name;
		uint16_t instance_id;
		uint64_t first_aware;
		uint64_t last_aware;
		uint64_t master_addr;
		uint16_t id;
		uint32_t hits;
		bool is_player;
		bool is_gadget;
		bool is_character;
	};

	struct Player {
		uint64_t addr;
		std::string name;
		std::string account;
		uint32_t profession;
		std::string profession_name;
		std::string profession_name_short;
		uint32_t elite_spec;
		std::string elite_spec_name;
		std::string elite_spec_name_short;
		uint16_t subgroup;
		uint64_t first_aware;
		uint64_t last_aware;
		
		uint32_t physical_damage;
		uint32_t condi_damage;
		uint32_t dps;

		uint32_t boss_physical_damage;
		uint32_t boss_condi_damage;
		uint32_t boss_dps;

		uint32_t might_current;
		uint32_t might_applied;
		uint32_t might_events;
		uint64_t might_last_event;
		float might_avg;

		std::string note;
		uint32_t note_counter;

		friend inline bool operator<(const Player& lhs, const Player& rhs);
	};

	struct Skill {
		int32_t id;
		std::string name;
	};

	struct Log {
		std::string version;
		BossID area_id;
		std::string encounter_name;
		std::string error;

		std::vector<Player> players;
		float reward_at;
		float boss_lifetime;
	};

	class Parser
	{
		const unsigned char* buf;
		size_t buf_len;
		uint64_t boss_addr;
		uint64_t last_event_time;
	public:
		std::unordered_map<uint64_t, Agent> agents;
		std::unordered_map<uint16_t, uint64_t> agent_addrs;
		std::unordered_map<uint64_t, Player> players;
		std::unordered_map<int32_t, Skill> skills;
		std::vector<CombatEvent> events;

		Parser(const unsigned char* buf, size_t len);
		~Parser();

		Log parse();
		std::string encounterName(BossID area_id);
		std::pair<std::string, std::string> professionName(uint32_t prof);
		std::pair<std::string, std::string> eliteSpecName(uint32_t elite);
	};

}

