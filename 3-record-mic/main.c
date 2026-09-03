#include <simple_wav.h>
#include <machine/endian.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>

#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <mic/mic.h>
#include <sndcore2/core.h>
#include <vpad/input.h>

#include <stdlib.h>
#include <sys/endian.h>

#include "simple_wav.h"

#define MAX_SAMPLES (1 << 18)
#define MIC_SAMPLE_RATE 32000

static void inplace_byteswap_16(int16_t *data, uint32_t size);

int main() {
    bool earlyExit = true;
    // Initialize process transition library
    WHBProcInit();
    // Initialize screen log
    WHBLogConsoleInit();
    WHBLogConsoleDraw();

    // Cemu requires AX to be initialized to update the mic
    // The real Wii U does NOT need this
    AXInit();

    const auto micSampleBuffer = (int16_t *) aligned_alloc(0x40, MAX_SAMPLES * sizeof(int16_t));

    if (!micSampleBuffer) {
        goto early_exit;
    }

    MICWorkMemory workMem = {.sampleMaxCount = MAX_SAMPLES, .sampleBuffer = micSampleBuffer};
    MICError error = MIC_ERROR_OK;
    const auto micHandle = MICInit(MIC_INSTANCE_0, 0, &workMem, &error);

    if (error != MIC_ERROR_OK) {
        WHBLogPrintf("Failed to initialize mic: %d", error);
        goto dealloc_work_mem;;
    }


    // Mic produces signed big-endian LPCM 16 samples
    const simple_wav_format_info formatInfo = {
        .channel_count = 1,
        .sample_rate = MIC_SAMPLE_RATE,
        .bits_per_sample = 16,
        .audio_format = SIMPLE_WAV_AUDIO_FMT_PCM
    };
    simple_wav_error wavError;
    const auto wav = simple_wav_wr_open(&formatInfo, "output.wav", &wavError);
    if (!wav) {
        WHBLogPrintf("Failed to create WAV file: %s", simple_wav_strerrorname(wavError));
        goto deinit_mic;
    }

    WHBLogPrint("Tap A to toggle recording");
    WHBLogConsoleDraw();

    VPADStatus vpadStatus;
    auto lastTime = OSGetTick();

    bool isMicOpen = false;
    while (WHBProcIsRunning()) {
        // Read input from gamepad
        const auto sampleCount = VPADRead(VPAD_CHAN_0, &vpadStatus, 1, nullptr);
        if (sampleCount == 0)
            continue;

        // Check for A press
        if (vpadStatus.trigger & VPAD_BUTTON_A) {
            if (!isMicOpen) {
                // Start recording
                error = MICOpen(micHandle);
                isMicOpen = error == MIC_ERROR_OK;
                if (!isMicOpen)
                    WHBLogPrintf("Failed to open mic: %d", error);
                else
                    WHBLogPrint("Opened mic");
            } else {
                // Stop recording
                MICClose(micHandle);
                isMicOpen = false;
                wavError = simple_wav_wr_flush(wav);
                if (wavError != SIMPLE_WAV_SUCCESS)
                    WHBLogPrintf("Failed to commit WAV to file: %s", simple_wav_strerrorname(wavError));
                else
                    WHBLogPrint("Closed mic and committed");
            }
            WHBLogConsoleDraw();
        }
        if (!isMicOpen)
            continue;

        const auto currentTime = OSGetTick();
        if ((currentTime - lastTime) < OSMicrosecondsToTicks(100))
            continue;
        lastTime = currentTime;

        MICStatus micStatus = {};
        error = MICGetStatus(micHandle, &micStatus);

        const auto endPos = (micStatus.bufferPos + micStatus.availableData);
        if (endPos > MAX_SAMPLES) {
            auto sampleCount = MAX_SAMPLES - micStatus.bufferPos;
            // MIC samples are big endian, and WAV only supports little-endian samples
            inplace_byteswap_16(micSampleBuffer + micStatus.bufferPos, sampleCount);
            simple_wav_wr_write_sample_bytes(wav, micSampleBuffer + micStatus.bufferPos, sampleCount * sizeof(int16_t),
                                          &wavError);

            sampleCount = endPos - MAX_SAMPLES;
            inplace_byteswap_16(micSampleBuffer, sampleCount);
            simple_wav_wr_write_sample_bytes(wav, micSampleBuffer, sampleCount * sizeof(int16_t), &wavError);
        } else {
            inplace_byteswap_16(micSampleBuffer + micStatus.bufferPos, micStatus.availableData);
            simple_wav_wr_write_sample_bytes(wav, micSampleBuffer + micStatus.bufferPos, micStatus.availableData * sizeof(int16_t),
                                          &wavError);
        }
        MICSetDataConsumed(micHandle, micStatus.availableData);

        if (wavError != SIMPLE_WAV_SUCCESS) {
            WHBLogPrintf("WAV Error: %s", simple_wav_strerrorname(wavError));
            goto close_mic;
        }
    }
    earlyExit = false;


close_mic:
    if (isMicOpen)
        MICClose(micHandle);

    simple_wav_wr_close(wav);

deinit_mic:
    MICUninit(micHandle);

dealloc_work_mem:
    free(micSampleBuffer);

early_exit:
    if (earlyExit) {
        WHBLogConsoleDraw();
        while (WHBProcIsRunning()) {
        }
    }
    AXQuit();
    WHBLogConsoleFree();
    WHBProcShutdown();
}

static void inplace_byteswap_16(int16_t *data, uint32_t size) {
    for (auto i = 0; i < size; ++i) {
        data[i] = bswap16(data[i]);
    }
}
