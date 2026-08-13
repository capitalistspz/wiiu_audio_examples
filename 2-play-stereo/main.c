#include "stb_vorbis.h"

#include <sndcore2/core.h>
#include <sndcore2/voice.h>
#include <whb/proc.h>
#include <whb/log_console.h>
#include <whb/log.h>

#include <stdlib.h>

#define AX_VOICE_MAX_PRIORITY 31

// Define as you wish
#define OGG_FILE_PATH "/vol/cz/stereo-test-new.ogg"


int get_samples_from_file(const char* filepath, int16_t** outLeftSamples, int16_t** outRightSamples, unsigned* outSampleRate);

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

    unsigned sampleRate = 1;
    int16_t* rightSamples = nullptr;
    int16_t* leftSamples = nullptr;
    const int sampleCount = get_samples_from_file(OGG_FILE_PATH, &leftSamples, &rightSamples, &sampleRate);
    if (sampleCount < 0)
    {
        WHBLogPrintf("Failed to read samples from file");
        goto wait_for_exit;
    }

    // Acquire a voice for each channel
    AXVoice* leftVoice = AXAcquireVoice(AX_VOICE_MAX_PRIORITY, nullptr, nullptr);
    if (!leftVoice)
    {
        WHBLogPrint("AXAcquireVoice error: Failed to acquire left voice");
        goto free_samples;
    }

    AXVoice* rightVoice = AXAcquireVoice(AX_VOICE_MAX_PRIORITY, nullptr, nullptr);
    if (!leftVoice)
    {
        AXFreeVoice(leftVoice);
        WHBLogPrint("AXAcquireVoice error: Failed to acquire right voice");
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
        .data = leftSamples,
    };
    AXSetVoiceOffsets(leftVoice, &offsets);

    offsets.data = rightSamples;
    AXSetVoiceOffsets(rightVoice, &offsets);

    // Sample rate conversion
    const auto srcRatio = (float)sampleRate / (float)AXGetInputSamplesPerSec();
    AXSetVoiceSrcType(leftVoice, AX_VOICE_SRC_TYPE_LINEAR);
    AXSetVoiceSrcRatio(leftVoice, srcRatio);

    AXSetVoiceSrcType(rightVoice, AX_VOICE_SRC_TYPE_LINEAR);
    AXSetVoiceSrcRatio(rightVoice, srcRatio);

    // Set sound volume
    AXVoiceVeData volume = {.volume = 0x1000, .delta = 0};
    AXSetVoiceVe(leftVoice, &volume);
    AXSetVoiceVe(rightVoice, &volume);

    AXVoiceDeviceMixData mixData[2] = {};
    // Left ear volume
    mixData[0].bus[0].volume = 0x800;
    mixData[0].bus[0].delta = 0;
    // Right ear volume
    mixData[1].bus[0].volume = 0;
    mixData[1].bus[0].delta = 0;
    AXSetVoiceDeviceMix(leftVoice, AX_DEVICE_TYPE_DRC, 0, mixData);
    mixData[0].bus[0].volume = 0;
    mixData[1].bus[0].volume = 0x800;
    AXSetVoiceDeviceMix(rightVoice, AX_DEVICE_TYPE_DRC, 0, mixData);

    // Play voice
    AXSetVoiceState(leftVoice, AX_VOICE_STATE_PLAYING);
    AXSetVoiceState(rightVoice, AX_VOICE_STATE_PLAYING);

    WHBLogPrint("Playing sound");
    WHBLogConsoleDraw();
    bool wasPlaying = true;
    earlyExit = false;

    // Wait for exit
    while (WHBProcIsRunning())
    {
        if (!wasPlaying)
            continue;

        if (!AXIsVoiceRunning(leftVoice))
        {
            WHBLogPrint("Finished playing sound (exit app via HOME menu)");
            WHBLogConsoleDraw();
            wasPlaying = false;
        }
    }

    // Release voice resources
    AXFreeVoice(leftVoice);
    AXFreeVoice(rightVoice);

    free_samples:
    free(leftSamples);
    free(rightSamples);

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

int get_samples_from_file(const char* filepath, int16_t** outLeftSamples, int16_t** outRightSamples, unsigned* outSampleRate)
{
    int error = 0;
    struct stb_vorbis* vorbis = stb_vorbis_open_filename(filepath, &error, nullptr);
    if (error)
    {
        WHBLogPrintf("stb_vorbis_open_filename error: %d", error);
        return -1;
    }
    const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    if (info.channels != 2)
    {
        stb_vorbis_close(vorbis);
        WHBLogPrintf("Error: This example only supports stereo audio, file audio has %d channels", info.channels);
        return -2;
    }
    const unsigned sampleCount = stb_vorbis_stream_length_in_samples(vorbis);

    // Get samples
    *outRightSamples = (int16_t*)malloc(sampleCount * sizeof(int16_t));
    *outLeftSamples = (int16_t*)malloc(sampleCount * sizeof(int16_t));
    int16_t* samplesArray[2] = { *outLeftSamples, *outRightSamples };
    stb_vorbis_get_samples_short(vorbis, 2, samplesArray, (int)sampleCount);

    stb_vorbis_close(vorbis);
    *outSampleRate = info.sample_rate;
    return (int)sampleCount;
}