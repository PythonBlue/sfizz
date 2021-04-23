#include "Defaults.h"

namespace sfz {

namespace Default {
constexpr auto uint32_t_max = std::numeric_limits<uint32_t>::max();
constexpr auto int32_t_max = std::numeric_limits<int32_t>::max();
constexpr auto float_max = std::numeric_limits<float>::max();

using FloatSpec = const OpcodeSpec<float>;
using Int8Spec = const OpcodeSpec<int8_t>;
using Int16Spec = const OpcodeSpec<int16_t>;
using Int32Spec = const OpcodeSpec<int32_t>;
using Int64Spec = const OpcodeSpec<int64_t>;
using UInt8Spec = const OpcodeSpec<uint8_t>;
using UInt16Spec = const OpcodeSpec<uint16_t>;
using UInt32Spec = const OpcodeSpec<uint32_t>;
using BoolSpec = const OpcodeSpec<bool>;
template <class Enum> using ESpec = const OpcodeSpec<Enum>;

FloatSpec delay { 0.0f, {0.0f, 100.0f}, kPermissiveBounds };
FloatSpec delayRandom { 0.0f, {0.0f, 100.0f}, kPermissiveBounds };
FloatSpec delayMod { 0.0f, {0.0f, 100.0f}, kPermissiveBounds };
Int64Spec offset { 0, {0, int32_t_max}, kPermissiveBounds };
Int64Spec offsetMod { 0, {0, int32_t_max}, kPermissiveBounds };
Int64Spec offsetRandom { 0, {0, int32_t_max}, kPermissiveBounds };
Int64Spec sampleEnd { int32_t_max, {0, int32_t_max}, kEnforceBounds };
Int64Spec sampleEndMod { 0, {-int32_t_max, int32_t_max}, kPermissiveBounds };
UInt32Spec sampleCount { 0, {0, uint32_t_max}, kEnforceUpperBound };
Int64Spec loopStart { 0, {0, int32_t_max}, kEnforceUpperBound };
Int64Spec loopEnd { int32_t_max, {0, int32_t_max}, kEnforceUpperBound };
Int64Spec loopMod { 0, {-int32_t_max, int32_t_max}, kPermissiveBounds };
UInt32Spec loopCount { 0, {0, uint32_t_max}, kEnforceUpperBound };
FloatSpec loopCrossfade { 1e-3, {1e-3, 1.0f}, kEnforceLowerBound|kPermissiveUpperBound };
ESpec<OscillatorEnabled> oscillator { OscillatorEnabled::Auto, {OscillatorEnabled::Auto, OscillatorEnabled::On}, 0 };
FloatSpec oscillatorPhase { 0.0f, {-1000.0f, 1000.0f}, kPermissiveBounds };
Int32Spec oscillatorMode { 0, {0, 2}, 0 };
Int32Spec oscillatorMulti { 1, {1, config::oscillatorsPerVoice}, kEnforceBounds };
FloatSpec oscillatorDetune { 0.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
FloatSpec oscillatorDetuneMod { 0.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
FloatSpec oscillatorModDepth { 0.0f, {0.0f, 10000.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec oscillatorModDepthMod { 0.0f, {0.0f, 10000.0f}, kNormalizePercent|kPermissiveBounds };
Int32Spec oscillatorQuality { 1, {0, 3}, 0 };
UInt32Spec group { 0, {0, uint32_t_max}, 0 };
FloatSpec offTime { 6e-3f, {0.0f, 100.0f}, kPermissiveBounds };
UInt32Spec polyphony { config::maxVoices, {0, config::maxVoices}, kEnforceBounds };
UInt32Spec notePolyphony { config::maxVoices, {0, config::maxVoices}, kEnforceBounds };
UInt8Spec key { 60, {0, 127}, kCanBeNote };
UInt8Spec loKey { 0, {0, 127}, kCanBeNote };
UInt8Spec hiKey { 127, {0, 127}, kCanBeNote };
FloatSpec loCC { 0, {0.0f, 127.0f}, kNormalizeMidi|kPermissiveBounds };
FloatSpec hiCC { 127, {0.0f, 127.0f}, kNormalizeMidi|kPermissiveBounds };
FloatSpec loVel { 0, {0.0f, 127.0f}, kNormalizeMidi|kPermissiveBounds };
FloatSpec hiVel { 127, {0.0f, 127.0f}, kNormalizeMidi|kPermissiveBounds };
FloatSpec loChannelAftertouch { 0, {0, 127}, kNormalizeMidi|kPermissiveBounds };
FloatSpec hiChannelAftertouch { 127, {0, 127}, kNormalizeMidi|kPermissiveBounds };
FloatSpec loPolyAftertouch { 0, {0, 127}, kNormalizeMidi|kPermissiveBounds };
FloatSpec hiPolyAftertouch { 127, {0, 127}, kNormalizeMidi|kPermissiveBounds };
FloatSpec loBend { -8191, {-8192.0f, 8191.0f}, kNormalizeBend|kPermissiveBounds };
FloatSpec hiBend { 8191, {-8192.0f, 8191.0f}, kNormalizeBend|kPermissiveBounds };
FloatSpec loNormalized { 0.0f, {0.0f, 1.0f}, kPermissiveBounds };
FloatSpec hiNormalized { 1.0f, {0.0f, 1.0f}, kPermissiveBounds };
FloatSpec loBipolar { -1.0f, {-1.0f, 1.0f}, kPermissiveBounds };
FloatSpec hiBipolar { 1.0f, {-1.0f, 1.0f}, kPermissiveBounds };
UInt16Spec ccNumber { 0, {0, config::numCCs}, 0 };
UInt16Spec smoothCC { 0, {0, 100}, kPermissiveUpperBound };
UInt8Spec curveCC { 0, {0, 255}, 0 };
UInt8Spec sustainCC { 64, {0, 127}, 0 };
UInt8Spec sostenutoCC { 66, {0, 127}, 0 };
FloatSpec sustainThreshold { 1.0f, {0.0f, 127.0f}, kNormalizeMidi|kPermissiveBounds };
FloatSpec sostenutoThreshold { 1.0f, {0.0f, 127.0f}, kNormalizeMidi|kPermissiveBounds };
BoolSpec checkSustain { true, {0, 1}, kEnforceBounds };
BoolSpec checkSostenuto { true, {0, 1}, kEnforceBounds };
FloatSpec loBPM { 0.0f, {0.0f, 500.0f}, kPermissiveBounds };
FloatSpec hiBPM { 500.0f, {0.0f, 500.0f}, kPermissiveBounds };
UInt8Spec sequence { 1, {1, 100}, 0 };
FloatSpec volume { 0.0f, {-144.0f, 48.0f}, kPermissiveBounds };
FloatSpec volumeMod { 0.0f, {-144.0f, 48.0f}, kPermissiveBounds };
FloatSpec amplitude { 100.0f, {0.0f, 10000.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec amplitudeMod { 0.0f, {0.0f, 10000.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec pan { 0.0f, {-100.0f, 100.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec panMod { 0.0f, {-200.0f, 200.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec position { 0.0f, {-100.0f, 100.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec positionMod { 0.0f, {-200.0f, 200.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec width { 100.0f, {-100.0f, 100.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec widthMod { 0.0f, {-200.0f, 200.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec ampKeytrack { 0.0f, {-96.0f, 12.0f}, kPermissiveBounds };
FloatSpec ampVeltrack { 100.0f, {-100.0f, 100.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec ampVelcurve { 0.0f, {0.0f, 1.0f}, kPermissiveBounds };
FloatSpec ampRandom { 0.0f, {-24.0f, 24.0f}, kPermissiveBounds };
BoolSpec rtDead { false, {false, true}, kEnforceBounds };
FloatSpec rtDecay { 0.0f, {0.0f, 200.0f}, kEnforceLowerBound|kPermissiveUpperBound };
FloatSpec filterCutoff { 0.0f, {0.0f, 20000.0f}, kPermissiveBounds };
FloatSpec filterCutoffMod { 0.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
FloatSpec filterResonance { 0.0f, {0.0f, 96.0f}, kPermissiveBounds };
FloatSpec filterResonanceMod { 0.0f, {0.0f, 96.0f}, kPermissiveBounds };
FloatSpec filterGain { 0.0f, {-96.0f, 96.0f}, kPermissiveBounds };
FloatSpec filterGainMod { 0.0f, {-96.0f, 96.0f}, kPermissiveBounds };
FloatSpec filterRandom { 0.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
FloatSpec filterKeytrack { 0, {0, 1200}, kPermissiveBounds };
FloatSpec filterVeltrack { 0, {-12000, 12000}, kPermissiveBounds };
FloatSpec eqBandwidth { 1.0f, {0.001f, 4.0f}, kPermissiveBounds };
FloatSpec eqBandwidthMod { 0.0f, {-4.0f, 4.0f}, kPermissiveBounds };
FloatSpec eqFrequency { 0.0f, {0.0f, 20000.0f}, kPermissiveBounds };
FloatSpec eqFrequencyMod { 0.0f, {-20000.0f, 20000.0f}, kPermissiveBounds };
FloatSpec eqGain { 0.0f, {-96.0f, 96.0f}, kPermissiveBounds };
FloatSpec eqGainMod { 0.0f, {-96.0f, 96.0f}, kPermissiveBounds };
FloatSpec eqVel2Frequency { 0.0f, {-30000.0f, 30000.0f}, kPermissiveBounds };
FloatSpec eqVel2Gain { 0.0f, {-96.0f, 96.0f}, kPermissiveBounds };
FloatSpec pitchKeytrack { 100, {-1200, 1200}, kPermissiveBounds };
FloatSpec pitchRandom { 0.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
FloatSpec pitchVeltrack { 0, {-12000, 12000}, kPermissiveBounds };
FloatSpec transpose { 0, {-127, 127}, kPermissiveBounds };
FloatSpec pitch { 0.0f, {-2400.0f, 2400.0f}, kPermissiveBounds };
FloatSpec pitchMod { 0.0f, {-9600.0f, 9600.0f}, kPermissiveBounds };
FloatSpec bendUp { 200.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
FloatSpec bendDown { -200.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
FloatSpec bendStep { 1.0f, {1.0f, 1200.0f}, kEnforceLowerBound|kPermissiveBounds };
FloatSpec ampLFODepth { 0.0f, {-10.0f, 10.0f}, kPermissiveBounds };
FloatSpec pitchLFODepth { 0.0f, {-1200.0f, 1200.0f}, kPermissiveBounds };
FloatSpec filLFODepth { 0.0f, {-1200.0f, 1200.0f}, kPermissiveBounds };
FloatSpec lfoFreq { 0.0f, {0.0f, 100.0f}, kPermissiveBounds };
FloatSpec lfoFreqMod { 0.0f, {-100.0f, 100.0f}, kPermissiveBounds };
FloatSpec lfoBeats { 0.0f, {0.0f, 1000.0f}, kPermissiveBounds };
FloatSpec lfoBeatsMod { 0.0f, {-1000.0f, 1000.0f}, kPermissiveBounds };
FloatSpec lfoPhase { 0.0f, {0.0f, 1.0f}, kWrapPhase|kPermissiveBounds };
FloatSpec lfoPhaseMod { 0.0f, {0.0f, 1.0f}, kPermissiveBounds };
FloatSpec lfoDelay { 0.0f, {0.0f, 30.0f}, kPermissiveBounds };
FloatSpec lfoDelayMod { 0.0f, {0.0f, 30.0f}, kPermissiveBounds };
FloatSpec lfoFade { 0.0f, {0.0f, 30.0f}, kPermissiveBounds };
FloatSpec lfoFadeMod { 0.0f, {0.0f, 30.0f}, kPermissiveBounds };
UInt32Spec lfoCount { 0, {0, 1000}, kEnforceLowerBound|kPermissiveUpperBound };
UInt32Spec lfoSteps { 0, {0, static_cast<unsigned>(config::maxLFOSteps)}, kEnforceBounds };
FloatSpec lfoStepX { 0.0f, {-100.0f, 100.0f}, kNormalizePercent|kPermissiveBounds };
ESpec<LFOWave> lfoWave { LFOWave::Triangle, {LFOWave::Triangle, LFOWave::RandomSH}, 0 };
FloatSpec lfoOffset { 0.0f, {-1.0f, 1.0f}, kPermissiveBounds };
FloatSpec lfoRatio { 1.0f, {0.0f, 100.0f}, kPermissiveBounds };
FloatSpec lfoScale { 1.0f, {0.0f, 1.0f}, kPermissiveBounds };
FloatSpec egTime { 0.0f, {0.0f, 100.0f}, kPermissiveBounds };
FloatSpec egRelease { 0.001f, {0.0f, 100.0f}, kPermissiveBounds };
FloatSpec egTimeMod { 0.0f, {-100.0f, 100.0f}, kPermissiveBounds };
FloatSpec egSustain { 100.0f, {0.0f, 100.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec egPercent { 0.0f, {0.0f, 100.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec egPercentMod { 0.0f, {-100.0f, 100.0f}, kNormalizePercent|kPermissiveBounds };
FloatSpec egDepth { 0.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
FloatSpec egVel2Depth { 0.0f, {-12000.0f, 12000.0f}, kPermissiveBounds };
BoolSpec flexEGAmpeg { false, {0, 1}, kEnforceBounds };
BoolSpec flexEGDynamic { 0, {0, 1}, kEnforceBounds };
Int32Spec flexEGSustain { 0, {0, 100}, kEnforceLowerBound|kPermissiveUpperBound };
FloatSpec flexEGPointTime { 0.0f, {0.0f, 100.0f}, kPermissiveBounds };
FloatSpec flexEGPointLevel { 0.0f, {-1.0f, 1.0f}, kPermissiveBounds };
FloatSpec flexEGPointShape { 0.0f, {-100.0f, 100.0f}, kPermissiveBounds };
Int32Spec sampleQuality { 2, {0, 10}, 0 };
Int32Spec octaveOffset { 0, {-10, 10}, kPermissiveBounds };
Int32Spec noteOffset { 0, {-127, 127}, kPermissiveBounds };

// TODO: effect opcode flags
FloatSpec effect { 0.0f, {0.0f, 100.0f}, kNormalizePercent };
FloatSpec effectPercent { 0.0f, {0.0f, 100.0f}, 0 };
ESpec<LFOWave> apanWaveform { LFOWave::Triangle, {LFOWave::Triangle, LFOWave::Saw}, 0 };
FloatSpec apanFrequency { 0.0f, {0.0f, float_max}, 0 };
FloatSpec apanPhase { 0.5f, {0.0f, 1.0f}, kWrapPhase };
FloatSpec apanLevel { 0.0f, {0.0f, 100.0f}, kNormalizePercent };
FloatSpec distoTone { 100.0f, {0.0f, 100.0f}, 0 };
FloatSpec distoDepth { 0.0f, {0.0f, 100.0f}, 0 };
UInt32Spec distoStages { 1, {1, maxDistoStages}, 0 };
FloatSpec compAttack { 0.005f, {0.0f, 10.0f}, 0 };
FloatSpec compRelease { 0.05f, {0.0f, 10.0f}, 0 };
BoolSpec compSTLink { false, {0, 1}, 0 };
FloatSpec compThreshold { 0.0f, {-100.0f, 0.0f}, 0 };
FloatSpec compRatio { 1.0f, {1.0f, 50.0f}, 0 };
FloatSpec compGain { 0.0f, {-100.0f, 100.0f}, kDb2Mag };
FloatSpec fverbSize { 0.0f, {0.0f, 100.0f}, 0 };
FloatSpec fverbPredelay { 0.0f, {0.0f, 10.0f}, 0 };
FloatSpec fverbTone { 100.0f, {0.0f, 100.0f}, 0 };
FloatSpec fverbDamp { 0.0f, {0.0f, 100.0f}, 0 };
BoolSpec gateSTLink { false, {0, 1}, 0 };
FloatSpec gateAttack { 0.005f, {0.0f, 10.0f}, 0 };
FloatSpec gateRelease { 0.05f, {0.0f, 10.0f}, 0 };
FloatSpec gateHold { 0.0f, {0.0f, 10.0f}, 0 };
FloatSpec gateThreshold { 0.0f, {-100.0f, 0.0f}, 0 };
FloatSpec lofiBitred { 0.0f, {0.0f, 100.0f}, 0 };
FloatSpec lofiDecim { 0.0f, {0.0f, 100.0f}, 0 };
FloatSpec rectify { 0.0f, {0.0f, 100.0f}, 0 };
UInt32Spec stringsNumber { maxStrings, {0, maxStrings}, 0 };

ESpec<Trigger> trigger { Trigger::attack, {Trigger::attack, Trigger::release_key}, 0};
ESpec<CrossfadeCurve> crossfadeCurve { CrossfadeCurve::power, {CrossfadeCurve::gain, CrossfadeCurve::power}, 0};
ESpec<OffMode> offMode { OffMode::fast, {OffMode::fast, OffMode::time}, 0};
ESpec<LoopMode> loopMode { LoopMode::no_loop, {LoopMode::no_loop, LoopMode::loop_sustain}, 0};
ESpec<VelocityOverride> velocityOverride { VelocityOverride::current, {VelocityOverride::current, VelocityOverride::previous}, 0};
ESpec<SelfMask> selfMask { SelfMask::mask, {SelfMask::mask, SelfMask::dontMask}, 0};
ESpec<FilterType> filter { FilterType::kFilterNone, {FilterType::kFilterNone, FilterType::kFilterPeq}, 0};
ESpec<EqType> eq { EqType::kEqNone, {EqType::kEqNone, EqType::kEqHshelf}, 0};
} // namespace Default

} // namespace sfz
