// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstController.h"
#include "SfizzVstEditor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

tresult PLUGIN_API SfizzVstControllerNoUi::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultTrue)
        return result;

    Vst::ParamID pid = 0;

    // Ordinary parameters
    parameters.addParameter(
        kParamVolumeRange.createParameter(
            Steinberg::String("Volume"), pid++, Steinberg::String("dB"),
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamNumVoicesRange.createParameter(
            Steinberg::String("Polyphony"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamOversamplingRange.createParameter(
            Steinberg::String("Oversampling"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamPreloadSizeRange.createParameter(
            Steinberg::String("Preload size"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamScalaRootKeyRange.createParameter(
            Steinberg::String("Scala root key"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamTuningFrequencyRange.createParameter(
            Steinberg::String("Tuning frequency"), pid++, Steinberg::String("Hz"),
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamStretchedTuningRange.createParameter(
            Steinberg::String("Stretched tuning"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));

    // MIDI special controllers
    parameters.addParameter(Steinberg::String("Aftertouch"), nullptr, 0, 0.5, 0, pid++, Vst::kRootUnitId);
    parameters.addParameter(Steinberg::String("Pitch bend"), nullptr, 0, 0.5, 0, pid++, Vst::kRootUnitId);

    // MIDI controllers
    for (unsigned i = 0; i < kNumControllerParams; ++i) {
        Steinberg::String title;
        Steinberg::String shortTitle;
        title.printf("Controller %u", i);
        shortTitle.printf("CC%u", i);

        parameters.addParameter(
            title, nullptr, 0, 0, Vst::ParameterInfo::kNoFlags,
            pid++, Vst::kRootUnitId, shortTitle);
    }

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstControllerNoUi::terminate()
{
    return EditController::terminate();
}

tresult PLUGIN_API SfizzVstControllerNoUi::getMidiControllerAssignment(int32 busIndex, int16 channel, Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id)
{
    switch (midiControllerNumber) {
    case Vst::kAfterTouch:
        id = kPidMidiAftertouch;
        return kResultTrue;

    case Vst::kPitchBend:
        id = kPidMidiPitchBend;
        return kResultTrue;

    default:
        if (midiControllerNumber < 0 || midiControllerNumber >= kNumControllerParams)
            return kResultFalse;

        id = kPidMidiCC0 + midiControllerNumber;
        return kResultTrue;
    }
}

tresult PLUGIN_API SfizzVstControllerNoUi::getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
    switch (tag) {
    case kPidOversampling:
        {
            auto factorLog2 = static_cast<int>(kParamOversamplingRange.denormalize(valueNormalized));
            Steinberg::String buf;
            buf.printf("%dX", 1 << factorLog2);
            buf.copyTo(string);
            return kResultTrue;
        }
    }

    return EditController::getParamStringByValue(tag, valueNormalized, string);
}

tresult PLUGIN_API SfizzVstControllerNoUi::getParamValueByString(Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
    switch (tag) {
    case kPidOversampling:
        {
            int32 factor;
            if (!Steinberg::String::scanInt32(string, factor, false) || factor < 1)
                factor = 1;

            int32 log2Factor = 0;
            for (int32 f = factor; f > 1; f /= 2)
                ++log2Factor;

            valueNormalized = kParamOversamplingRange.normalize(log2Factor);
            return kResultTrue;
        }
    }

    return EditController::getParamValueByString(tag, string, valueNormalized);
}

// --- Controller with UI --- //

IPlugView* PLUGIN_API SfizzVstController::createView(FIDString _name)
{
    ConstString name(_name);

    fprintf(stderr, "[sfizz] about to create view: %s\n", _name);

    if (name != Vst::ViewType::kEditor)
        return nullptr;

    if (IPtr<SfizzVstEditor> editor = _editor.lock()) {
        withStateLock([this, editor]() {
            _uiState = editor->getCurrentUiState();
        });
    }

    IPtr<SfizzVstEditor> editor = Steinberg::owned(new SfizzVstEditor(this));
    _editor = editor->getWeakPtr();

    withStateLock([this, editor]() {
        editor->updateState(_state);
        editor->updateUiState(_uiState);
        editor->updatePlayState(_playState);
    });

    editor->remember();
    return editor;
}

tresult PLUGIN_API SfizzVstController::setParamNormalized(Vst::ParamID tag, Vst::ParamValue normValue)
{
    tresult r = SfizzVstControllerNoUi::setParamNormalized(tag, normValue);
    if (r != kResultTrue)
        return r;

    float *slotF32 = nullptr;
    int32 *slotI32 = nullptr;
    float value = 0;

    switch (tag) {
    case kPidVolume: {
        slotF32 = &_state.volume;
        value = kParamVolumeRange.denormalize(normValue);
        break;
    }
    case kPidNumVoices: {
        slotI32 = &_state.numVoices;
        value = kParamNumVoicesRange.denormalize(normValue);
        break;
    }
    case kPidOversampling: {
        slotI32 = &_state.oversamplingLog2;
        value = kParamOversamplingRange.denormalize(normValue);
        break;
    }
    case kPidPreloadSize: {
        slotI32 = &_state.preloadSize;
        value = kParamPreloadSizeRange.denormalize(normValue);
        break;
    }
    case kPidScalaRootKey: {
        slotI32 = &_state.scalaRootKey;
        value = kParamScalaRootKeyRange.denormalize(normValue);
        break;
    }
    case kPidTuningFrequency: {
        slotF32 = &_state.tuningFrequency;
        value = kParamTuningFrequencyRange.denormalize(normValue);
        break;
    }
    case kPidStretchedTuning: {
        slotF32 = &_state.stretchedTuning;
        value = kParamStretchedTuningRange.denormalize(normValue);
        break;
    }
    }

    if (slotF32 && *slotF32 != value) {
        withStateLock([this, slotF32, value]() {
            *slotF32 = value;
            if (IPtr<SfizzVstEditor> editor = _editor.lock())
                editor->updateState(_state);
        });
    }
    else if (slotI32 && *slotI32 != (int32)value) {
        withStateLock([this, slotI32, value]() {
            *slotI32 = (int32)value;
            if (IPtr<SfizzVstEditor> editor = _editor.lock())
                editor->updateState(_state);
        });
    }

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstController::setState(IBStream* stream)
{
    SfizzUiState s;

    tresult r = s.load(stream);
    if (r != kResultTrue)
        return r;

    withStateLock([this, &s]() {
        _uiState = s;
        if (IPtr<SfizzVstEditor> editor = _editor.lock())
            editor->updateUiState(_uiState);
    });

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstController::getState(IBStream* stream)
{
    tresult result;

    withStateLock([this, stream, &result]() {
        if (IPtr<SfizzVstEditor> editor = _editor.lock())
            _uiState = editor->getCurrentUiState();
        result = _uiState.store(stream);
    });

    return result;
}

tresult PLUGIN_API SfizzVstController::setComponentState(IBStream* stream)
{
    SfizzVstState s;

    tresult r = s.load(stream);
    if (r != kResultTrue)
        return r;

    setParamNormalized(kPidVolume, kParamVolumeRange.normalize(s.volume));
    setParamNormalized(kPidNumVoices, kParamNumVoicesRange.normalize(s.numVoices));
    setParamNormalized(kPidOversampling, kParamOversamplingRange.normalize(s.oversamplingLog2));
    setParamNormalized(kPidPreloadSize, kParamPreloadSizeRange.normalize(s.preloadSize));
    setParamNormalized(kPidScalaRootKey, kParamScalaRootKeyRange.normalize(s.scalaRootKey));
    setParamNormalized(kPidTuningFrequency, kParamTuningFrequencyRange.normalize(s.tuningFrequency));
    setParamNormalized(kPidStretchedTuning, kParamStretchedTuningRange.normalize(s.stretchedTuning));

    withStateLock([this, &s]() {
        _state = s;
        if (IPtr<SfizzVstEditor> editor = _editor.lock())
            editor->updateState(_state);
    });

    return kResultTrue;
}

tresult SfizzVstController::notify(Vst::IMessage* message)
{
    // Note: may be called from any thread (Reaper)

    tresult result = SfizzVstControllerNoUi::notify(message);
    if (result != kResultFalse)
        return result;

    const char* id = message->getMessageID();
    Vst::IAttributeList* attr = message->getAttributes();

    if (!strcmp(id, "LoadedSfz")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        withStateLock([this, data, size]() {
            _state.sfzFile.assign(static_cast<const char *>(data), size);
            if (IPtr<SfizzVstEditor> editor = _editor.lock())
                editor->updateState(_state);
        });
    }
    else if (!strcmp(id, "LoadedScala")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        withStateLock([this, data, size]() {
            _state.scalaFile.assign(static_cast<const char *>(data), size);
            if (IPtr<SfizzVstEditor> editor = _editor.lock())
                editor->updateState(_state);
        });
    }
    else if (!strcmp(id, "NotifiedPlayState")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("PlayState", data, size);

        if (result != kResultTrue)
            return result;

        withStateLock([this, data]() {
            _playState = *static_cast<const SfizzPlayState*>(data);
            if (IPtr<SfizzVstEditor> editor = _editor.lock())
                editor->updatePlayState(_playState);
        });
    }
    else if (!strcmp(id, "ReceivedMessage")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("Message", data, size);

        if (result != kResultTrue)
            return result;

        if (IPtr<SfizzVstEditor> editor = _editor.lock())
            editor->receiveMessage(data, size);
    }

    return result;
}

FUnknown* SfizzVstController::createInstance(void*)
{
    return static_cast<Vst::IEditController*>(new SfizzVstController);
}

/*
  Note(jpc) Generated at random with uuidgen.
  Can't find docs on it... maybe it's to register somewhere?
 */
FUID SfizzVstController::cid(0x7129736c, 0xbc784134, 0xbb899d56, 0x2ebafe4f);
