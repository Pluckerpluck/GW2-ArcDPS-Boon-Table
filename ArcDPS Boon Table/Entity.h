#pragma once

#include <string>
#include <map>
#include <mutex>

#include "extension/arcdps_structs.h"
#include "Helpers.h"
#include "Boon.h"
#include "BuffIds.h"

class Entity
{
public:
	uintptr_t id;
	std::string name;

	std::map<uint32_t, Boon> boons_uptime;
	std::map<uint32_t, Boon> boons_uptime_initial;
	std::map<uint32_t, Boon> boons_generation;
	std::map<uint32_t, Boon> boons_generation_initial;

	virtual bool operator==(uintptr_t other_id) const;
	virtual bool operator==(std::string other_name) const;
	virtual bool operator==(const Entity& other) const;

	void applyBoon(cbtevent* ev);
	void removeBoon(cbtevent* ev);
	void gaveBoon(cbtevent* ev);
	void flushAllBoons();

	float getBoonUptime(const BoonDef& boon) const;
	float getBoonGeneration(const BoonDef& new_boon) const;

	virtual void combatEnter(cbtevent* ev);
	void combatExit(cbtevent* ev);
	uint64_t getCombatDuration() const;
	uint64_t enter_combat_time = getCurrentTime();
	uint64_t exit_combat_time = getCurrentTime();;
	bool in_combat = false;

	virtual ImVec4 getColor() const;
};

extern std::mutex boons_mtx;
