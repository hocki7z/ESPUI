#include "ESPUI.h"

static const String ControlError = "*** ESPUI ERROR: Could not transfer control ***";


static String global_value;
static bool global_value_reserve = true;
 
Control::Control(Control::ControlId_t id, Control::Type type, const char* label, std::function<void(Control*, int)> callback,
    const String& value, Control::Color color, bool visible, ControlId_t parentControl)
    : id(id),
      type(type),
      label(label),
      callback(callback),
      //value(value),
      color(color),
      //visible(visible),
      //wide(false),
      //vertical(false),
      //enabled(true),
      parentControl(parentControl)
{
    ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    //value.reserve(128);
    //value = PSTR("1");
    //log_i("label %s, type %u", label, type);
    //if (type == Control::Type::Label)
     //log_i("label size %u", value.length());

    switch (type) {


      case Option:
      case Number:
      case Select:
      case Switcher:
      case Slider: {
        control_flags |= CONTROL_FLAG_NUMERIC;
        numeric_value = value.toInt();}
      break;


      default:{
	if (global_value_reserve) {
         global_value_reserve = false;
         global_value.reserve(1024);
       }
        string_value = new String();
//if (type == Control::Type::Label)
if (value.length() < 16)
  string_value->reserve(16);
        //log_i("string value ptr 0x%08X", string_value);
        (*string_value) = value;
      }
  
    }
}

Control::Control(Control::ControlId_t id, Control::Type type, const char* label, std::function<void(Control*, int)> callback,
    long value, Control::Color color, bool visible, ControlId_t parentControl)
    : id(id),
      type(type),
      label(label),
      callback(callback),
      //value(value),
      color(color),
      //visible(visible),
      //wide(false),
      //vertical(false),
      //enabled(true),
      parentControl(parentControl)
{
    ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    //value.reserve(128);
    //value = PSTR("1");
//log_i("label %s, type %u", label, type);


    switch (type) {


      case Option:
      case Number:
      case Select:
      case Switcher:
      case Slider:
      case Min:
      case Max:
      case Gauge: {
        control_flags |= CONTROL_FLAG_NUMERIC;
        numeric_value = value;}
      break;


      default: {

        string_value = new String();
        *string_value = value;
       }
  
    }
}

Control::Control(Control::ControlId_t id, Control::Type type, const char* label, std::function<void(Control*, int)> callback,
    const char* value, Control::Color color, bool visible, ControlId_t parentControl)
    : id(id),
      type(type),
      label(label),
      callback(callback),
      //value(value),
      color(color),
      //visible(visible),
      //wide(false),
      //vertical(false),
      //enabled(true),
      parentControl(parentControl)
{
    ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    //value.reserve(128);
    //value = PSTR("1");
//log_i("label %s, type %u", label, type);

    switch (type) {


      case Option:
      case Number:
      case Select:
      case Switcher:
      case Slider: {
        control_flags |= CONTROL_FLAG_NUMERIC;
        numeric_value = strtol(value, nullptr, 0);}
      break;


      default:
	control_flags |= CONTROL_FLAG_PCHAR;
        cstr_value = value;
  
    }
}


Control::Control(const Control& Control)
    : type(Control.type),
      id(Control::noParent),
      label(Control.label),
      callback(Control.callback),
      //value(Control.value),
      color(Control.color),
      //visible(Control.visible),
      parentControl(Control.parentControl),
      ControlChangeID(Control.ControlChangeID)
{ 
    if (Control.control_flags & CONTROL_FLAG_VISIBLE)
      control_flags |= CONTROL_FLAG_VISIBLE;

switch (Control.type) {


      case Option:
      case Number:
       
        numeric_value = Control.numeric_value;
      break;


      default: {

        string_value = new String();
        *string_value = *Control.string_value;
      }
  
    }

}

void Control::SendCallback(int type)
{
    if(callback)
    {
        callback(this, type);
    }
}

long Control::getValueInt(){

  if (control_flags & CONTROL_FLAG_NUMERIC)
    return numeric_value;
  else
   if (string_value)
    return string_value->toInt();
  return 0;
}
    
const char *Control::getValueCstr() {

  if (control_flags & CONTROL_FLAG_NUMERIC)
    return nullptr;
  else
   if (control_flags & CONTROL_FLAG_PCHAR)
    return cstr_value;
  else 
    if (string_value)
     return string_value->c_str();
  return nullptr;
}

const String &Control::getValue() {

  if (control_flags & CONTROL_FLAG_NUMERIC)
    return emptyString;
  else
    return *string_value;
}


void Control::DeleteControl()
{
    //_ToBeDeleted = true;
    control_flags |= CONTROL_FLAG_DELETED;
    callback = nullptr;
}


void Control::MarshalControl(const JsonObject &item, const bool refresh) const
{
	// this code assumes MaxMarshaledLength > JsonMarshalingRatio
	if (refresh && type == Control::Type::Tab)
		return;

	Control::Type TempType = (Control::Type::Password == type) ? Control::Type::Text : type;
	
	item["type"] = static_cast<uint32_t>(TempType) + (refresh ? 100 : 0);	
	item["id"] = id;
	item["label"] = label;

	if (control_flags & CONTROL_FLAG_NUMERIC)
	item["value"]  = numeric_value;
	else
	if (control_flags & CONTROL_FLAG_PCHAR)
      	  item["value"] = cstr_value;
    	else 
      	if (string_value)
      	item["value"] = *string_value;

	item["visible"] = control_flags & CONTROL_FLAG_VISIBLE;

	item["color"] = static_cast<int>(color);
	item["enabled"] = control_flags & CONTROL_FLAG_ENABLED;

	if (panelStyle) {item["panelStyle"]    = panelStyle;}
    	if (elementStyle)  {item["elementStyle"]  = elementStyle;}

	
	if (Control::Type::Option == type) {

      	  if (secondParent > 0) {item["secondParent"] = secondParent;}
      	  if (thirdParent > 0) {item["thirdParent"] = thirdParent;}
    	}


	if (control_flags & CONTROL_FLAG_OPTION_UPDATE) {

	  item["optionMode"]     = 1;
       	  item["dOptionId"]     = dynamic_option_id;
        }
    	else
    	if (control_flags & CONTROL_FLAG_OPTION_ADD) {
   	
	  item["optionMode"]     = 2;
          item["dOptionId"]     = dynamic_option_id;
	}
    	else
    	if (control_flags & CONTROL_FLAG_OPTION_REMOVE) {
      	
	  item["optionMode"]     = 3;
       	  item["dOptionId"]     = dynamic_option_id;
	}
    	else
    	if ((Control::Type::Option != type) && (inputType)) 
 	  item["inputType"]     = inputType;
    	
	if (control_flags & CONTROL_FLAG_WIDE) 
	  item["wide"] = true;
	
    	if (control_flags & CONTROL_FLAG_VERTICAL) 
	  item["vertical"] = true;

    	if (parentControl != Control::noParent) 
          item["parentControl"] = parentControl;
    

    	// special case for selects: to preselect an option, you have to add
    	// "selected" to <option>
    	if (Control::Type::Option == type) {

          Control* ParentControl = ESPUI.getControlNoLock(parentControl);
          
	  if (nullptr == ParentControl) {
            item["selected"] = "";
          }
          else 
	  if (ParentControl->numeric_value == numeric_value) {
            item["selected"] = "selected";
          }
          else
          {
            item["selected"] = "";
          }
        }
     }


bool Control::MarshalControl(JsonObject & _item,
                             bool refresh,
                             uint32_t StartingOffset,
                             uint32_t AvailMarshaledLength,
                             uint32_t &EstimatedMarshaledLength)
{
    // this code assumes MaxMarshaledLength > JsonMarshalingRatio
    //Serial.println(String("MarshalControl:           StartingOffset: ") + String(StartingOffset));
    //Serial.println(String("MarshalControl:     AvailMarshaledLength: ") + String(AvailMarshaledLength));
    //Serial.println(String("MarshalControl:               Control ID: ") + String(id));

    bool ControlIsFragmented = false;
    // create a new item in the response document
    JsonObject & item = _item;


    if (control_flags & CONTROL_FLAG_NUMERIC)
     global_value = numeric_value;
    else 
     if (control_flags & CONTROL_FLAG_PCHAR)
      global_value = cstr_value;
    else 
      if (string_value)
      global_value = *string_value;

    // how much space do we expect to use?
    uint32_t ValueMarshaledLength   = (global_value.length() - StartingOffset) * JsonMarshalingRatio;
    uint32_t LabelMarshaledLength   = strlen(label) * JsonMarshalingRatio;
    uint32_t MinimumMarshaledLength = LabelMarshaledLength + JsonMarshaledOverhead;
    uint32_t MaximumMarshaledLength = ValueMarshaledLength + MinimumMarshaledLength;
    uint32_t SpaceForMarshaledValue = AvailMarshaledLength - MinimumMarshaledLength;
    //Serial.println(String("MarshalControl:           value.length(): ") + String(value.length()));
    //Serial.println(String("MarshalControl:     ValueMarshaledLength: ") + String(ValueMarshaledLength));
    //Serial.println(String("MarshalControl:     LabelMarshaledLength: ") + String(LabelMarshaledLength));
    //Serial.println(String("MarshalControl:   MaximumMarshaledLength: ") + String(MaximumMarshaledLength));
    //Serial.println(String("MarshalControl:   MinimumMarshaledLength: ") + String(MinimumMarshaledLength));
    //Serial.println(String("MarshalControl:   SpaceForMarshaledValue: ") + String(SpaceForMarshaledValue));

    // will the item fit in the remaining space? Fragment if not
    if (AvailMarshaledLength < MinimumMarshaledLength)
    {
        //Serial.println(String("MarshalControl: Cannot Marshal control. Not enough space for basic headers."));
        EstimatedMarshaledLength = 0;
        return false;
    }

    uint32_t MaxValueLength = (SpaceForMarshaledValue / JsonMarshalingRatio);
    //Serial.println(String("MarshalControl:           MaxValueLength: ") + String(MaxValueLength));

    uint32_t ValueLenToSend = min((global_value.length() - StartingOffset), MaxValueLength);
    //Serial.println(String("MarshalControl:           ValueLenToSend: ") + String(ValueLenToSend));

    uint32_t AdjustedMarshaledLength = (ValueLenToSend * JsonMarshalingRatio) + MinimumMarshaledLength;
    //Serial.println(String("MarshalControl:  AdjustedMarshaledLength: ") + String(AdjustedMarshaledLength));

    bool NeedToFragment = (ValueLenToSend < global_value.length());
    //Serial.println(String("MarshalControl:           NeedToFragment: ") + String(NeedToFragment));

    if ((AdjustedMarshaledLength > AvailMarshaledLength) && (0 != ValueLenToSend))
    {
        //Serial.println(String("MarshalControl: Cannot Marshal control. Not enough space for marshaled control."));
        EstimatedMarshaledLength = 0;
        return false;
    }

    EstimatedMarshaledLength = AdjustedMarshaledLength;

    // are we fragmenting?
    if(NeedToFragment || StartingOffset)
    {
        //Serial.println(String("MarshalControl:Start Fragment Processing"));
        //Serial.println(String("MarshalControl:id:                ") + String(id));
        //Serial.println(String("MarshalControl:StartingOffset:    ") + String(StartingOffset));
/*
        if(0 == StartingOffset)
        {
            //Serial.println(String("MarshalControl: New control to fragement. ID: ") + String(id));
        }
        else
        {
            //Serial.println(String("MarshalControl: Next fragement. ID: ") + String(id));
        }
*/
        // indicate that no additional controls should be sent
        ControlIsFragmented = true;

        // fill in the fragment header
        _item[F("type")] = uint32_t(Control::Type::Fragment);
        _item[F("id")]   = id;

        ////Serial.println(String("MarshalControl:Final length:      ") + String(length));

        _item[F("offset")] = StartingOffset;
        _item[F("length")] = ValueLenToSend;
        _item[F("total")] = global_value.length();
        AllocateNamedJsonObject(item, _item, F("control"));
    }

    item[F("id")]      = id;
    Control::Type TempType = (Control::Type::Password == type) ? Control::Type::Text : type;
    if(refresh)
    {
        item[F("type")] = uint32_t(TempType) + uint32_t(Control::Type::UpdateOffset);
    }
    else
    {
        item[F("type")] = uint32_t(TempType);
    }

    item[F("label")]   = label;
    global_value = (Control::Type::Password == type) ? 
	F ("--------") : global_value.substring(StartingOffset, StartingOffset + ValueLenToSend);
    item[F ("value")]  = global_value;
    item[F("visible")] = control_flags & CONTROL_FLAG_VISIBLE;
    item[F("color")]   = (int)color;
    item[F("enabled")] = control_flags & CONTROL_FLAG_ENABLED;

    if (panelStyle) {item[F("panelStyle")]    = panelStyle;}
    if (elementStyle)  {item[F("elementStyle")]  = elementStyle;}

    if (Control::Type::Option == type) {

      if (secondParent > 0) {item[F("secondParent")] = secondParent;}
      if (thirdParent > 0) {item[F("thirdParent")] = thirdParent;}
    }

    if (control_flags & CONTROL_FLAG_OPTION_UPDATE)
      {item[F("optionMode")]     = 1;
       item[F("dOptionId")]     = dynamic_option_id;
       }
    else
    if (control_flags & CONTROL_FLAG_OPTION_ADD)
      {item[F("optionMode")]     = 2;
       item[F("dOptionId")]     = dynamic_option_id;}
    else
    if (control_flags & CONTROL_FLAG_OPTION_REMOVE)
      {item[F("optionMode")]     = 3;
       item[F("dOptionId")]     = dynamic_option_id;}
    else
    if ((Control::Type::Option != type) && (inputType)) 
      {item[F("inputType")]     = inputType;}
    if (control_flags & CONTROL_FLAG_WIDE) {item[F("wide")] = true;}
    if (control_flags & CONTROL_FLAG_VERTICAL) {item[F("vertical")] = true;}
    if (parentControl != Control::noParent)
    {
        item[F("parentControl")] = parentControl;
    }

    // special case for selects: to preselect an option, you have to add
    // "selected" to <option>
    if (Control::Type::Option == type)
    {
        Control* ParentControl = ESPUI.getControlNoLock(parentControl);
        if (nullptr == ParentControl)
        {
            item[F("selected")] = emptyString;
        }
        else if (ParentControl->numeric_value == numeric_value)
        {
            item[F("selected")] = F("selected");
        }
        else
        {
            item[F("selected")] = "";
        }
    }

    //Serial.println(String("MarshalControl:Done"));
    return ControlIsFragmented;
}

void Control::MarshalErrorMessage(JsonObject & item)
{
    item[F("id")]      = id;
    item[F("type")]    = uint32_t(Control::Type::Label);
    item[F("label")]   = ControlError.c_str();
    item[F("value")]   = ControlError;
    item[F("visible")] = true;
    item[F("color")]   = (int)Control::Color::Carrot;
    item[F("enabled")] = true;

    if (parentControl != Control::noParent)
    {
        item[F("parentControl")] = parentControl;
    }
}

void Control::onWsEvent(String & cmd, String& data)
{
    do // once
    {
        //Serial.println(String(F("Control::onWsEvent")));
        SetControlChangedId(ESPUI.GetNextControlChangeId());
        if (!HasCallback())
        {
            #if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity)
                {
                    //Serial.println(String(F("Control::onWsEvent:No callback found for ID ")) + String(id));
                }
            #endif
            break;
        }

        //Serial.println("Control::onWsEvent:Generating callback");
        if (cmd.equals(F("bdown")))
        {
            SendCallback(B_DOWN);
            break;
        }

        if (cmd.equals(F("bup")))
        {
            SendCallback(B_UP);
            break;
        }

        if (cmd.equals(F("pfdown")))
        {
            SendCallback(P_FOR_DOWN);
            break;
        }

        if (cmd.equals(F("pfup")))
        {
            SendCallback(P_FOR_UP);
            break;
        }

        if (cmd.equals(F("pldown")))
        {
            SendCallback(P_LEFT_DOWN);
            break;
        }
        else if (cmd.equals(F("plup")))
        {
            SendCallback(P_LEFT_UP);
        }
        else if (cmd.equals(F("prdown")))
        {
            SendCallback(P_RIGHT_DOWN);
        }
        else if (cmd.equals(F("prup")))
        {
            SendCallback(P_RIGHT_UP);
        }
        else if (cmd.equals(F("pbdown")))
        {
            SendCallback(P_BACK_DOWN);
        }
        else if (cmd.equals(F("pbup")))
        {
            SendCallback(P_BACK_UP);
        }
        else if (cmd.equals(F("pcdown")))
        {
            SendCallback(P_CENTER_DOWN);
        }
        else if (cmd.equals(F("pcup")))
        {
            SendCallback(P_CENTER_UP);
        }
        else if (cmd.equals(F("sactive")))
        {
            //value = "1";
            numeric_value = 1;
            SendCallback(S_ACTIVE);
        }
        else if (cmd.equals(F("sinactive")))
        {
            //value = "0";
            numeric_value = 0;
            // updateControl(c, client->id());
            SendCallback(S_INACTIVE);
        }
        else if (cmd.equals(F("slvalue")))
        {
            numeric_value = data.toInt();
            // updateControl(c, client->id());
            SendCallback(SL_VALUE);
        }
        else if (cmd.equals(F("nvalue")))
        {
            numeric_value = data.toInt();
            // updateControl(c, client->id());
            SendCallback(N_VALUE);
        }
        else if (cmd.equals(F("tvalue")))
        {
            *string_value = data;
            // updateControl(c, client->id());
            SendCallback(T_VALUE);
        }
        else if (cmd.equals(F("tabvalue")))
        {
            SendCallback(0);
        }
        else if (cmd.equals(F("svalue")))
        {
            numeric_value = data.toInt();
            // updateControl(c, client->id());
            SendCallback(S_VALUE);
        }
        else if (cmd.equals(F("time")))
        {
            *string_value = data;
            // updateControl(c, client->id());
            SendCallback(TM_VALUE);
        }
        else
        {
            #if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity)
                {
                    //Serial.println(F("Control::onWsEvent:Malformed message from the websocket"));
                }
            #endif
        }
    } while (false);
}


void Control::onWsEvent(const std::string &cmd, const std::string &data)
{
    do // once
    {
        //Serial.println(String(F("Control::onWsEvent")));
        SetControlChangedId(ESPUI.GetNextControlChangeId());
        if (!HasCallback())
        {
            #if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity)
                {
                    //Serial.println(String(F("Control::onWsEvent:No callback found for ID ")) + String(id));
                }
            #endif
            break;
        }

        //Serial.println("Control::onWsEvent:Generating callback");
        if (cmd == "bdown")
        {
            SendCallback(B_DOWN);
            break;
        }

        if (cmd == "bup")
        {
            SendCallback(B_UP);
            break;
        }

        if (cmd == "pfdown")
        {
            SendCallback(P_FOR_DOWN);
            break;
        }

        if (cmd == "pfup")
        {
            SendCallback(P_FOR_UP);
            break;
        }

        if (cmd == "pldown")
        {
            SendCallback(P_LEFT_DOWN);
            break;
        }
        else if (cmd == "plup")
        {
            SendCallback(P_LEFT_UP);
        }
        else if (cmd == "prdown")
        {
            SendCallback(P_RIGHT_DOWN);
        }
        else if (cmd == "prup")
        {
            SendCallback(P_RIGHT_UP);
        }
        else if (cmd == "pbdown")
        {
            SendCallback(P_BACK_DOWN);
        }
        else if (cmd == "pbup")
        {
            SendCallback(P_BACK_UP);
        }
        else if (cmd == "pcdown")
        {
            SendCallback(P_CENTER_DOWN);
        }
        else if (cmd == "pcup")
        {
            SendCallback(P_CENTER_UP);
        }
        else if (cmd == "sactive")
        {
           
            numeric_value = 1;
            SendCallback(S_ACTIVE);
        }
        else if (cmd == "sinactive")
        {
            
            numeric_value = 0;
            
            SendCallback(S_INACTIVE);
        }
        else if (cmd == "slvalue")
        {
            numeric_value = std::stoul(data);
            
            SendCallback(SL_VALUE);
        }
        else if (cmd == "nvalue")
        {
            numeric_value = std::stoul(data);
            
            SendCallback(N_VALUE);
        }
        else if (cmd == "tvalue")
        {
            *string_value = data.c_str();
            
            SendCallback(T_VALUE);
        }
        else if (cmd == "tabvalue")
        {
            SendCallback(0);
        }
        else if (cmd == "svalue")
        {
            numeric_value = std::stoul(data);
            
            SendCallback(S_VALUE);
        }
        else if (cmd == "time")
        {
            *string_value = data.c_str();
         
            SendCallback(TM_VALUE);
        }
        else
        {
            #if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity)
                {
                    //Serial.println(F("Control::onWsEvent:Malformed message from the websocket"));
                }
            #endif
        }
    } while (false);
}
