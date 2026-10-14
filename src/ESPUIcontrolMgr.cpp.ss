
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

Control* _ESPUIcontrolMgr::getControl(Control::ControlId_t id)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // !def ESP32
    Control* Response = getControlNoLock(id);
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // !def ESP32
    return Response;
}

// WARNING: Anytime you walk the chain of controllers, the protection semaphore
//          MUST be locked. This function assumes that the semaphore is locked
//          at the time it is called. Make sure YOU locked it :)
Control* _ESPUIcontrolMgr::getControlNoLock(Control::ControlId_t id)
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

Control* _ESPUIcontrolMgr::getFirstOptionId(Control::ControlId_t selector, long value)
{
#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // !def ESP32
    Control* Response = getFirstOptionIdNoLock(selector, value);
#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // !def ESP32
    return Response;
}

Control* _ESPUIcontrolMgr::getFirstOptionIdNoLock(Control::ControlId_t selector, long value)
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
            (CurrentControl->control_flags & CONTROL_FLAG_NUMERIC) && (CurrentControl->numeric_value == value))
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

    Control* control = getControl(id);
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
            CurrentControl->ToBeDeleted();
    	    CurrentControl->callback = nullptr;
            Response++;
            controlCount--;
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
                                            const std::string &value,
                                            uint32_t CurrentSyncID)
{
Serial.printf("\n\rstartindex = %u", startindex);	
xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);

	// Follow the list until control points to the startindex node
	const JsonArray items = rootDoc["controls"];

	if (!value.empty())
	{
		// this is actually a fragment or directed update request
		// parse the string we got from the UI and try to update that specific
		// control.
		JsonDocument FragmentRequest;
		const auto FragmentRequestStartOffset = value.find('{');
		const DeserializationError error =
				deserializeJson(FragmentRequest, value.substr(FragmentRequestStartOffset));
		if (DeserializationError::Ok != error)
		{
			Serial.println("ERROR:prepareJSONChunk:Fragmentation:Could not extract json from the fragment request");
			xSemaphoreGive(ControlsSemaphore);
			return 0;
		}

		if (!FragmentRequest["id"].is<std::string>())
		{
			Serial.println("ERROR:prepareJSONChunk:Fragmentation:Request does not contain a control ID");
			xSemaphoreGive(ControlsSemaphore);
			return 0;
		}
		const auto ControlId = FragmentRequest["id"].as<uint16_t>();

		const auto control = getControlObjectNoLock(ControlId);
		if (nullptr == control)
		{
			Serial.println((
				std::string("ERROR:prepareJSONChunk:Fragmentation:Requested control: ") + std::to_string(ControlId) +
				" does not exist").c_str());
			xSemaphoreGive(ControlsSemaphore);
			return 0;
		}

		//Send Update for a Single Element
		const auto item = items.add<JsonObject>();
		control->MarshalControl(item, InUpdateMode);
		xSemaphoreGive(ControlsSemaphore);
		return 1;
	}

	// keep track of the number of elements we have serialised into this
	// message. Overflow is detected and handled later in this loop
	// and needs an index to the last item added.
	uint32_t currentIndex = 0;

	int elementCount = 0;
	ControlObject_t * CurrentControlObject = controls;

	while ((startindex > currentIndex) && (nullptr != CurrentControlObject))
        {
            // only count active controls
            if ((!CurrentControlObject->ToBeDeleted()) && (!CurrentControlObject->SkipSend()))
            {
                if(InUpdateMode)
                {
                    // In update mode we only count the controls that have been updated.
                    if(CurrentControlObject->NeedsSync(CurrentSyncID))
                    {
                        ++currentIndex;
                    }
                }
                else
                {
                    // not in update mode. Count all active controls
                    ++currentIndex;
                }
            }
            CurrentControlObject = CurrentControlObject->next;
        }

        // any controls left to be processed?
        if(nullptr == CurrentControlObject)
        {
            //Serial.println("prepareJSONChunk: No controls to process");
            return 0;
        }


	while (nullptr != CurrentControlObject)
	
	{
		
		if (CurrentControlObject->ToBeDeleted() || CurrentControlObject->SkipSend())
            {
                CurrentControlObject = CurrentControlObject->next;
                continue;
            }	
		// control has not been updated. Skip it
		if (InUpdateMode && !CurrentControlObject->NeedsSync(CurrentSyncID)) {
			CurrentControlObject = CurrentControlObject->next;
			continue;
		}

		auto item = items.add<JsonObject>();
		elementCount++;
		CurrentControlObject->MarshalControl(item, InUpdateMode);
		CurrentControlObject = CurrentControlObject->next;
		if (elementCount > 6) break;
	}

	xSemaphoreGive(ControlsSemaphore);
	return elementCount;
}


Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  const String& value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  std::function<void(Control*, int)> callback)
{
    // Create a Wrapper and a control

    ControlObject_t * NewControlObject  = new ControlObject_t(++idCounter, type, label, callback, value, color, visible, parentControl);
    NewControlObject->next = nullptr;

#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // def ESP32

    if (controls == nullptr)
    {
        controls = NewControlObject;
    }
    else
    {
        ControlObject_t * iterator = controls;
        while (iterator->next != nullptr)
        {
            iterator = iterator->next;
        }
        iterator->next = NewControlObject;
    }

    controlCount++;

#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // def ESP32

    return NewControlObject->GetId();
}


Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  long value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  std::function<void(Control*, int)> callback)
{
    // Create a Wrapper and a control

    ControlObject_t * NewControlObject  = new ControlObject_t(++idCounter, type, label, callback, value, color, visible, parentControl);
    NewControlObject->next = nullptr;

#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // def ESP32

    if (controls == nullptr)
    {
        controls = NewControlObject;
    }
    else
    {
        ControlObject_t * iterator = controls;
        while (iterator->next != nullptr)
        {
            iterator = iterator->next;
        }
        iterator->next = NewControlObject;
    }

    controlCount++;

#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // def ESP32

    return NewControlObject->GetId();
}

Control::ControlId_t _ESPUIcontrolMgr::addControl(Control::Type type,
                                                  const char* label,
                                                  const char* value,
                                                  Control::Color color,
                                                  Control::ControlId_t parentControl,
                                                  bool visible,
                                                  std::function<void(Control*, int)> callback)
{
    // Create a Wrapper and a control

    ControlObject_t * NewControlObject  = new ControlObject_t(++idCounter, type, label, callback, value, color, visible, parentControl);
    NewControlObject->next = nullptr;

#ifdef ESP32
    xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
#endif // def ESP32

    if (controls == nullptr)
    {
        controls = NewControlObject;
    }
    else
    {
        ControlObject_t * iterator = controls;
        while (iterator->next != nullptr)
        {
            iterator = iterator->next;
        }
        iterator->next = NewControlObject;
    }

    controlCount++;

#ifdef ESP32
    xSemaphoreGive(ControlsSemaphore);
#endif // def ESP32

    return NewControlObject->GetId();
}


_ESPUIcontrolMgr::ControlObject_t::ControlObject_t(Control::ControlId_t id, Control::Type type, const char* label, std::function<void(Control*, int)> callback,
    const String& value, Control::Color color, bool visible, ControlId_t parentControl)
    : Control(id, type, label, callback, value, color, visible, parentControl)
{}


_ESPUIcontrolMgr::ControlObject_t::ControlObject_t(Control::ControlId_t id, Control::Type type, const char* label, std::function<void(Control*, int)> callback,
    long value, Control::Color color, bool visible, ControlId_t parentControl)
    : Control(id, type, label, callback, value, color, visible, parentControl)
{}


_ESPUIcontrolMgr::ControlObject_t::ControlObject_t(Control::ControlId_t id, Control::Type type, const char* label, std::function<void(Control*, int)> callback,
    const char* value, Control::Color color, bool visible, ControlId_t parentControl)
    : Control(id, type, label, callback, value, color, visible, parentControl)
{}

// Instantiate the singleton
_ESPUIcontrolMgr ESPUIcontrolMgr;
