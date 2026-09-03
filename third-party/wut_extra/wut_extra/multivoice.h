#pragma once
#include <sndcore2/voice.h>

#ifdef __cplusplus
extern "C" {



#endif

typedef struct AXMultiVoice AXMultiVoice;
typedef struct AXMultiVoiceAcquireInfo AXMultiVoiceAcquireInfo;

struct AXMultiVoiceAcquireInfo {
    WUT_PADDING_BYTES(0x4a);
    //! Number of audio channels
    uint16_t channelCount;
};

struct AXMultiVoice {
    BOOL inUse;
    uint32_t channelCount;
    AXVoice *voices[6];
};

/**
 * Acquire a group of voices for playing multi-channel audio
 * \param voicePriority priority of sub-voices, min 0, max 31
 * \param callback function to call when voices are dropped
 * \param userCtx
 * \param acquireInfo
 * \param [out] outMultiVoice returned voice
 * \return 0 - success
 * \return -20 - too many multi-voices acquired (max 96)
 * \return -21 - channel count > 6
 * \return -22 - failed to acquire regular voice
 * \see AXAcquireVoice
 * \see AXFreeMultiVoice
 */
int32_t AXAcquireMultiVoice(int32_t voicePriority,
                            AXVoiceCallbackExFn *callback,
                            void *userCtx,
                            const AXMultiVoiceAcquireInfo *acquireInfo,
                            AXMultiVoice **outMultiVoice);

/**
 * Release a multi-voice
 * \param multiVoice voice group to release
 * \note freeing a voice allows it to reused in a later acquire
 */
void AXFreeMultiVoice(AXMultiVoice *multiVoice);

/**
 * Set data and sample offsets for the sub-voices
 *
 * \param multiVoice
 * \param offsets array of voice offsets of length equal to channel count
 * \note Make sure the offsets are set so that all sub-voices play for the same length of time
 */
void AXSetMultiVoiceOffsets(AXMultiVoice *multiVoice,
                            const AXVoiceOffsets *offsets);

//! \see AXSetVoiceState
void AXSetMultiVoiceState(AXMultiVoice *multiVoice,
                          enum AX_VOICE_STATE STATE);

/**
 * Returns whether the multi-voice is playing
 * \note Strictly speaking, it only checks whether the first sub-voice is playing
 * \see AXIsVoiceRunning
 */
BOOL AXIsMultiVoiceRunning(AXMultiVoice *multiVoice);

/**
 * Set voice volume data
 * \note Strictly speaking, it only checks whether the first voice is playing
 * \see AXSetVoiceVe
 */
void AXSetMultiVoiceVe(AXMultiVoice *multiVoice,
                       const AXVoiceVeData *ve);

/**
 * Set multi-voice volume mix
 * \param multiVoice
 * \param deviceType type of target device
 * \param deviceIndex the index of the target device
 * \param busIndex the audio bus to play to
 * \param vol volume of the voice on device
 * \param delta change in volume per sample on the device
 */
void AXSetMultiVoiceDeviceMix(AXMultiVoice *multiVoice,
                              AXDeviceType deviceType,
                              uint32_t deviceIndex,
                              uint32_t busIndex,
                              uint16_t vol,
                              int16_t delta);

/**
 * Set the sample rate conversion type for all sub-voices
 * \param multiVoice voice group to modify
 * \param srcType
 */
void AXSetMultiVoiceSrcType(AXMultiVoice *multiVoice,
                            enum AX_VOICE_SRC_TYPE srcType);

/**
 * Set the sample rate conversion ratio for all sub-voices, interpreted according to the sample ratio conversion type
 * \param multiVoice voice group to modify
 * \param ratio conversion ratio
 * \returns 0
 * \see AXSetMultiVoiceSrcType
 * \see AXSetVoiceSrcRatio
 */
int32_t
AXSetMultiVoiceSrcRatio(AXMultiVoice *multiVoice,
                        float ratio);

#ifdef __cplusplus
}
#endif
