#ifndef SEARCH_FUNCTION_H
#define SEARCH_FUNCTION_H

#include "../ImGui/imgui.h"
#include "../ImGui/imgui_internal.h"
#include "../ImGui/custom_widgets.hpp"

extern float menu[4];

inline void RenderSearchFunction(ImDrawList* draw) {
    static int foundCount = -1;
    
    ImVec2 buttonSize = ImVec2(280, 40);
    ImGui::SetCursorPosX((315 - buttonSize.x) / 2);
    
    if (custom::Button("Search 67109633", buttonSize)) {
        kFox::ClearResult();
        kFox::SetSearchRange(RegionType::ALL);
        kFox::MemorySearch((char *)"67109633", Type::TYPE_DWORD);
        
        Maps_t* result = kFox::GetResult();
        foundCount = 0;
        while (result != nullptr) {
            foundCount++;
            result = result->next;
        }
    }
    
    if (foundCount != -1) {
        ImGui::Spacing();
        ImGui::SetCursorPosX((315 - ImGui::CalcTextSize("Results found: 999").x) / 2);
        ImGui::Text("Results found: %d", foundCount);
    }
}

#endif // SEARCH_FUNCTION_H
