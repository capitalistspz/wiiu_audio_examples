#include <stdio.h>

#include "stb_vorbis.h"

#include <sndcore2/core.h>
#include <sndcore2/voice.h>
#include <wut_extra/multivoice.h>
#include <whb/proc.h>
#include <whb/log_console.h>
#include <whb/log.h>

#include <stdlib.h>

// Modify this if the path is different
#define OGG_FILE_PATH "wiiu/stereo-sample.ogg"

#define AX_VOICE_MAX_PRIORITY 31


static int get_samples_from_file(const char *filepath, int16_t **outLeftSamples, int16_t **outRightSamples,
                                 unsigned *outSampleRate);

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
    int16_t *rightSamples = nullptr;
    int16_t *leftSamples = nullptr;
    const int sampleCount = get_samples_from_file(OGG_FILE_PATH, &leftSamples, &rightSamples, &sampleRate);
    if (sampleCount < 0) {
        WHBLogPrintf("Failed to read samples from file");
        goto wait_for_exit;
    }

    // Acquire a multi-channel voice
    AXMultiVoice *multiVoice;
    AXMultiVoiceAcquireInfo info;
    info.channelCount = 2;
    const auto acquireResult = AXAcquireMultiVoice(AX_VOICE_MAX_PRIORITY, nullptr, nullptr, &info, &multiVoice);
    if (acquireResult != 0) {
        WHBLogPrintf("Failed to acquire multi-voice: %d", acquireResult);
        goto free_samples;
    }

    const AXVoiceOffsets offsets[2] = {
        {
            // Audio format
            .dataType = AX_VOICE_FORMAT_LPCM16,
            .loopingEnabled = false,
            .loopOffset = 0,
            // End point
            .endOffset = sampleCount,
            // Start point
            .currentOffset = 0,
            .data = leftSamples,
        },
        {
            // Audio format
            .dataType = AX_VOICE_FORMAT_LPCM16,
            .loopingEnabled = false,
            .loopOffset = 0,
            // End point
            .endOffset = sampleCount,
            // Start point
            .currentOffset = 0,
            .data = rightSamples,
        }
    };
    AXSetMultiVoiceOffsets(multiVoice, offsets);

    // Sample rate conversion
    const auto srcRatio = (float) sampleRate / (float) AXGetInputSamplesPerSec();
    AXSetMultiVoiceSrcType(multiVoice, AX_VOICE_SRC_TYPE_LINEAR);
    AXSetMultiVoiceSrcRatio(multiVoice, srcRatio);

    // Set volume of voice
    const AXVoiceVeData volume = {.volume = 0x8000, .delta = 0};
    AXSetMultiVoiceVe(multiVoice, &volume);

    // Set volume mix of voice for device
    AXSetMultiVoiceDeviceMix(multiVoice, AX_DEVICE_TYPE_DRC, 0, 0, 0x800, 0);
    AXSetMultiVoiceDeviceMix(multiVoice, AX_DEVICE_TYPE_TV, 0, 0, 0x800, 0);

    // Play voice
    AXSetMultiVoiceState(multiVoice, AX_VOICE_STATE_PLAYING);

    WHBLogPrint("Playing sound");
    WHBLogConsoleDraw();
    bool wasPlaying = true;
    earlyExit = false;

    // Wait for exit
    while (WHBProcIsRunning()) {
        if (!wasPlaying)
            continue;

        if (!AXIsMultiVoiceRunning(multiVoice)) {
            WHBLogPrint("Finished playing sound (exit app via HOME menu)");
            WHBLogConsoleDraw();
            wasPlaying = false;
        }
    }

    // Release voice resources
    AXFreeMultiVoice(multiVoice);

free_samples:
    free(leftSamples);
    free(rightSamples);

wait_for_exit:
    if (earlyExit) {
        WHBLogPrint("Waiting for exit... (exit via HOME menu)");
        WHBLogConsoleDraw();
        while (WHBProcIsRunning()) {
        }
    }
    // Free libraries
    AXQuit();
    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}

static int get_samples_from_file(const char *filepath, int16_t **outLeftSamples, int16_t **outRightSamples,
                                 unsigned *outSampleRate) {
    int error = 0;
    struct stb_vorbis *vorbis = stb_vorbis_open_filename(filepath, &error, nullptr);
    if (error) {
        WHBLogPrintf("stb_vorbis_open_filename error: %d", error);
        return -1;
    }
    const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    if (info.channels != 2) {
        stb_vorbis_close(vorbis);
        WHBLogPrintf("Error: This example only supports stereo audio, file audio has %d channels", info.channels);
        return -2;
    }
    const unsigned sampleCount = stb_vorbis_stream_length_in_samples(vorbis);

    // Get samples
    const auto leftSamples =  (int16_t *) malloc(sampleCount * sizeof(int16_t));
    const auto rightSamples =  (int16_t *) malloc(sampleCount * sizeof(int16_t));
    int16_t *samplesArray[2] = {leftSamples, rightSamples};
    const auto actualSampleCount = stb_vorbis_get_samples_short(vorbis, 2, samplesArray, (int) sampleCount);
    for (auto i = 0; i < sampleCount; ++i) {
        if (rightSamples[i] != 0) {
            OSReport("%d\n", i);
            break;
        }
    }
    *outRightSamples = rightSamples;
    *outLeftSamples = leftSamples;

    stb_vorbis_close(vorbis);
    *outSampleRate = info.sample_rate;
    return (int) actualSampleCount;
}
