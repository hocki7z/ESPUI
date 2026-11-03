#include "ESPUIcontrolMgr.h"

_ESPUIcontrolMgr::_ESPUIcontrolMgr()
{
#ifdef ESP32
    ControlsSemaphore = xSemaphoreCreateMutex();
    xSemaphoreGive(ControlsSemaphore);
#endif
}

void _ESPUIcontrolMgr::RemoveToBeDeletedControls()
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif

    ControlObject_t* PreviousControl = nullptr;
    ControlObject_t* CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        ControlObject_t* NextControl = CurrentControl->next;
        if (CurrentControl->ToBeDeleted())
        {
            if (CurrentControl == controls)
            {
                controls = NextControl;
            }
            else
            {
                PreviousControl->next = NextControl;
            }

            if (CurrentControl == controlsTail)
            {
                controlsTail = PreviousControl;
            }

            delete CurrentControl; // Virtual destructor frees dyn_str automatically
            CurrentControl = NextControl;
        }
        else
        {
            PreviousControl = CurrentControl;
            CurrentControl = NextControl;
        }
    }

#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif
}

BasicControl* _ESPUIcontrolMgr::getControl(Control::ControlId_t id)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif
    BasicControl* Response = getControlNoLock(id);
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif
    return Response;
}

BasicControl* _ESPUIcontrolMgr::getControlNoLock(Control::ControlId_t id)
{
    return getControlObjectNoLock(id);
}

_ESPUIcontrolMgr::ControlObject_t* _ESPUIcontrolMgr::getControlObjectNoLock(Control::ControlId_t id)
{
    ControlObject_t* CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        if (CurrentControl->GetId() == id)
        {
            return CurrentControl->ToBeDeleted() ? nullptr : CurrentControl;
        }
        CurrentControl = CurrentControl->next;
    }

    return nullptr;
}

BasicControl* _ESPUIcontrolMgr::getFirstOptionId(Control::ControlId_t selector, long value)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif
    BasicControl* Response = getFirstOptionIdNoLock(selector, value);
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif
    return Response;
}

BasicControl* _ESPUIcontrolMgr::getFirstOptionIdNoLock(Control::ControlId_t selector, long value)
{
    return getFirstOptionIdObjectNoLock(selector, value);
}

_ESPUIcontrolMgr::ControlObject_t* _ESPUIcontrolMgr::getFirstOptionIdObjectNoLock(Control::ControlId_t selector, long value)
{
    ControlObject_t* CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        if ((CurrentControl->parentControl == selector) &&
            (CurrentControl->type == Control::Type::Option) &&
            (CurrentControl->control_flags & CONTROL_FLAG_NUMERIC) &&
            ((value == -4) || (CurrentControl->numeric_value == value)))
        {
            return CurrentControl->ToBeDeleted() ? nullptr : CurrentControl;
        }
        CurrentControl = CurrentControl->next;
    }

    return nullptr;
}

BasicControl* _ESPUIcontrolMgr::getNextOptionId(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif
    BasicControl* Response = getNextOptionIdNoLock(selector, value, prev_option_id);
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif
    return Response;
}

BasicControl* _ESPUIcontrolMgr::getNextOptionIdNoLock(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id)
{
    return getNextOptionIdObjectNoLock(selector, value, prev_option_id);
}

_ESPUIcontrolMgr::ControlObject_t* _ESPUIcontrolMgr::getNextOptionIdObjectNoLock(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id)
{
    ControlObject_t* CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        if ((CurrentControl->parentControl == selector) &&
            (CurrentControl->type == Control::Type::Option) &&
            (CurrentControl->control_flags & CONTROL_FLAG_NUMERIC) &&
            ((value == -4) || (CurrentControl->numeric_value == value)) &&
            (CurrentControl->id > prev_option_id))
        {
            return CurrentControl->ToBeDeleted() ? nullptr : CurrentControl;
        }
        CurrentControl = CurrentControl->next;
    }

    return nullptr;
}

bool _ESPUIcontrolMgr::removeControl(Control::ControlId_t id)
{
    bool Response = false;

#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif
    BasicControl* control = getControlNoLock(id);
    if (control)
    {
        Response = true;
        control->DeleteControl();
        controlCount--;
    }
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif

    return Response;
}

uint16_t _ESPUIcontrolMgr::removeSelectOptions(Control::ControlId_t select_id, Control::ControlId_t skip_id)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif

    uint16_t Response = 0;
    ControlObject_t* CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        if ((CurrentControl->parentControl == select_id) &&
            (CurrentControl->GetType() == Control::Type::Option) &&
            (CurrentControl->GetId() != skip_id))
        {
            CurrentControl->numeric_value = -2;
            CurrentControl->SetControlChangedId(ESPUI.GetNextControlChangeId());
            Response++;
        }
        CurrentControl = CurrentControl->next;
    }

#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif

    return Response;
}

uint32_t _ESPUIcontrolMgr::prepareJSONChunk(uint16_t startindex,
                                            JsonDocument& rootDoc,
                                            bool InUpdateMode,
                                            int32_t FragmentOffset,
                                            uint32_t CurrentSyncID)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif

    int elementcount = 0;
    uint32_t MaxMarshaledJsonSize = (!InUpdateMode) ? ESPUI.jsonInitialDocumentSize : ESPUI.jsonUpdateDocumentSize;
    uint32_t EstimatedUsedMarshaledJsonSize = 0;

    do // once
    {
        ControlObject_t* CurrentControlObject = controls;
        uint32_t currentIndex = 0;
        uint32_t DataOffset = 0;
        JsonArray items = rootDoc[F("controls")];
        bool SingleControl = false;

        if (FragmentOffset >= 0)
        {
            // FragmentOffset >= 0 means startindex holds the target ControlId
            CurrentControlObject = getControlObjectNoLock(startindex);
            if (nullptr == CurrentControlObject)
            {
                break;
            }

            DataOffset = (uint32_t)FragmentOffset;
            currentIndex = 1;
            startindex = 0;
            SingleControl = true;
        }

        // Find the starting control to send
        while ((startindex > currentIndex) && (nullptr != CurrentControlObject))
        {
            if ((!CurrentControlObject->ToBeDeleted()) && (!CurrentControlObject->SkipSend()))
            {
                if (InUpdateMode)
                {
                    if (CurrentControlObject->NeedsSync(CurrentSyncID))
                    {
                        ++currentIndex;
                    }
                }
                else
                {
                    ++currentIndex;
                }
            }
            CurrentControlObject = CurrentControlObject->next;
        }

        if (nullptr == CurrentControlObject)
        {
            break;
        }

        // Serialize controls into this chunk message
        while (nullptr != CurrentControlObject)
        {
            if ((CurrentControlObject->ToBeDeleted() || CurrentControlObject->SkipSend()) && !SingleControl)
            {
                CurrentControlObject = CurrentControlObject->next;
                continue;
            }

            if (InUpdateMode && !SingleControl)
            {
                if (!CurrentControlObject->NeedsSync(CurrentSyncID))
                {
                    CurrentControlObject = CurrentControlObject->next;
                    continue;
                }
            }

            JsonObject item = AllocateJsonObject(items);
            elementcount++;

            uint32_t RemainingSpace = (MaxMarshaledJsonSize > (EstimatedUsedMarshaledJsonSize + 100))
                                      ? (MaxMarshaledJsonSize - EstimatedUsedMarshaledJsonSize - 100)
                                      : 0;
            uint32_t SpaceUsedByMarshaledControl = 0;

            bool ControlIsFragmented = CurrentControlObject->MarshalControl(item,
                                                                            InUpdateMode,
                                                                            DataOffset,
                                                                            RemainingSpace,
                                                                            SpaceUsedByMarshaledControl);

            EstimatedUsedMarshaledJsonSize += SpaceUsedByMarshaledControl;

            if (0 == SpaceUsedByMarshaledControl)
            {
                if (1 == elementcount)
                {
                    rootDoc.clear();
                    item = AllocateJsonObject(items);
                    CurrentControlObject->MarshalErrorMessage(item);
                    elementcount = 0;
                }
                else
                {
                    items.remove(elementcount - 1);
                    --elementcount;
                }
                CurrentControlObject = nullptr;
            }
            else if (SingleControl ||
                     ControlIsFragmented ||
                     (ESPUI.jsonChunkNumberMax > 0 && elementcount >= (int)ESPUI.jsonChunkNumberMax) ||
                     (MaxMarshaledJsonSize < (EstimatedUsedMarshaledJsonSize + 100)))
            {
                CurrentControlObject = nullptr;
            }
            else
            {
                CurrentControlObject = CurrentControlObject->next;
            }
        }
    } while (false);

#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif

    return elementcount;
}

void _ESPUIcontrolMgr::appendControlObject(ControlObject_t* NewControlObject)
{
    NewControlObject->next = nullptr;
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif
    if (controls == nullptr)
    {
        controls = NewControlObject;
        controlsTail = NewControlObject;
    }
    else
    {
        controlsTail->next = NewControlObject;
        controlsTail = NewControlObject;
    }
    controlCount++;
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif
}

Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  const String& value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  void (*callback)(BasicControl*, int, void*),
                                                  void* userData)
{
    BasicControl* NewControlObject = nullptr;

    if (callback == nullptr && type == Control::Type::Label)
    {
        NewControlObject = new LabelControl(++idCounter, type, label, value, color, visible, parentControl);
    }
    else
    {
        NewControlObject = new Control(++idCounter, type, label, callback, value, color, visible, parentControl, userData);
    }

    appendControlObject(NewControlObject);
    return NewControlObject->GetId();
}

Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  long value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  void (*callback)(BasicControl*, int, void*),
                                                  void* userData)
{
    BasicControl* NewControlObject = nullptr;

    if (callback == nullptr && (type == Control::Type::Option || type == Control::Type::Separator ||
                                type == Control::Type::Min || type == Control::Type::Max ||
                                type == Control::Type::Step))
    {
        NewControlObject = new BasicControl(++idCounter, type, label, value, color, visible, parentControl);
    }
    else if (callback == nullptr && (type == Control::Type::Label || type == Control::Type::Tab))
    {
        NewControlObject = new LabelControl(++idCounter, type, label, value, color, visible, parentControl);
    }
    else
    {
        NewControlObject = new Control(++idCounter, type, label, callback, value, color, visible, parentControl, userData);
    }

    appendControlObject(NewControlObject);
    return NewControlObject->GetId();
}

Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  const char* value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  void (*callback)(BasicControl*, int, void*),
                                                  void* userData)
{
    BasicControl* NewControlObject = nullptr;

    if (callback == nullptr && (type == Control::Type::Separator))
    {
        NewControlObject = new BasicControl(++idCounter, type, label, 0L, color, visible, parentControl);
    }
    else if (callback == nullptr && (type == Control::Type::Label || type == Control::Type::Tab))
    {
        NewControlObject = new LabelControl(++idCounter, type, label, value, color, visible, parentControl);
    }
    else
    {
        NewControlObject = new Control(++idCounter, type, label, callback, value, color, visible, parentControl, userData);
    }

    appendControlObject(NewControlObject);
    return NewControlObject->GetId();
}

_ESPUIcontrolMgr ESPUIcontrolMgr;