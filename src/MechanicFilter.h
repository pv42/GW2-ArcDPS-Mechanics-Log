#pragma once

#include "LogEvent.h"
#include "npc_ids.h"
#include "imgui/imgui.h"

class MechanicFilter
{
public:

	ImGuiTextFilter filter_player;
	ImGuiTextFilter filter_boss;
	ImGuiTextFilter filter_mechanic;

	bool show_in_squad_only = false;
	bool show_all_mechanics = false;

	MechanicFilter();
	~MechanicFilter();

	void drawPopup();
	bool isActive();
	bool passFilter(Player* new_player, Boss* new_boss, Mechanic* new_mechanic, int new_display_section);
	bool passFilter(LogEvent* new_event);
};

