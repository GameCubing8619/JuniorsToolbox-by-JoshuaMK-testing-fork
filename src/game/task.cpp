#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "core/application.hpp"
#include "core/core.hpp"
#include "core/types.hpp"

#include "game/task.hpp"

#include "gui/settings.hpp"

using namespace Toolbox;

namespace Toolbox::Game {

    void TaskCommunicator::run() {
        while (!m_kill_flag.load()) {
            AppSettings &settings = SettingsManager::instance().getCurrentProfile();

            DolphinCommunicator &communicator =
                MainApplication::instance().getDolphinCommunicator();

            // Dismiss tasks if disconnected to avoid errors
            if (!communicator.manager().isHooked()) {
                // Clear tasks
                while (!m_task_queue.empty()) {
                    m_task_queue.pop();
                }
                TOOLBOX_DEBUG_LOG("Clearing tasks since communicator is not hooked!");
            } else {
                while (!m_task_queue.empty()) {
                    std::unique_lock<std::mutex> lk(m_mutex);
                    std::function<bool(DolphinCommunicator &)> task = m_task_queue.front();
                    if (task(communicator)) {
                        m_task_queue.pop();
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(settings.m_dolphin_refresh_rate));
        }

        m_kill_condition.notify_all();
    }

Result<void> TaskCommunicator::loadScene(u8 stage, u8 scenario) {
        constexpr u8 c_mar_director_id = 5;

        return submitTask([](Dolphin::DolphinCommunicator &communicator, u8 stage,
                      u8 scenario) {
                constexpr u32 application_addr = 0x803E9700;

                // Early exit to avoid errors
                if (!communicator.manager().isHooked()) {
                    return true;
                }

                auto game_stage_result = communicator.read<u8>(application_addr + 0xE);
                if (!game_stage_result) {
                    logError(game_stage_result.error());
                    return true;
                }

                auto game_scenario_result = communicator.read<u8>(application_addr + 0xF);
                if (!game_scenario_result) {
                    logError(game_scenario_result.error());
                    return true;
                }

                u8 game_stage = game_stage_result.value();
                u8 game_scenario = game_scenario_result.value();

                // The game stage is not what we want, tell the game to reload is possible
                if (game_stage != stage || game_scenario != game_scenario) {
                    communicator.write<u8>(application_addr + 0x12, stage);
                    communicator.write<u8>(application_addr + 0x13, scenario);

                    // In this case TMarDirector is running and we should
                    // also tell it about the changes
                    if (communicator.read<u8>(application_addr + 0x8).value() ==
                        c_mar_director_id) {
                        u32 mar_director_address =
                            communicator.read<u32>(application_addr + 0x4).value();

                        u16 mar_director_state =
                            communicator.read<u16>(mar_director_address + 0x4C).value();

                        u16 new_flags = 0;
                        if ((game_stage == 1 && stage == 5) || (game_stage == 1 && stage == 6) ||
                            (game_stage == 1 && stage == 11)) {
                            new_flags |= 0x8;
                        } else {
                            new_flags |= 0x2;
                        }
                        if (stage == 7) {
                            new_flags |= 0x100;
                        }

                        communicator.write<u16>(mar_director_address + 0x4C,
                                                mar_director_address | new_flags);
                    }
                }

                return true;
            },
            stage, scenario);
    }

Result<void> TaskCommunicator::addSceneObject(RefPtr<ISceneObject> object,
                                                                RefPtr<GroupSceneObject> parent) {
        return Result<void>();
}

Result<void>
TaskCommunicator::removeSceneObject(RefPtr<ISceneObject> object, RefPtr<GroupSceneObject> parent) {
        return Result<void>();
}

Result<void>
TaskCommunicator::updateSceneObjectParameter(const QualifiedName &member_name,
                                             RefPtr<ISceneObject> object,
                                             RefPtr<GroupSceneObject> parent) {
        return Result<void>();
}

Result<void>
TaskCommunicator::setObjectTransformToMario(RefPtr<ISceneObject> object,
                                            RefPtr<GroupSceneObject> parent) {
        return Result<void>();
}

Result<void>
TaskCommunicator::setObjectTransformToCamera(RefPtr<ISceneObject> object,
                                             RefPtr<GroupSceneObject> parent) {
        return Result<void>();
}

Result<void> TaskCommunicator::playCameraDemo(std::string_view demo_name) {
        return Result<void>();
}

Result<void>
TaskCommunicator::setCameraTransformToGameCamera(Transform &camera_transform) {
        return Result<void>();
}

Result<void>
TaskCommunicator::setMarioToCameraTransform(const Transform &camera_transform) {
        return Result<void>();
}

}  // namespace Toolbox::Game