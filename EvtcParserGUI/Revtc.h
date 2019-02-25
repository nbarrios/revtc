#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <set>

namespace Revtc {

#define DEIMOS_HANDS 0x4345
#define KC_CONSTRUCT_CORE 0x3F85
#define MIGHT 0x2E4
#define QUICKNESS 0x4A3
#define ALACRITY 0x7678
#define FURY 0x2D5

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
		CONJURED_AMALGAMATE = 43974,
		NIKARE = 21105,
		KENUT = 21089,
		QUADIM = 20934,
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
		CBTS_SPAWN, // src_agent is now in game tracking range (not in realtime api)
		CBTS_DESPAWN, // src_agent is no longer being tracked (not in realtime api)
		CBTS_HEALTHUPDATE, // src_agent has reached a health marker. dst_agent = percent * 10000 (eg. 99.5% will be 9950) (not in realtime api)
		CBTS_LOGSTART, // log start. value = server unix timestamp **uint32**. buff_dmg = local unix timestamp. src_agent = 0x637261 (arcdps id)
		CBTS_LOGEND, // log end. value = server unix timestamp **uint32**. buff_dmg = local unix timestamp. src_agent = 0x637261 (arcdps id)
		CBTS_WEAPSWAP, // src_agent swapped weapon set. dst_agent = current set id (0/1 water, 4/5 land)
		CBTS_MAXHEALTHUPDATE, // src_agent has had it's maximum health changed. dst_agent = new max health (not in realtime api)
		CBTS_POINTOFVIEW, // src_agent is agent of "recording" player
		CBTS_LANGUAGE, // src_agent is text language
		CBTS_GWBUILD, // src_agent is game build
		CBTS_SHARDID, // src_agent is sever shard id
		CBTS_REWARD, // src_agent is self, dst_agent is reward id, value is reward type. these are the wiggly boxes that you get
		CBTS_BUFFINITIAL, // combat event that will appear once per buff per agent on logging start (statechange==18, buff==18, normal cbtevent otherwise)
		CBTS_POSITION, // src_agent changed, cast float* p = (float*)&dst_agent, access as x/y/z (float[3]) (not in realtime api)
		CBTS_VELOCITY, // src_agent changed, cast float* v = (float*)&dst_agent, access as x/y/z (float[3]) (not in realtime api)
		CBTS_FACING, // src_agent changed, cast float* f = (float*)&dst_agent, access as x/y (float[2]) (not in realtime api)
		CBTS_TEAMCHANGE, // src_agent change, dst_agent new team id
		CBTS_ATTACKTARGET, // src_agent is an attacktarget, dst_agent is the parent agent (gadget type), value is the current targetable state (not in realtime api)
		CBTS_TARGETABLE, // dst_agent is new target-able state (0 = no, 1 = yes. default yes) (not in realtime api)
		CBTS_MAPID, // src_agent is map id
		CBTS_REPLINFO, // internal use, won't see anywhere
		CBTS_STACKACTIVE, // src_agent is agent with buff, dst_agent is the stackid marked active
		CBTS_STACKRESET // src_agent is agent with buff, value is the duration to reset to (also marks inactive), pad61- is the stackid
	};

	/* combat buff remove type */
	enum cbtbuffremove {
		CBTB_NONE, // not used - not this kind of event
		CBTB_ALL, // all stacks removed
		CBTB_SINGLE, // single stack removed. disabled on server trigger, will happen for each stack on cleanse
		CBTB_MANUAL, // autoremoved by ooc or allstack (ignore for strip/cleanse calc, use for in/out volume)
	};

	struct CombatEventRev0 {
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
		uint8_t is_offcycle; /* zero if buff dmg happened during tick, non-zero otherwise */
		uint8_t pad64; /* internal tracking. garbage */
	};

	struct CombatEvent {
		uint64_t time;
		uint64_t src_agent;
		uint64_t dst_agent;
		int32_t value;
		int32_t buff_dmg;
		uint32_t overstack_value;
		uint32_t skillid;
		uint16_t src_instid;
		uint16_t dst_instid;
		uint16_t src_master_instid;
		uint16_t dst_master_instid;
		uint8_t iff;
		uint8_t buff;
		uint8_t result;
		uint8_t is_activation;
		uint8_t is_buffremove;
		uint8_t is_ninety;
		uint8_t is_fifty;
		uint8_t is_moving;
		uint8_t is_statechange;
		uint8_t is_flanking;
		uint8_t is_shields;
		uint8_t is_offcycle;
		uint32_t buff_instid;
	};

	enum class AgentType {
		Unknown,
		Gadget,
		Npc,
		Player
	};

	struct Agent {
		uint64_t addr;
		uint32_t prof;
		uint32_t is_elite;
		int16_t toughness;
		int16_t concentration;
		int16_t healing;
		int16_t hitbox_width;
		int16_t condition;
		int16_t hitbox_height;
		std::string name;
		uint16_t instance_id;
		uint64_t first_aware;
		bool first_aware_set;
		uint64_t last_aware;
		uint64_t master_addr;
		AgentType agtype;
		uint16_t species_id;
		uint32_t direct_damage;
		uint32_t boss_direct_damage;
		uint32_t condi_damage;
		uint32_t boss_condi_damage;
		uint32_t hits;
		uint32_t note_counter;
	};

	struct Boon {
		bool is_removal;
		uint64_t applied_at;
		int32_t  duration;
		uint64_t expires_at;
		uint32_t overstack;

		friend inline bool operator<(const Boon& lhs, const Boon& rhs);
	};

	struct DurationStack {
		std::vector<Boon> boon_queue;
		std::vector<Boon> boon;
		uint32_t boon_accum;
		uint32_t boon_samples;
		float boon_avg;
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

		std::set<uint64_t> slaves;
		
		uint32_t physical_damage;
		uint32_t condi_damage;
		uint32_t dps;

		uint32_t boss_physical_damage;
		uint32_t boss_condi_damage;
		uint32_t boss_dps;

		std::vector<Boon> might;
		uint32_t might_accum;
		uint32_t might_samples;
		std::vector<float> might_points;
		float might_avg;

		float quickness_avg;
		float alacrity_avg;
		float fury_avg;

		std::unordered_map<uint16_t, DurationStack> duration_boons;

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
		uint8_t revision;
		BossID area_id;
		std::set<uint16_t> boss_ids;
		std::string encounter_name;
		bool valid;
		std::string error;

		std::vector<Player> players;
		uint64_t reward_at;
		uint64_t boss_lifetime;
		uint64_t boss_death;
		uint64_t encounter_duration;
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
		void calcBoons(uint64_t encounter_duration);
		std::string encounterName(BossID area_id);
		std::pair<std::string, std::string> professionName(uint32_t prof);
		std::pair<std::string, std::string> eliteSpecName(uint32_t elite);
	};

}

