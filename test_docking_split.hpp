#include <imgui_internal.h>

ImGuiID GetOrCreateWindowDockID(ImGuiWindow* window)
{
    ImGuiID res = window->DockId;
    if (res == 0) // Dockspace not yet created by imgui
    {
        // Create it programmatically
        res = ImGui::DockBuilderAddNode();
        ImGui::DockBuilderSetNodePos(res, window->Pos);
        ImGui::DockBuilderSetNodeSize(res, window->Size);
        ImGui::DockBuilderDockWindow(window->Name, res);
        ImGui::DockBuilderFinish(res);
    }
    return res;
}

class Test
{
    struct Child
    {
        bool open = {};
        bool open_side = {};
        ImGuiID dock_id = {};
    };
    const char* _window_label = "Test Docking Split Bug";
    ImVector<Child> _children;
    bool _nested_children = true;
    bool _late_dock = false;
    bool _reset = true;

    static Test _g;

    Test()
    {
        _children.push_back(Child{});
        _children.push_back(Child{});
    };

    void openChild(ImGuiWindow* window, Child& child, bool split_open)
    {
        child.open = true;
        ImGuiID parent = GetOrCreateWindowDockID(window);
        if (split_open)
        {
            child.dock_id = ImGui::DockBuilderSplitNode(parent, ImGuiDir_Right, 0.5f, nullptr, nullptr);
            ImGui::DockBuilderFinish(parent);
        }
        else
        {
            child.dock_id = parent;
        }
    }

    void declareChildren()
    {
        for (size_t i = 0; i < _children.size(); ++i)
        {
            Child& child = _children[i];
            if (child.open)
            {
                char child_label[] = "Test  ";
                child_label[5] = '0' + char(i);
                if (_late_dock && child.dock_id == 0)
                {
                    openChild(ImGui::FindWindowByName(_window_label), child, child.open_side);
                }
                ImGui::SetNextWindowDockID(child.dock_id, ImGuiCond_Appearing);
                if (ImGui::Begin(child_label, &child.open))
                {
                    ImGui::Text("Lorem Ipsum %d", i);
                }
                child.dock_id = ImGui::GetWindowDockID();
                ImGui::End();
            }
        }
    }

    void declare()
    {
        if (_reset)
        {
            ImGui::SetNextWindowFocus();
            ImGui::SetNextWindowSize(ImVec2(512, 0), ImGuiCond_Always);
            _reset = false;
        }
        if (ImGui::Begin(_window_label))
        {
            ImGui::Checkbox("Use Nested children", &_nested_children);
            ImGui::SetItemTooltip(_nested_children ? "Child windows are declared before End()" : "Child windows are declared after End()");
            ImGui::Checkbox("Late child docking", &_late_dock);
            ImGui::SetItemTooltip(_late_dock ? "DockBuilder is called just before child's Begin()" : "DockBuilder is called when button is cliked (necessarily before End())");

            bool bug_should_trigger = !(!_nested_children && _late_dock);
            ImGui::BeginDisabled();
            ImGui::Checkbox("Bug should trigger", &bug_should_trigger);
            ImGui::EndDisabled();

            auto control_child = [&](int i)
            {
                Child& child = _children[i];
                char child_label[] = "Test  ";
                child_label[5] = '0' + char(i);
                ImGui::PushID(i);
                ImGui::SeparatorText(child_label);
                bool should_open = false;
                bool split_open = false;
                ImGui::BeginDisabled(child.open);
                if (ImGui::Button("Open Split Side ->"))
                {
                    should_open = true;
                    split_open = true;
                }
                ImGui::SetItemTooltip("This will trigger the bug.");
                if (ImGui::Button("Open Tab ^"))
                {
                    should_open = true;
                    split_open = false;
                }
                ImGui::SetItemTooltip("This works perfectly fine");
                ImGui::EndDisabled();
                ImGui::PopID();

                if (should_open)
                {
                    if (_late_dock)
                    {
                        child.open = true;
                        child.open_side = split_open;
                        child.dock_id = 0;
                    }
                    else
                    {
                        openChild(ImGui::GetCurrentWindow(), child, split_open);
                    }
                }
            };
            for (size_t i = 0; i < _children.size(); ++i)
            {
                control_child(i);
            }
        }
        if (_nested_children)
        {
            declareChildren();
        }
        ImGui::End();
        if (!_nested_children)
        {
            declareChildren();
        }
    }

public:
    // Call during an ImGui frame
    static void Declare()
    {
        _g.declare();
    }
};
Test Test::_g = {};
