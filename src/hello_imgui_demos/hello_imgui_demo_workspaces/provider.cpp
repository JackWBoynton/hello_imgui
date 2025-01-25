
#include "provider.hpp"
#include "lib.hpp"

void Provider::RenderDockspaceContent()
{
    if (jacks.empty())
    {
        jacks.emplace_back(std::make_unique<JacksView>(ViewDockspace<Provider>::label + "_LeftSpace"));
        jacks.emplace_back(std::make_unique<JacksView>(ViewDockspace<Provider>::label + "_RightSpace"));
        ctr += 2;

        modal = std::make_unique<SomeModal>(ViewDockspace<Provider>::label);
    }

    if (ImGui::Button("Add Jack UD"))
    {
        if (ctr % 2 == 0)
            jacks.emplace_back(std::make_unique<JacksView>(ViewDockspace<Provider>::label));
        else
            jacks.emplace_back(std::make_unique<JacksView>(ViewDockspace<Provider>::label + "_CommandSpace"));
        ctr++;
    }
    if (ImGui::Button("Add Jack LR"))
    {
        if (ctr % 2 == 0)
            jacks.emplace_back(std::make_unique<JacksView>(ViewDockspace<Provider>::label + "_LeftSpace"));
        else
            jacks.emplace_back(std::make_unique<JacksView>(ViewDockspace<Provider>::label + "_RightSpace"));
        ctr++;
    }

    if (ImGui::Button("Timeseries"))
    {
        timeseriesPlots.emplace_back(
            std::make_unique<TimeseriesPlot>(ViewDockspace<Provider>::label + "_CommandSpace"));
    }

    // convert the above to a combo
    static int selectedJack = 0;
    std::vector<std::string> jackNames;
    for (auto& jack : jacks)
        jackNames.push_back(jack->label);
    if (jackNames.size() > 0)
    {
        if (ImGui::BeginCombo("Focus Jack", jackNames[selectedJack].c_str()))
        {
            for (int i = 0; i < jackNames.size(); i++)
            {
                bool isSelected = (i == selectedJack);
                if (ImGui::Selectable(jackNames[i].c_str(), isSelected))
                    selectedJack = i;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Focus"))
            jacks[selectedJack]->focusWindowAtNextFrame = true;
    }

    if (ImGui::Button("Modal"))
        ImGui::OpenPopup(modal->label.c_str());
}