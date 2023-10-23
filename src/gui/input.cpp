#include "gui/input.hpp"
#include <GLFW/glfw3.h>

namespace Toolbox::UI::Input {
    // Anonymous namespace within Input allows us to have private-scope variables that are only
    // accessible from this TU. This allows us to hide direct access to the input state variables
    // and only allow access via the Get functions.
    namespace {
        constexpr uint32_t KEY_MAX          = 512;
        constexpr uint32_t MOUSE_BUTTON_MAX = 3;

        glm::vec2 mMousePosition;
        glm::vec2 mMouseDelta;
        int32_t mMouseScrollDelta;

        bool mMouseWrapped;

        bool mKeysDown[KEY_MAX];
        bool mPrevKeysDown[KEY_MAX];

        bool mMouseButtonsDown[MOUSE_BUTTON_MAX];
        bool mPrevMouseButtonsDown[MOUSE_BUTTON_MAX];
        glm::vec2 mPrevMousePosition;

        void SetKeyboardState(uint32_t key, bool pressed) { mKeysDown[key] = pressed; }

        void SetMouseState(uint32_t button, bool pressed) { mMouseButtonsDown[button] = pressed; }

        void SetMousePosition(uint32_t x, uint32_t y) { mMousePosition = glm::vec2(x, y); }

        void SetMouseScrollDelta(uint32_t delta) { mMouseScrollDelta = delta; }
    }  // namespace
}  // namespace Toolbox::UI::Input

bool Toolbox::UI::Input::GetKey(uint32_t key) { return mKeysDown[key]; }

bool Toolbox::UI::Input::GetKeyDown(uint32_t key) { return mKeysDown[key] && !mPrevKeysDown[key]; }

bool Toolbox::UI::Input::GetKeyUp(uint32_t key) { return mPrevKeysDown[key] && !mKeysDown[key]; }

bool Toolbox::UI::Input::GetMouseButton(uint32_t button) { return mMouseButtonsDown[button]; }

bool Toolbox::UI::Input::GetMouseButtonDown(uint32_t button) {
    return mMouseButtonsDown[button] && !mPrevMouseButtonsDown[button];
}

bool Toolbox::UI::Input::GetMouseButtonUp(uint32_t button) {
    return mPrevMouseButtonsDown[button] && !mMouseButtonsDown[button];
}

glm::vec2 Toolbox::UI::Input::GetMousePosition() { return mMousePosition; }

glm::vec2 Toolbox::UI::Input::GetMouseDelta() { return mMouseDelta; }

int32_t Toolbox::UI::Input::GetMouseScrollDelta() { return mMouseScrollDelta; }

bool Toolbox::UI::Input::GetMouseWrapped() { return mMouseWrapped; }

void Toolbox::UI::Input::SetMouseWrapped(bool wrapped) { mMouseWrapped = wrapped; }

void Toolbox::UI::Input::UpdateInputState() {
    for (int i = 0; i < KEY_MAX; i++)
        mPrevKeysDown[i] = mKeysDown[i];
    for (int i = 0; i < MOUSE_BUTTON_MAX; i++)
        mPrevMouseButtonsDown[i] = mMouseButtonsDown[i];

    if (!mMouseWrapped) {
        mMouseDelta = mMousePosition - mPrevMousePosition;
    } else {
        mMouseWrapped = false;
    }
    mPrevMousePosition = mMousePosition;

    mMouseScrollDelta = 0;
}

void Toolbox::UI::Input::GLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action,
                                         int mods) {
    if (key >= KEY_MAX)
        return;

    if (action == GLFW_PRESS)
        SetKeyboardState(key, true);
    else if (action == GLFW_RELEASE)
        SetKeyboardState(key, false);
}

void Toolbox::UI::Input::GLFWMousePositionCallback(GLFWwindow *window, double xpos, double ypos) {
    SetMousePosition(xpos, ypos);
}

void Toolbox::UI::Input::GLFWMouseButtonCallback(GLFWwindow *window, int button, int action,
                                                 int mods) {
    if (button >= MOUSE_BUTTON_MAX)
        return;

    if (action == GLFW_PRESS)
        SetMouseState(button, true);
    else if (action == GLFW_RELEASE)
        SetMouseState(button, false);
}

void Toolbox::UI::Input::GLFWMouseScrollCallback(GLFWwindow *window, double xoffset,
                                                 double yoffset) {
    SetMouseScrollDelta(yoffset);
}
