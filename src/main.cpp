#define _CRT_SECURE_NO_WARNINGS
#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "discord.h"

#if defined(_WIN32)
#pragma pack(push, 1)
struct BitmapImageHeader {
    uint32_t const structSize{sizeof(BitmapImageHeader)};
    int32_t width{0};
    int32_t height{0};
    uint16_t const planes{1};
    uint16_t const bpp{32};
    uint32_t const pad0{0};
    uint32_t const pad1{0};
    uint32_t const hres{2835};
    uint32_t const vres{2835};
    uint32_t const pad4{0};
    uint32_t const pad5{0};

    BitmapImageHeader& operator=(BitmapImageHeader const&) = delete;
};

struct BitmapFileHeader {
    uint8_t const magic0{'B'};
    uint8_t const magic1{'M'};
    uint32_t size{0};
    uint32_t const pad{0};
    uint32_t const offset{sizeof(BitmapFileHeader) + sizeof(BitmapImageHeader)};

    BitmapFileHeader& operator=(BitmapFileHeader const&) = delete;
};
#pragma pack(pop)
#endif

struct DiscordState {
    discord::User currentUser;

    std::unique_ptr<discord::Core> core;
};

namespace {
volatile bool interrupted{false};
}

int main(int, char**)
{
    DiscordState state{};

    discord::Core* core{};
    auto result = discord::Core::Create(1383983730787356733, DiscordCreateFlags_Default, &core);
    state.core.reset(core);
    if (!state.core) {
        std::cout << "Failed to instantiate discord core! (err " << static_cast<int>(result)
                  << ")\n";
        std::exit(-1);
    }

    // Set up logging
    state.core->SetLogHook(
      discord::LogLevel::Info, [](discord::LogLevel level, const char* message) {
          std::cout << "Discord Log: " << message << "\n";
      });

    // Get current user info when ready
    core->UserManager().OnCurrentUserUpdate.Connect([&state]() {
        state.core->UserManager().GetCurrentUser(&state.currentUser);
        std::cout << "Connected as: " << state.currentUser.GetUsername() << "#"
                  << state.currentUser.GetDiscriminator() << "\n";
    });

    // Set up NTS Radio Rich Presence
    std::cout << "Setting up NTS Radio Rich Presence...\n";
    
    discord::Activity activity{};
    activity.SetType(discord::ActivityType::Listening);
    activity.SetDetails("NTS Radio");
    activity.SetState("Live Electronic Music");
    
    // Set up assets (you would need to upload these to your Discord app)
    activity.GetAssets().SetLargeImage("nts_logo");  // Upload NTS logo to Discord app
    activity.GetAssets().SetLargeText("NTS Radio - The World's Most Adventurous Music Platform");
    activity.GetAssets().SetSmallImage("radio_icon");  // Upload radio icon to Discord app
    activity.GetAssets().SetSmallText("Listening to live radio");
    
    // Update the activity
    state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
        if (result == discord::Result::Ok) {
            std::cout << "NTS Radio Rich Presence activated!\n";
            std::cout << "Your Discord status now shows you're listening to NTS Radio.\n";
        } else {
            std::cout << "Failed to set Rich Presence. Error: " << static_cast<int>(result) << "\n";
        }
    });

    std::signal(SIGINT, [](int) { interrupted = true; });

    do {
        state.core->RunCallbacks();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    } while (!interrupted);

    return 0;
}
