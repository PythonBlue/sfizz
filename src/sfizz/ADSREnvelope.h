// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "LeakDetector.h"
#include "Region.h"
#include "MidiState.h"
#include <absl/types/span.h>
namespace sfz {
/**
 * @brief Describe an attack/delay/sustain/release envelope that can
 * produce its coefficient in a blockwise manner for SIMD-type operations.
 */
class ADSREnvelope {
public:
    using Float = float;

    ADSREnvelope() = default;
    /**
     * @brief Resets the ADSR envelope given a Region, the current midi state, and a delay and
     * trigger velocity
     *
     * @param desc
     * @param region
     * @param state
     * @param delay
     * @param velocity
     */
    void reset(const EGDescription& desc, const Region& region, const MidiState& state, int delay, float velocity, float sampleRate) noexcept;
    /**
     * @brief Get the next value for the envelope
     *
     * @return Float
     */
    Float getNextValue() noexcept;
    /**
     * @brief Get a block of values for the envelope. This method tries hard to be efficient
     * and hopefully it is.
     *
     * @param output
     */
    void getBlock(absl::Span<Float> output) noexcept;
    /**
     * @brief Set the release time for the envelope
     *
     * @param timeInSeconds
     */
    void setReleaseTime(Float timeInSeconds) noexcept;
    /**
     * @brief Start the envelope release after a delay.
     *
     * @param releaseDelay the delay before releasing in samples
     */
    void startRelease(int releaseDelay) noexcept;
    /**
     * @brief Is the envelope smoothing?
     *
     * @return true
     * @return false
     */
    bool isSmoothing() const noexcept { return currentState != State::Done; }
    /**
     * @brief Is the envelope released?
     *
     * @return true
     * @return false
     */
    bool isReleased() const noexcept { return currentState == State::Release || shouldRelease; }
    /**
     * @brief Get the remaining delay samples
     *
     * @return int
     */
    int getRemainingDelay() const noexcept { return delay; }

private:
    float sampleRate { config::defaultSampleRate };
    Float secondsToSamples(Float timeInSeconds) const noexcept;
    Float secondsToLinRate(Float timeInSeconds) const noexcept;
    Float secondsToExpRate(Float timeInSeconds) const noexcept;

    enum class State {
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release,
        Done
    };
    State currentState { State::Done };
    Float currentValue { 0.0 };
    int delay { 0 };
    Float attackStep { 0 };
    Float decayRate { 0 };
    Float releaseRate { 0 };
    int hold { 0 };
    Float start { 0 };
    Float peak { 0 };
    Float sustain { 0 };
    Float sustainThreshold { config::virtuallyZero };
    int releaseDelay { 0 };
    bool shouldRelease { false };
    bool freeRunning { false };
    LEAK_DETECTOR(ADSREnvelope);
};

}
