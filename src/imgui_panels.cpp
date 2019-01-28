#include "imgui_panels.h"

void    AppLog::draw(const char* title, bool* p_open, ImGuiWindowFlags flags, Tracker* tracker)
{
    ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiSetCond_FirstUseEver);
	ImGui::Begin(title, p_open, flags);
	ImGui::PushAllowKeyboardFocus(false);
	ImGui::BeginChild("Buttons",ImVec2(0,ImGui::GetItemsLineHeightWithSpacing()));
	if (ImGui::Button("Clear")) tracker->log_events.clear();
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::SameLine();
	filter.Draw("Filter", -50.0f);
	ImGui::EndChild();
    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
	int64_t last_mechanic_time = 0;
	bool beginning = true;
	if (copy) ImGui::LogToClipboard();

	std::lock_guard<std::mutex> lg(tracker->log_events_mtx);

	for (auto current_event = tracker->log_events.begin(); current_event != tracker->log_events.end(); ++current_event)
	{
		if (filter.PassFilter(current_event->getFilterText().c_str()))
		{
			if (!beginning
				&& current_event->time > (last_mechanic_time + line_break_frequency))
			{
				ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
			}
			last_mechanic_time = current_event->time;
			
			current_event->draw();
			beginning = false;
		}
    }

    if (scroll_to_bottom)
        ImGui::SetScrollHere(1.0f);
	scroll_to_bottom = false;
    ImGui::EndChild();
	ImGui::PopAllowKeyboardFocus();
    ImGui::End();
}

void    AppChart::clear(Tracker* tracker)
{
	tracker->resetAllPlayerStats();
}

void    AppChart::draw(Tracker* tracker, const char* title, bool* p_open, ImGuiWindowFlags flags, bool show_all)
{
    ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiSetCond_FirstUseEver);
    ImGui::Begin(title, p_open, flags);
	ImGui::PushAllowKeyboardFocus(false);

    float window_width = ImGui::GetWindowContentRegionWidth();
    bool expand = false;

	Player* current_player = nullptr;

    if (ImGui::Button("Clear")) clear(tracker);
    ImGui::SameLine();
    if (ImGui::Button("Export")) exportData(tracker);
    ImGui::SameLine();
    if (ImGui::Button("Copy")) ImGui::LogToClipboard();
	ImGui::SameLine();
	filter.Draw("Filter", -50.0f);
    ImGui::Separator();

    ImGui::BeginGroup();
    ImGui::Text("Name");
    ImGui::SameLine(getChartColumnLoc(window_width,1));
    ImGui::Text("Neutral");
    ImGui::SameLine(getChartColumnLoc(window_width,2));
    ImGui::Text("Failed");
    ImGui::SameLine(getChartColumnLoc(window_width,3));
    ImGui::Text("Downs");
    ImGui::SameLine(getChartColumnLoc(window_width,4));
    ImGui::Text("Deaths");
    ImGui::SameLine(getChartColumnLoc(window_width,5));
    ImGui::Text("Pulls");
    ImGui::EndGroup();

    ImGui::BeginChild("scrolling");
	for (auto current_entry = tracker->player_entries.begin(); current_entry != tracker->player_entries.end(); ++current_entry)
    {
		current_player = current_entry->player;

        if(current_entry->isRelevant())
        {
			ImGui::Separator();
            ImGui::PushItemWidth(window_width*0.9);
            ImGui::AlignFirstTextHeightToWidgets();
            expand = ImGui::TreeNode(current_player->name.c_str());

            ImGui::SameLine(getChartColumnLoc(window_width,1));
            ImGui::Text("%d", current_entry->mechanics_neutral);
            ImGui::SameLine(getChartColumnLoc(window_width,2));
            ImGui::Text("%d", current_entry->mechanics_failed);
            ImGui::SameLine(getChartColumnLoc(window_width,3));
            ImGui::Text("%d", current_entry->downs);
            ImGui::SameLine(getChartColumnLoc(window_width,4));
            ImGui::Text("%d", current_entry->deaths);
            ImGui::SameLine(getChartColumnLoc(window_width,5));
            ImGui::Text("%d", current_entry->pulls);
            ImGui::PopItemWidth();

            if(expand
				|| show_all)
            {
				for (auto current_player_mechanics = current_entry->entries.begin(); current_player_mechanics != current_entry->entries.end(); ++current_player_mechanics)
                {
					if (!current_player_mechanics->isRelevant()) continue;
					if (!filter.PassFilter(current_player_mechanics->mechanic->name.c_str())) continue;

                    ImGui::PushItemWidth(window_width*0.9);
                    ImGui::Indent();
                    ImGui::Text(current_player_mechanics->mechanic->name.c_str());
                    if(!current_player_mechanics->mechanic->fail_if_hit)
                    {
                        ImGui::SameLine(getChartColumnLoc(window_width,1));
                    }
                    else
                    {
                        ImGui::SameLine(getChartColumnLoc(window_width,2));
                    }
                    ImGui::Text("%d", current_player_mechanics->hits);
                    ImGui::SameLine(getChartColumnLoc(window_width,5));
                    ImGui::Text("%d", current_player_mechanics->pulls);
                    ImGui::Unindent();
                    ImGui::PopItemWidth();

                    ImGui::Separator();
                }

				if (expand) ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild();
	ImGui::PopAllowKeyboardFocus();
    ImGui::End();
}

float getChartColumnWidth(float window_width)
{
    return window_width/6.0*3.0/5.0;
}

float getChartColumnLoc(float window_width, uint16_t col)
{
     return (window_width/3.0) + col * getChartColumnWidth(window_width);
}

std::string AppChart::toString(Tracker* tracker)
{
    std::string output = "";

    output += "Player Name,Account Name,Boss Name,Mechanic Name,Neutral,Failed,Downs,Deaths,Pulls\n";

	for (auto current_entry = tracker->player_entries.begin(); current_entry != tracker->player_entries.end(); ++current_entry)
    {
        if(current_entry->isRelevant())
        {
            output += current_entry->toString();
        }
    }
    return output;
}

void AppChart::exportData(Tracker* tracker)
{
	writeToDisk(tracker);

	ShellExecuteA(NULL, "open", last_file_path.c_str(), NULL, export_dir.c_str(), SW_HIDE);
}

void AppChart::writeToDisk(Tracker* tracker)
{
    uint16_t new_export_total = tracker->getMechanicsTotal();
    if(last_export_total == new_export_total || new_export_total < 2)
    {
        return;
    }

	std::string text = toString(tracker);

    std::time_t t = std::time(nullptr);
    char time_str[100];
    if (std::strftime(time_str, sizeof(time_str), "%Y%m%d-%H%M%S", std::localtime(&t)))
    {
        //std::cout << time_str << '\n';
    }

    CreateDirectory(export_dir.c_str(), NULL);

	std::string file_path = export_dir + "\\" + std::string(time_str) + "-" + std::to_string(new_export_total) + ".csv";

    std::ofstream out(file_path);
    out << text;
    out.close();

    last_export_total = new_export_total;

	last_file_path = file_path;
}

std::string AppChart::getDefaultExportPath()
{
	CHAR my_documents[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
	if (result != S_OK)
	{
		//std::cout << "Error: " << result << "\n";
	}
	else
	{
		return std::string(my_documents) + "\\Guild Wars 2\\addons\\arcdps\\arcdps.mechanics";
	}
	return "";
}

void AppOptions::draw(Options* options, Tracker* tracker, const char * title, bool * p_open, ImGuiWindowFlags flags)
{
	ImGui::SetNextWindowSize(ImVec2(550, 650), ImGuiSetCond_FirstUseEver);
	ImGui::Begin(title, p_open, flags);
	ImGui::PushAllowKeyboardFocus(false);
	
	ImGui::Checkbox("Only show mechanics for self", &options->show_only_self);

	ImGui::InputInt("Max mechanics in log", &tracker->max_log_events, 25);

	ImGui::Separator();
	
	ImGui::Text("Where to show each mechanic");

	ImGui::PushItemWidth(ImGui::GetWindowWidth()/3.0f);

	for (auto current_mechanic = options->mechanics->begin(); current_mechanic != options->mechanics->end(); ++current_mechanic)
	{
		ImGui::Combo(current_mechanic->getIniName().c_str(), &current_mechanic->verbosity,
			"Hidden\0"
			"Chart Only\0"
			"Log only\0"
			"Everywhere\0\0",4);
	}
	ImGui::PopItemWidth();

	ImGui::PopAllowKeyboardFocus();
	ImGui::End();
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
static void showHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}