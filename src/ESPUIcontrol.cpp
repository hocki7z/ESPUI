#include "ESPUI.h"

static const String ControlError = "*** ESPUI ERROR: Could not transfer control ***";


static String global_value;
static bool global_value_reserve = true;
static uint32_t new_string_counter = 0;

BasicControl::BasicControl(ControlId_t id, Type type, const char* label,
    long value, Color color, bool visible, ControlId_t parentControl)
    : id(id),
      type(type),
      label(label),
      color(color),
      parentControl(parentControl)
{
    ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
//log_i("pchar %u, %s", type, label);

    
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
       }  
    }
}

BasicControl::BasicControl(const BasicControl& Control)
    : type(Control.type),
      id(Control::noParent),
      label(Control.label),
      color(Control.color),
      control_flags(Control.control_flags),
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
      } 
    }
}


LabelControl::LabelControl(Control::ControlId_t id, Control::Type type, const char* label,
    const String& value, Control::Color color, bool visible, ControlId_t parentControl) :
BasicControl(id, type, label, 0, color, visible, parentControl)
    
{
    
    
    ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    
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
        if (value != emptyString)
          (*string_value) = value;
        control_flags |= CONTROL_FLAG_STRING;

        new_string_counter++;
        log_i("label %s, new_string_counter = %u, length %u", label, new_string_counter, string_value->length());
      }
  
    }
}

LabelControl::LabelControl(Control::ControlId_t id, Control::Type type, const char* label,
    long value, Control::Color color, bool visible, ControlId_t parentControl) :
BasicControl(id, type, label, 0, color, visible, parentControl)
    
{
  
    ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    
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
        (*string_value) = value;
        control_flags |= CONTROL_FLAG_STRING;

        new_string_counter++;
        log_i("label %s, new_string_counter = %u, length %u", label, new_string_counter, string_value->length());
       }  
    }
}

LabelControl::LabelControl(Control::ControlId_t id, Control::Type type, const char* label,
    const char* value, Control::Color color, bool visible, ControlId_t parentControl) :
BasicControl(id, type, label, 0, color, visible, parentControl)
    
{
    ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    
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


LabelControl::LabelControl(const LabelControl& Control) :
BasicControl(noParent, Control.type, Control.label, 0, Control.color, true, Control.parentControl) {
 
  elementStyle = Control.elementStyle;
  panelStyle = Control.panelStyle;

if (Control.control_flags & CONTROL_FLAG_VISIBLE)
      control_flags |= CONTROL_FLAG_VISIBLE;

switch (Control.type) {


      case Option:
      case Number:
       
        numeric_value = Control.numeric_value;
      break;


      default: {

	if (control_flags & CONTROL_FLAG_PCHAR)
	  cstr_value = Control.cstr_value;
        else {
         
         string_value = new String();
        (*string_value) = (*Control.string_value);
        control_flags |= CONTROL_FLAG_STRING;

        new_string_counter++;
        log_i("label %s, new_string_counter = %u, length %u", label, new_string_counter, string_value->length());
        }
      } break;
    }
}

void LabelControl::reserve(size_t reserved_size) {

  if ((control_flags & CONTROL_FLAG_STRING) && (string_value))
    string_value->reserve(reserved_size);
}
 
Control::Control(Control::ControlId_t id, Control::Type type, const char* label, void (*_callback)(BasicControl*, int, void*),
    const String& value, Control::Color color, bool visible, ControlId_t parentControl, void *_userData) :
LabelControl(id, type, label, value, color, visible, parentControl)
    
{    
    callback = _callback;
    userData = _userData;

    /*ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    
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
        new_string_counter++;
        log_i("new_string_counter = %lu", new_string_counter);
  if (value.length() < 16)
  string_value->reserve(16);

        (*string_value) = value;
      }
  
    }*/
}

Control::Control(Control::ControlId_t id, Control::Type type, const char* label, void (*_callback)(BasicControl*, int, void*),
    long value, Control::Color color, bool visible, ControlId_t parentControl, void *_userData) :
LabelControl(id, type, label, value, color, visible, parentControl)
    
{
        
    callback = _callback;
    userData = _userData;

    /*ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    
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
new_string_counter++;
        log_i("new_string_counter = %lu", new_string_counter);
        *string_value = value;
       }  
    }*/
}

Control::Control(Control::ControlId_t id, Control::Type type, const char* label, void (*_callback)(BasicControl*, int, void*),
    const char* value, Control::Color color, bool visible, ControlId_t parentControl, void *_userData) :
LabelControl(id, type, label, value, color, visible, parentControl)
    
{
    
    callback = _callback;
    userData = _userData;

    /*ControlChangeID = 1;
    control_flags = CONTROL_FLAG_ENABLED;
    if (visible) control_flags |= CONTROL_FLAG_VISIBLE;
    
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
  
    }*/
}


Control::Control(const Control& Control) :
LabelControl(noParent, Control.type, Control.label, (long)0, Control.color, true, Control.parentControl) {
 
  
  elementStyle = Control.elementStyle;
  panelStyle = Control.panelStyle;
  callback = Control.callback;
  userData = Control.userData;

if (Control.control_flags & CONTROL_FLAG_VISIBLE)
      control_flags |= CONTROL_FLAG_VISIBLE;

switch (Control.type) {


      case Option:
      case Number:
       
        numeric_value = Control.numeric_value;
      break;


      default: {

	if (Control.control_flags & CONTROL_FLAG_PCHAR)
      	  cstr_value = Control.cstr_value;
        else {
          
        string_value = new String();
        (*string_value) = (*Control.string_value);
        control_flags |= CONTROL_FLAG_STRING;

        new_string_counter++;
        log_i("Control& new_string_counter = %u, length %u", new_string_counter, string_value->length());

        }
      } break;
    }
}

void Control::SendCallback(int type)
{
    if(callback)
    {
        callback(this, type, userData);
    }
}

long BasicControl::getValueInt() {

  if (control_flags & CONTROL_FLAG_NUMERIC)
    return numeric_value;
  else
   return 0;
}


long LabelControl::getValueInt() {

  if (control_flags & CONTROL_FLAG_NUMERIC)
    return numeric_value;
  else
   if (string_value)
    return string_value->toInt();
  return 0;
}
    
const char *LabelControl::getValueCstr() {

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

const String &LabelControl::getValue() {

  if (control_flags & CONTROL_FLAG_NUMERIC)
    return emptyString;
  else
    return *string_value;
}


void BasicControl::DeleteControl()
{
    control_flags |= CONTROL_FLAG_DELETED;
}


void Control::DeleteControl()
{
    control_flags |= CONTROL_FLAG_DELETED;
    callback = nullptr;
}


bool BasicControl::MarshalControl(JsonObject & _item,
                             bool refresh,
                             uint32_t StartingOffset,
                             uint32_t AvailMarshaledLength,
                             uint32_t &EstimatedMarshaledLength)
{
    
    bool ControlIsFragmented = false;
    // create a new item in the response document
    JsonObject & item = _item;


    if (control_flags & CONTROL_FLAG_NUMERIC)
      global_value = numeric_value;
   
    
    // how much space do we expect to use?
    uint32_t ValueMarshaledLength   = (global_value.length() - StartingOffset) * JsonMarshalingRatio;
    uint32_t LabelMarshaledLength   = strlen(label) * JsonMarshalingRatio;
    uint32_t MinimumMarshaledLength = LabelMarshaledLength + JsonMarshaledOverhead;
    uint32_t MaximumMarshaledLength = ValueMarshaledLength + MinimumMarshaledLength;
    uint32_t SpaceForMarshaledValue = AvailMarshaledLength - MinimumMarshaledLength;
    
    // will the item fit in the remaining space? Fragment if not
    if (AvailMarshaledLength < MinimumMarshaledLength)
    {
        EstimatedMarshaledLength = 0;
        return false;
    }

    uint32_t MaxValueLength = (SpaceForMarshaledValue / JsonMarshalingRatio);
    
    uint32_t ValueLenToSend = min((global_value.length() - StartingOffset), MaxValueLength);
    
    uint32_t AdjustedMarshaledLength = (ValueLenToSend * JsonMarshalingRatio) + MinimumMarshaledLength;
    
    bool NeedToFragment = (ValueLenToSend < global_value.length());
  

    if ((AdjustedMarshaledLength > AvailMarshaledLength) && (0 != ValueLenToSend))
    {
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
    Type TempType = (Type::Password == type) ? Type::Text : type;
    if(refresh)
    {
        item[F("type")] = uint32_t(TempType) + uint32_t(Type::UpdateOffset);
    }
    else
    {
        item[F("type")] = uint32_t(TempType);
    }

    item[F("label")]   = label;
    global_value = (Type::Password == type) ? 
	F ("--------") : global_value.substring(StartingOffset, StartingOffset + ValueLenToSend);
    item[F ("value")]  = global_value;
    item[F("visible")] = control_flags & CONTROL_FLAG_VISIBLE;
    item[F("color")]   = (int)color;
    item[F("enabled")] = control_flags & CONTROL_FLAG_ENABLED;

    
    if (Type::Option == type) {

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
    if ((Type::Option != type) && (inputType)) 
      {item[F("inputType")]     = inputType;}
    if (control_flags & CONTROL_FLAG_WIDE) {item[F("wide")] = true;}
    if (control_flags & CONTROL_FLAG_VERTICAL) {item[F("vertical")] = true;}
    if (parentControl != Control::noParent)
    {
        item[F("parentControl")] = parentControl;
    }

    // special case for selects: to preselect an option, you have to add
    // "selected" to <option>
    if (Type::Option == type)
    {
        BasicControl* ParentControl = ESPUI.getControlNoLock(parentControl);
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


bool LabelControl::MarshalControl(JsonObject & _item,
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
      global_value = *string_value; //string_value->length();

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
        Control* ParentControl = static_cast<Control *>(ESPUI.getControlNoLock(parentControl));
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

void BasicControl::MarshalErrorMessage(JsonObject & item)
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

void BasicControl::onWsEvent(String & cmd, String& data)
{
    do // once
    {
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
        
    } while (false);
}

void Control::onWsEvent(String & cmd, String& data) {

  do // once
    {
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
            numeric_value = 1;
            SendCallback(S_ACTIVE);
        }
        else if (cmd.equals(F("sinactive")))
        {
         
            numeric_value = 0;
            SendCallback(S_INACTIVE);
        }
        else if (cmd.equals(F("slvalue")))
        {
            numeric_value = data.toInt();
            SendCallback(SL_VALUE);
        }
        else if (cmd.equals(F("nvalue")))
        {
            numeric_value = data.toInt();
            SendCallback(N_VALUE);
        }
        else if (cmd.equals(F("tabvalue")))
        {
            SendCallback(0);
        }
        else if (cmd.equals(F("svalue")))
        {
            numeric_value = data.toInt();
            SendCallback(S_VALUE);
        }
        else if (cmd.equals(F("tvalue")))
        {
            *string_value = data;
            SendCallback(T_VALUE);
        }
        else if (cmd.equals(F("time")))
        {
            *string_value = data;
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
