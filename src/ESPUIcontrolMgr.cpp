
#include "ESPUIcontrolMgr.h"

static Control::ControlId_t idCounter = 0;

_ESPUIcontrolMgr::_ESPUIcontrolMgr()
{
#ifdef ESP32
    ControlsSemaphore = xSemaphoreCreateMutex();
    xSemaphoreGive(ControlsSemaphore);
#endif // def ESP32
}

void _ESPUIcontrolMgr::RemoveToBeDeletedControls()
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // def ESP32

    ControlObject_t * PreviousControl = nullptr;
    ControlObject_t * CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        ControlObject_t * NextControl = CurrentControl->next;
        if (CurrentControl->ToBeDeleted())
        {
            if (CurrentControl == controls)
            {
                // this is the root control
                controls = NextControl;
            }
            else
            {
                PreviousControl->next = NextControl;
            }
            if  ((CurrentControl->control_flags & CONTROL_FLAG_NUMERIC) ||
                 (CurrentControl->control_flags & CONTROL_FLAG_PCHAR)) 
            ;
            else
             if (CurrentControl->string_value) 
	      delete CurrentControl->string_value;
            delete CurrentControl;
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
#endif // def ESP32
}

BasicControl* _ESPUIcontrolMgr::getControl(Control::ControlId_t id)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // !def ESP32
    BasicControl* Response = getControlNoLock(id);
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // !def ESP32
    return Response;
}

// WARNING: Anytime you walk the chain of controllers, the protection semaphore
//          MUST be locked. This function assumes that the semaphore is locked
//          at the time it is called. Make sure YOU locked it :)
BasicControl* _ESPUIcontrolMgr::getControlNoLock(Control::ControlId_t id)
{
    return getControlObjectNoLock(id);
} // getControlNoLock

// WARNING: Anytime you walk the chain of controllers, the protection semaphore
//          MUST be locked. This function assumes that the semaphore is locked
//          at the time it is called. Make sure YOU locked it :)
_ESPUIcontrolMgr::ControlObject_t * _ESPUIcontrolMgr::getControlObjectNoLock(Control::ControlId_t id)
{
    ControlObject_t * Response = nullptr;
    ControlObject_t * CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        if (CurrentControl->GetId() == id)
        {
            if (!CurrentControl->ToBeDeleted())
            {
                Response = CurrentControl;
            }
            break;
        }
        CurrentControl = CurrentControl->next;
    }

    return Response;
} // getControlObjectNoLock

BasicControl* _ESPUIcontrolMgr::getFirstOptionId(Control::ControlId_t selector, long value)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // !def ESP32
    BasicControl* Response = getFirstOptionIdNoLock(selector, value);
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // !def ESP32
    return Response;
}

BasicControl* _ESPUIcontrolMgr::getFirstOptionIdNoLock(Control::ControlId_t selector, long value)
{
    return getFirstOptionIdObjectNoLock(selector, value);
} 

_ESPUIcontrolMgr::ControlObject_t * _ESPUIcontrolMgr::getFirstOptionIdObjectNoLock(Control::ControlId_t selector, long value)
{
    ControlObject_t * Response = nullptr;
    ControlObject_t * CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        if ((CurrentControl->parentControl == selector) && (CurrentControl->type == Control::Type::Option) &&
            (CurrentControl->control_flags & CONTROL_FLAG_NUMERIC) && 
	    ((value == -4) || (CurrentControl->numeric_value == value)))
        {
            if (!CurrentControl->ToBeDeleted())
            {
                Response = CurrentControl;
            }
            break;
        }
        CurrentControl = CurrentControl->next;
    }

    return Response;
} 


BasicControl* _ESPUIcontrolMgr::getNextOptionId(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // !def ESP32
    BasicControl* Response = getNextOptionIdNoLock(selector, value, prev_option_id);
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // !def ESP32
    return Response;
}

BasicControl* _ESPUIcontrolMgr::getNextOptionIdNoLock(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id)
{
    return getNextOptionIdObjectNoLock(selector, value, prev_option_id);
} 

_ESPUIcontrolMgr::ControlObject_t * _ESPUIcontrolMgr::getNextOptionIdObjectNoLock(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id)
{
    ControlObject_t * Response = nullptr;
    ControlObject_t * CurrentControl = controls;

    while (nullptr != CurrentControl)
    {
        if ((CurrentControl->parentControl == selector) && (CurrentControl->type == Control::Type::Option) &&
            (CurrentControl->control_flags & CONTROL_FLAG_NUMERIC) && 
	    ((value == -4) || (CurrentControl->numeric_value == value)) &&
            (CurrentControl->id > prev_option_id))
        {
            if (!CurrentControl->ToBeDeleted())
            {
                Response = CurrentControl;
            }
            break;
        }
        CurrentControl = CurrentControl->next;
    }

    return Response;
} 


bool _ESPUIcontrolMgr::removeControl(Control::ControlId_t id)
{
    bool Response = false;

    BasicControl* control = getControl(id);
    if (control)
    {
        Response = true;
        control->DeleteControl();
        controlCount--;
    }
#ifdef DEBUG_ESPUI
    else
    {
        //Serial.println(String("Could not Remove Control ") + String(id));
    }
#endif // def DEBUG_ESPUI

    return Response;
}

uint16_t _ESPUIcontrolMgr::removeSelectOptions(Control::ControlId_t select_id, Control::ControlId_t skip_id)
{
    
 #ifdef ESP32
   xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // !def ESP32
  

  uint16_t Response = 0;
  ControlObject_t * CurrentControl = controls;

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
#endif // !def ESP32
  

  return Response;
}


/*
Prepare a chunk of elements as a single JSON string. If the allowed number of elements is greater than the total
number this will represent the entire UI. More likely, it will represent a small section of the UI to be sent. The
client will acknowledge receipt by requesting the next chunk.
 */
uint32_t _ESPUIcontrolMgr::prepareJSONChunk(uint16_t startindex,
                                            JsonDocument & rootDoc,
                                            bool InUpdateMode,
                                            String FragmentRequestString,
                                            uint32_t CurrentSyncID)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // def ESP32

    int elementcount = 0;
    uint32_t MaxMarshaledJsonSize = (!InUpdateMode) ? ESPUI.jsonInitialDocumentSize : ESPUI.jsonUpdateDocumentSize;
    uint32_t EstimatedUsedMarshaledJsonSize = 0;

    do // once
    {
        // Follow the list until control points to the startindex'th node
        ControlObject_t * CurrentControlObject = controls;
        uint32_t currentIndex = 0;
        uint32_t DataOffset = 0;
        JsonArray items = rootDoc[F("controls")];
        bool SingleControl = false;

        if (!emptyString.equals(FragmentRequestString))
        {
            // Parse the fragment or directed update request from the UI
#if ARDUINOJSON_VERSION_MAJOR > 6
            JsonDocument FragmentRequest;
#else
            AllocateJsonDocument(document, 192);
#endif
            int FragmentRequestStartOffset = FragmentRequestString.indexOf('{');
            if (FragmentRequestStartOffset < 0)
            {
                break;
            }

            DeserializationError error = deserializeJson(FragmentRequest, FragmentRequestString.c_str() + FragmentRequestStartOffset);
            if (DeserializationError::Ok != error)
            {
                break;
            }

            if (!FragmentRequest.containsKey(F("id")) || !FragmentRequest.containsKey(F("offset")))
            {
                break;
            }

            uint16_t ControlId = uint16_t(FragmentRequest[F("id")]);
            DataOffset = uint16_t(FragmentRequest[F("offset")]);

            CurrentControlObject = getControlObjectNoLock(ControlId);
            if (nullptr == CurrentControlObject)
            {
                break;
            }

            currentIndex = 1;
            startindex = 0;
            SingleControl = true;
        }

        // Find the starting control to send
        while ((startindex > currentIndex) && (nullptr != CurrentControlObject))
        {
            // Only count active controls
            if ((!CurrentControlObject->ToBeDeleted()) && (!CurrentControlObject->SkipSend()))
            {
                if (InUpdateMode)
                {
                    // In update mode we only count the controls that have been updated
                    if (CurrentControlObject->NeedsSync(CurrentSyncID))
                    {
                        ++currentIndex;
                    }
                }
                else
                {
                    // Not in update mode. Count all active controls
                    ++currentIndex;
                }
            }
            CurrentControlObject = CurrentControlObject->next;
        }

        // Any controls left to be processed?
        if (nullptr == CurrentControlObject)
        {
            break;
        }

        // Serialize controls into this chunk message
        while (nullptr != CurrentControlObject)
        {
            // Skip deleted controls or controls marked to skip
            if ((CurrentControlObject->ToBeDeleted() || CurrentControlObject->SkipSend()) && !SingleControl)
            {
                CurrentControlObject = CurrentControlObject->next;
                continue;
            }

            if (InUpdateMode && !SingleControl)
            {
                if (!CurrentControlObject->NeedsSync(CurrentSyncID))
                {
                    // Control has not been updated. Skip it
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

            // Did the control fail to fit into the remaining space?
            if (0 == SpaceUsedByMarshaledControl)
            {
                if (1 == elementcount)
                {
                    // Control is too large even as the sole element in the chunk
                    rootDoc.clear();
                    item = AllocateJsonObject(items);
                    CurrentControlObject->MarshalErrorMessage(item);
                    elementcount = 0;
                }
                else
                {
                    // Defer this control to the next chunk (0-based index removal)
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
                // Chunk limit reached, fragmented control, or single control; exit loop cleanly
                CurrentControlObject = nullptr;
            }
            else
            {
                CurrentControlObject = CurrentControlObject->next;
            }
        } // end while (nullptr != CurrentControlObject)

    } while (false);

#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // def ESP32

    return elementcount;
}


Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  const String& value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  void (*callback)(BasicControl*, int, void*),
                                                  void *userData)
{
    BasicControl * NewControlObject = nullptr;

    if (callback == nullptr && type == Control::Type::Label) {
        NewControlObject = new LabelControl(++idCounter, type, label, value, color, visible, parentControl);
    } else {
        NewControlObject = new Control(++idCounter, type, label, callback, value, color, visible, parentControl, userData);
    }
    NewControlObject->next = nullptr;

#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif
    if (controls == nullptr) {
        controls = NewControlObject;
    } else {
        ControlObject_t * iterator = controls;
        while (iterator->next != nullptr) { iterator = iterator->next; }
        iterator->next = NewControlObject;
    }
    controlCount++;
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif
    return NewControlObject->GetId();
}

Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  long value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  void (*callback)(BasicControl*, int, void*),
                                                  void *userData)
{
    BasicControl * NewControlObject = nullptr;

    if (callback == nullptr && (type == Control::Type::Option || type == Control::Type::Separator ||
                                type == Control::Type::Min || type == Control::Type::Max ||
                                type == Control::Type::Step)) {
        NewControlObject = new BasicControl(++idCounter, type, label, value, color, visible, parentControl);
    } else if (callback == nullptr && (type == Control::Type::Label || type == Control::Type::Tab)) {
        NewControlObject = new LabelControl(++idCounter, type, label, value, color, visible, parentControl);
    } else {
        NewControlObject = new Control(++idCounter, type, label, callback, value, color, visible, parentControl, userData);
    }
    NewControlObject->next = nullptr;

#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif
    if (controls == nullptr) {
        controls = NewControlObject;
    } else {
        ControlObject_t * iterator = controls;
        while (iterator->next != nullptr) { iterator = iterator->next; }
        iterator->next = NewControlObject;
    }
    controlCount++;
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif
    return NewControlObject->GetId();
}

Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  const char* value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  void (*callback)(BasicControl*, int, void*),
                                                  void *userData)
{
    BasicControl * NewControlObject = nullptr;

    if (callback == nullptr && (type == Control::Type::Separator)) {
        NewControlObject = new BasicControl(++idCounter, type, label, 0L, color, visible, parentControl);
    } else if (callback == nullptr && (type == Control::Type::Label  || type == Control::Type::Tab)) {
        NewControlObject = new LabelControl(++idCounter, type, label, value, color, visible, parentControl);
    } else {
        NewControlObject = new Control(++idCounter, type, label, callback, value, color, visible, parentControl, userData);
    }
    NewControlObject->next = nullptr;

#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif
    if (controls == nullptr) {
        controls = NewControlObject;
    } else {
        ControlObject_t * iterator = controls;
        while (iterator->next != nullptr) { iterator = iterator->next; }
        iterator->next = NewControlObject;
    }
    controlCount++;
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif
    return NewControlObject->GetId();
}
/*
_ESPUIcontrolMgr::ControlObject_t::ControlObject_t(Control::ControlId_t id, Control::Type type, const char* label, void (*callback)(BasicControl*, int),
    const String& value, Control::Color color, bool visible, ControlId_t parentControl)
    : Control(id, type, label, callback, value, color, visible, parentControl)
{}


_ESPUIcontrolMgr::ControlObject_t::ControlObject_t(Control::ControlId_t id, Control::Type type, const char* label, void (*callback)(BasicControl*, int),
    long value, Control::Color color, bool visible, ControlId_t parentControl)
    : Control(id, type, label, callback, value, color, visible, parentControl)
{}


_ESPUIcontrolMgr::ControlObject_t::ControlObject_t(Control::ControlId_t id, Control::Type type, const char* label, void (*callback)(BasicControl*, int),
    const char* value, Control::Color color, bool visible, ControlId_t parentControl)
    : Control(id, type, label, callback, value, color, visible, parentControl)
{}*/

// Instantiate the singleton
_ESPUIcontrolMgr ESPUIcontrolMgr;
