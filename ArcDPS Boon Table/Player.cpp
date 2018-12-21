#include "Player.h"

std::mutex boons_mtx;

bool Player::operator==(uintptr_t other_id)
{
	return id == other_id;
}

bool Player::operator==(std::string other_name)
{
	return name == other_name
		|| account_name == other_name;
}

Player::Player(uintptr_t new_id, std::string new_name, std::string new_account_name, uint8_t new_subgroup)
{
	id = new_id;
	name = new_name;
	account_name = new_account_name;
	enter_combat_time = getCurrentTime();
	in_combat = false;
	subgroup = new_subgroup;
	is_relevant = true;
}

Player::~Player()
{

}

bool Player::isRelevant()
{
	return is_relevant;
}

void Player::applyBoon(cbtevent* ev)
{
	if (!ev) return;
	if (ev->value == 0) return;
//	if (ev->value <= ev->overstack_value) return;

	BoonDef* current_def = getTrackedBoon(ev->skillid);
	if (!current_def) return;
	
	std::list<Boon>* current_boon_list = in_combat ? &boons : &boons_initial;

	Boon* current_boon = getBoon(current_boon_list, ev->skillid);

	if (current_boon)
	{
		current_boon->Apply(getBuffApplyDuration(ev));
	}
	else
	{
		std::lock_guard<std::mutex> lock(boons_mtx);
		current_boon_list->push_back(Boon(current_def, getBuffApplyDuration(ev)));
	}
}

void Player::removeBoon(cbtevent* ev)
{
	if (!ev) return;
	if (ev->value == 0) return;
//	if (ev->value <= ev->overstack_value) return;
	if (!getTrackedBoon(ev->skillid)) return;

	Boon* current_boon = nullptr;
	if (in_combat)
		current_boon = getBoon(&boons, ev->skillid);
	else
		current_boon = getBoon(&boons_initial, ev->skillid);

	if (current_boon)
	{
		current_boon->Remove(ev->value);
	}
}

Boon* Player::getBoon(std::list<Boon>* new_boons_list, uint32_t new_boon)
{
	std::lock_guard<std::mutex> lock(boons_mtx);
	auto it = std::find(new_boons_list->begin(), new_boons_list->end(), new_boon);

	//boon not tracked
	if (it == new_boons_list->end())
	{
		return nullptr;
	}
	else//boon tracked
	{
		return &*it;
	}
}

float Player::getBoonUptime(BoonDef* new_boon)
{
	if (getCombatTime() == 0) return 0.0f;

	Boon* current_boon = getBoon(&boons, new_boon->id);

	if (current_boon)
	{
		double out = (double)current_boon->getDuration(in_combat ? getCurrentTime() : exit_combat_time) / (double)getCombatTime();

		switch (new_boon->stacking_type)
		{
		case StackingType_duration:
			out = out > 1.0f ? 1.0f : out;
			break;
		case StackingType_intensity:
			out = out > 25.0f ? 25.0f : out;
			break;
		case StackingType_single:
			out = out > 1.0f ? 1.0f : out;
			break;
		default:
			out = out > 1.0f ? 1.0f : out;
			break;
		}

		if (out < 0.0f) out = 0.0f;

		return out;
	}
	
	return 0.0f;
}

bool Player::hasBoonNow(BoonDef * new_boon)
{
	if (!in_combat) return false;

	Boon* current_boon = getBoon(&boons, new_boon->id);

	if (current_boon)
	{
		return current_boon->expected_end_time > getCurrentTime();
	}
	return false;
}

void Player::combatEnter(cbtevent* ev)
{
	if (!ev) return;
	enter_combat_time = ev->time;
	in_combat = true;
	subgroup = ev->dst_agent;

	std::lock_guard<std::mutex> lock(boons_mtx);
	boons.clear();

	for (auto current_initial_boon = boons_initial.begin(); current_initial_boon != boons_initial.end(); ++current_initial_boon)
	{
		if (current_initial_boon->getDurationRemaining(ev->time) > 0)
		{
			boons.push_back(Boon(current_initial_boon->def, current_initial_boon->getDurationRemaining(ev->time)));
		}
	}

	boons_initial.clear();
}

void Player::combatExit()
{
	exit_combat_time = getCurrentTime();
	in_combat = false;
}

uint64_t Player::getCombatTime()
{
	if (in_combat)
	{
		return getCurrentTime() - enter_combat_time;
	}
	else
	{
		return exit_combat_time - enter_combat_time;
	}
}
