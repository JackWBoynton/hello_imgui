
#include "provider_selector.hpp"

#include "lib.hpp"

void ProviderSelectorView::Render() {
      if (ImGui::BeginChild("Provider Selector", ImVec2(0, 0), true)) {
        for (auto& provider_type : workspace_lib::GetProviderTypes()) {
          if (ImGui::Button(provider_type.c_str())) {
            if (auto provider = workspace_lib::CreateProvider(provider_type); provider) {
              // ...
            }
          }
        }

        ImGui::EndChild();
      }
}