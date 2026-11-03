#pragma once

#include <Arduino.h>
#include <ESPUI.h>
#include <ESPUIcontrol.h>

class _ESPUIcontrolMgr
{
public:
    typedef BasicControl ControlObject_t;

#ifdef ESP32
    SemaphoreHandle_t ControlsSemaphore = NULL;
#endif

    _ESPUIcontrolMgr();
    ~_ESPUIcontrolMgr() {}
    _ESPUIcontrolMgr(_ESPUIcontrolMgr const&) = delete;
    void operator=(_ESPUIcontrolMgr const&)  = delete;

    Control::ControlId_t addControl(Control::Type type,
                                    const char* label,
                                    const String& value,
                                    Control::Color color,
                                    Control::ControlId_t parentControl,
                                    bool visible,
                                    void (*callback)(BasicControl*, int, void*),
                                    void* userData = nullptr);

    Control::ControlId_t addControl(Control::Type type,
                                    const char* label,
                                    long value,
                                    Control::Color color,
                                    Control::ControlId_t parentControl,
                                    bool visible,
                                    void (*callback)(BasicControl*, int, void*),
                                    void* userData = nullptr);

    Control::ControlId_t addControl(Control::Type type,
                                    const char* label,
                                    const char* value,
                                    Control::Color color,
                                    Control::ControlId_t parentControl,
                                    bool visible,
                                    void (*callback)(BasicControl*, int, void*),
                                    void* userData = nullptr);

    bool removeControl(Control::ControlId_t id);
    uint16_t removeSelectOptions(Control::ControlId_t select_id, Control::ControlId_t skip_id = 0xFFFF);
    void RemoveToBeDeletedControls();

    BasicControl* getControl(Control::ControlId_t id);
    BasicControl* getControlNoLock(Control::ControlId_t id);
    Control::ControlId_t GetControlCount() const { return controlCount; }
    BasicControl* getFirstOptionId(Control::ControlId_t selector, long value);
    BasicControl* getNextOptionId(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id);

    BasicControl* getFirstOptionIdNoLock(Control::ControlId_t selector, long value);
    BasicControl* getNextOptionIdNoLock(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id);

    uint32_t prepareJSONChunk(uint16_t startindex,
                              JsonDocument& rootDoc,
                              bool InUpdateMode,
                              int32_t FragmentOffset,
                              uint32_t CurrentSyncID);

private:
    ControlObject_t* controls = nullptr;
    ControlObject_t* controlsTail = nullptr;
    Control::ControlId_t controlCount = 0;
    Control::ControlId_t idCounter = 0;

    void appendControlObject(ControlObject_t* NewControlObject);
    ControlObject_t* getControlObjectNoLock(Control::ControlId_t id);
    ControlObject_t* getFirstOptionIdObjectNoLock(Control::ControlId_t selector, long value);
    ControlObject_t* getNextOptionIdObjectNoLock(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id);
};

extern _ESPUIcontrolMgr ESPUIcontrolMgr;