#include "stb_vorbis.h"

#include <sndcore2/core.h>
#include <sndcore2/voice.h>
#include <whb/proc.h>
#include <whb/log_console.h>
#include <whb/log.h>

#include <stdlib.h>

// Modify this if the path is different
#define OGG_FILE_PATH "mono-sample.ogg"

#define AX_VOICE_MAX_PRIORITY 31

int main()
{
    bool earlyExit = true;
    // Initialize process transition library
    WHBProcInit();
    // Initialize screen log
    WHBLogConsoleInit();
    WHBLogConsoleDraw();

    // Initialize audio library
    AXInitParams params = {AX_INIT_RENDERER_48KHZ, {}, AX_INIT_PIPELINE_SINGLE};
    AXInitWithParams(&params);

    short *samples = nullptr;
    int channels = 0;
    int sampleRate = 0;
    const int sampleCount = stb_vorbis_decode_filename(OGG_FILE_PATH, &channels, &sampleRate, &samples);
    if (sampleCount < 0)
    {
        WHBLogPrintf("stb_vorbis_decode_filename error: failed to decode file: %d", sampleCount);
        goto wait_for_exit;
    }
    if (channels != 1)
    {
        free(samples);
        WHBLogPrintf("Error: This example only supports mono audio, file audio has %d channels", channels);
        goto free_samples;
    }

    // Acquire a voice, an object used to play sound
    AXVoice* voice = AXAcquireVoice(AX_VOICE_MAX_PRIORITY, nullptr, nullptr);
    if (!voice)
    {
        WHBLogPrint("AXAcquireVoice error: Failed to acquire a voice");
        goto free_samples;
    }

    AXVoiceOffsets offsets = {
        // Audio format
        .dataType = AX_VOICE_FORMAT_LPCM16,
        .loopingEnabled = false,
        .loopOffset = 0,
        // End point
        .endOffset = sampleCount,
        // Start point
        .currentOffset = 0,
        .data = samples,
    };
    AXSetVoiceOffsets(voice, &offsets);

    // Sample rate conversion
    const auto srcRatio = (float)sampleRate / (float)AXGetInputSamplesPerSec();
    AXSetVoiceSrcType(voice, AX_VOICE_SRC_TYPE_LINEAR);
    AXSetVoiceSrcRatio(voice, srcRatio);

    // Set sound volume
    AXVoiceVeData volume = {.volume = 0x1000, .delta = 0};
    AXSetVoiceVe(voice, &volume);

    AXVoiceDeviceMixData mixData[2] = {};
    // Left ear volume
    mixData[0].bus[0].volume = 0x800;
    mixData[0].bus[0].delta = 0;
    // Right ear volume
    mixData[1].bus[0].volume = 0x800;
    mixData[1].bus[0].delta = 0;
    AXSetVoiceDeviceMix(voice, AX_DEVICE_TYPE_DRC, 0, mixData);

    // Play voice
    AXSetVoiceState(voice, AX_VOICE_STATE_PLAYING);

    WHBLogPrint("Playing sound");
    WHBLogConsoleDraw();
    bool wasPlaying = true;
    earlyExit = false;

    // Wait for exit
    while (WHBProcIsRunning())
    {
        if (!wasPlaying)
            continue;

        if (!AXIsVoiceRunning(voice))
        {
            WHBLogPrint("Finished playing sound (exit app via HOME menu)");
            WHBLogConsoleDraw();
            wasPlaying = false;
        }
    }

    // Release voice resources
    AXFreeVoice(voice);

    free_samples:
    free(samples);

    wait_for_exit:
    if (earlyExit)
    {
        WHBLogPrint("Waiting for exit... (exit via HOME menu)");
        WHBLogConsoleDraw();
        while (WHBProcIsRunning())
        {

        }
    }
    // Free libraries
    AXQuit();
    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}
