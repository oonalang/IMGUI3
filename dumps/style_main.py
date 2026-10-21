import re, sys

P = "src/main/jni/Main.cpp"
s = open(P, encoding="utf-8").read()
orig = s

def rep(a, b, n=1):
    global s
    c = s.count(a)
    if c != n:
        print("MISS(%d!=%d): %r" % (c, n, a[:70]))
        return False
    s = s.replace(a, b)
    return True

# ---- window shell: body panel and header bar -------------------------------
rep("bodyMin, bodyMax, IM_COL32(7, 10, 20, 236), 16.0f);",
    "bodyMin, bodyMax, C(13, 15, 22, 222), 14.0f);")
rep("headerMin, headerMax, IM_COL32(7, 10, 20, 236), 11.0f);",
    "headerMin, headerMax, C(11, 13, 20, 225), 11.0f);")

# ---- login page: purple -> IMGUI5 blue --------------------------------
rep("wp, {wp.x + WW, wp.y + WH}, IM_COL32(20, 10, 35, 240), 20.0f);",
    "wp, {wp.x + WW, wp.y + WH}, C(13, 15, 22, 240), 14.0f);")
rep("wp, {wp.x + WW, wp.y + WH}, IM_COL32(160, 80, 255, 100), 20.0f, 0, 1.5f);",
    "wp, {wp.x + WW, wp.y + WH}, C(46, 51, 66, 255), 14.0f, 0, 1.0f);")
rep("{wp.x + 6, wp.y + 6}, {wp.x + WW - 6, wp.y + WH - 6}, IM_COL32(160, 80, 255, 30), 16.0f, 0, 1.0f);",
    "{wp.x + 6, wp.y + 6}, {wp.x + WW - 6, wp.y + WH - 6}, C(33, 37, 48, 255), 11.0f, 0, 1.0f);")
rep("wp.y + 40.0f}, IM_COL32(160, 80, 255, 255), brand);",
    "wp.y + 40.0f}, C(235, 238, 245, 255), brand);")
rep("wp.y + 88.0f}, IM_COL32(180, 180, 200, 200), sub);",
    "wp.y + 88.0f}, C(110, 116, 132, 255), sub);")
rep("{wp.x + WW - 60, wp.y + 110}, IM_COL32(160, 80, 255, 60), 1.0f);",
    "{wp.x + WW - 60, wp.y + 110}, C(33, 37, 48, 255), 1.0f);")
rep("wp.y + 128.0f}, IM_COL32(160, 160, 180, 220), prompt);",
    "wp.y + 128.0f}, C(158, 164, 180, 255), prompt);")
rep("{inputX + inputW, inputY + inputH}, IM_COL32(30, 15, 50, 220), 10.0f);",
    "{inputX + inputW, inputY + inputH}, C(20, 23, 31, 255), 6.0f);")
rep("ImU32 bc = keyFocused ? IM_COL32(160, 80, 255, 255) : IM_COL32(160, 80, 255, 80);",
    "ImU32 bc = keyFocused ? C(74, 159, 230, 255) : C(46, 51, 66, 255);")
rep("{inputX + inputW, inputY + inputH}, bc, 10.0f, 0, keyFocused ? 2.0f : 1.0f);",
    "{inputX + inputW, inputY + inputH}, bc, 6.0f, 0, keyFocused ? 2.0f : 1.0f);")
rep("ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 240, 255));",
    "ImGui::PushStyleColor(ImGuiCol_Text, C(235, 238, 245, 255));")
rep("ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(160, 80, 255, 220));",
    "ImGui::PushStyleColor(ImGuiCol_Button, C(58, 132, 199, 255));")
rep("ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(190, 110, 255, 255));",
    "ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C(74, 159, 230, 255));")
rep("ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(120, 50, 200, 255));",
    "ImGui::PushStyleColor(ImGuiCol_ButtonActive, C(46, 112, 172, 255));")
rep("ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(15, 12, 25, 255));",
    "ImGui::PushStyleColor(ImGuiCol_Text, C(255, 255, 255, 255));")
rep("if (ImGui::Button(\"AUTHENTICATE\", {inputW, 44.0f})) {",
    "if (ImGui::Button(\"AUTHENTICATE\", {inputW, 40.0f})) {")
rep("ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(160, 80, 255, 40));",
    "ImGui::PushStyleColor(ImGuiCol_Button, C(26, 29, 38, 255));")
rep("ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(160, 80, 255, 80));",
    "ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C(33, 37, 48, 255));")
rep("ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(160, 80, 255, 120));",
    "ImGui::PushStyleColor(ImGuiCol_ButtonActive, C(40, 45, 58, 255));")
rep("ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 80, 255, 220));",
    "ImGui::PushStyleColor(ImGuiCol_Text, C(158, 164, 180, 255));")
rep("{errPos.x + inputW, errPos.y + 30.0f}, IM_COL32(200, 50, 50, 40), 6.0f);",
    "{errPos.x + inputW, errPos.y + 30.0f}, C(239, 68, 68, 26), 6.0f);")
rep("{errPos.x + inputW, errPos.y + 30.0f}, IM_COL32(200, 50, 150, 150), 6.0f, 0, 1.0f);",
    "{errPos.x + inputW, errPos.y + 30.0f}, C(239, 68, 68, 130), 6.0f, 0, 1.0f);")
rep("wp.y + WH - 28.0f}, IM_COL32(120, 120, 140, 160), footer);",
    "wp.y + WH - 28.0f}, C(86, 92, 107, 255), footer);")

if s == orig:
    print("nothing changed")
else:
    open(P, "w", encoding="utf-8").write(s)
    print("Main.cpp updated")
