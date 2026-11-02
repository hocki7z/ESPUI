#include "ESPUI.h"

static const String ControlError = "*** ESPUI ERROR: Could not transfer control ***";


//static String global_value;
//static bool global_value_reserve = true;
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
	/*if (global_value_reserve) {
         global_value_reserve = false;
         global_value.reserve(1024);
       }*/
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
    JsonObject item = _item;

    char num_buf[16];
    const char* val_ptr = "";
    size_t val_len = 0;

    if (Type::Password == type) {
        val_ptr = "--------";
        val_len = 8;
    } else if (control_flags & CONTROL_FLAG_NUMERIC) {
        ltoa(numeric_value, num_buf, 10);
        val_ptr = num_buf;
        val_len = strlen(num_buf);
    }

    uint32_t rem_len = (val_len > StartingOffset) ? (val_len - StartingOffset) : 0;
    uint32_t LabelMarshaledLength   = strlen(label) * JsonMarshalingRatio;
    uint32_t MinimumMarshaledLength = LabelMarshaledLength + JsonMarshaledOverhead;

    if (AvailMarshaledLength < MinimumMarshaledLength)
    {
        EstimatedMarshaledLength = 0;
        return false;
    }

    uint32_t SpaceForMarshaledValue = AvailMarshaledLength - MinimumMarshaledLength;
    uint32_t MaxValueLength = (SpaceForMarshaledValue / JsonMarshalingRatio);
    uint32_t ValueLenToSend = min(rem_len, MaxValueLength);
    uint32_t AdjustedMarshaledLength = (ValueLenToSend * JsonMarshalingRatio) + MinimumMarshaledLength;
    bool NeedToFragment = (ValueLenToSend < rem_len);

    if ((AdjustedMarshaledLength > AvailMarshaledLength) && (0 != ValueLenToSend))
    {
        EstimatedMarshaledLength = 0;
        return false;
    }

    EstimatedMarshaledLength = AdjustedMarshaledLength;

    if (NeedToFragment || StartingOffset)
    {
        ControlIsFragmented = true;
        _item[F("type")] = uint32_t(Control::Type::Fragment);
        _item[F("id")]   = id;
        _item[F("offset")] = StartingOffset;
        _item[F("length")] = ValueLenToSend;
        _item[F("total")]  = val_len;
        AllocateNamedJsonObject(item, _item, F("control"));
    }

    item[F("id")] = id;
    Type TempType = (Type::Password == type) ? Type::Text : type;
    item[F("type")] = refresh ? (uint32_t(TempType) + uint32_t(Type::UpdateOffset)) : uint32_t(TempType);
    item[F("label")] = label;

    if (ControlIsFragmented && Type::Password != type) {
        // Only copy substring when actually fragmenting
        String frag_str;
        frag_str.reserve(ValueLenToSend + 1);
        frag_str.concat(val_ptr + StartingOffset, ValueLenToSend);
        item[F("value")] = frag_str;
    } else if (control_flags & CONTROL_FLAG_NUMERIC) {
        // Copy the short stack string (num_buf) into JsonDocument
        item[F("value")] = (char*)num_buf;
    } else {
        // Zero-copy const char* literal
        item[F("value")] = val_ptr;
    }

    item[F("visible")] = (bool)(control_flags & CONTROL_FLAG_VISIBLE);
    item[F("color")]   = (int)color;
    item[F("enabled")] = (bool)(control_flags & CONTROL_FLAG_ENABLED);

    if (Type::Option == type) {
        if (secondParent > 0) { item[F("secondParent")] = secondParent; }
        if (thirdParent > 0)  { item[F("thirdParent")]  = thirdParent; }
    }

    if (control_flags & CONTROL_FLAG_OPTION_UPDATE) {
        item[F("optionMode")] = 1;
        item[F("dOptionId")]  = dynamic_option_id;
    } else if (control_flags & CONTROL_FLAG_OPTION_ADD) {
        item[F("optionMode")] = 2;
        item[F("dOptionId")]  = dynamic_option_id;
    } else if (control_flags & CONTROL_FLAG_OPTION_REMOVE) {
        item[F("optionMode")] = 3;
        item[F("dOptionId")]  = dynamic_option_id;
    } else if (control_flags & CONTROL_FLAG_TEXTAREA) {
        item[F("textarea")] = true;
        if (rows > 0) item[F("rows")] = rows;
        if (cols > 0) item[F("cols")] = cols;
    } else if ((Control::Type::Option != type) && (inputType)) {
        item[F("inputType")] = inputType;
    }

    if (control_flags & CONTROL_FLAG_WIDE)     { item[F("wide")]     = true; }
    if (control_flags & CONTROL_FLAG_VERTICAL) { item[F("vertical")] = true; }
    if (parentControl != Control::noParent)    { item[F("parentControl")] = parentControl; }

    if (Type::Option == type)
    {
        BasicControl* ParentControl = ESPUI.getControlNoLock(parentControl);
        if (nullptr != ParentControl && ParentControl->numeric_value == numeric_value) {
            item[F("selected")] = F("selected");
        } else {
            item[F("selected")] = "";
        }
    }

    return ControlIsFragmented;
}


bool LabelControl::MarshalControl(JsonObject & _item,
                             bool refresh,
                             uint32_t StartingOffset,
                             uint32_t AvailMarshaledLength,
                             uint32_t &EstimatedMarshaledLength)
{
    bool ControlIsFragmented = false;
    JsonObject item = _item;

    char num_buf[16];
    const char* val_ptr = "";
    size_t val_len = 0;

    if (Control::Type::Password == type) {
        val_ptr = "--------";
        val_len = 8;
    } else if (control_flags & CONTROL_FLAG_NUMERIC) {
        ltoa(numeric_value, num_buf, 10);
        val_ptr = num_buf;
        val_len = strlen(num_buf);
    } else if (control_flags & CONTROL_FLAG_PCHAR) {
        val_ptr = cstr_value ? cstr_value : "";
        val_len = strlen(val_ptr);
    } else if (string_value) {
        val_ptr = string_value->c_str();
        val_len = string_value->length();
    }

    uint32_t rem_len = (val_len > StartingOffset) ? (val_len - StartingOffset) : 0;
    uint32_t LabelMarshaledLength   = strlen(label) * JsonMarshalingRatio;
    uint32_t MinimumMarshaledLength = LabelMarshaledLength + JsonMarshaledOverhead;

    if (AvailMarshaledLength < MinimumMarshaledLength)
    {
        EstimatedMarshaledLength = 0;
        return false;
    }

    uint32_t SpaceForMarshaledValue = AvailMarshaledLength - MinimumMarshaledLength;
    uint32_t MaxValueLength = (SpaceForMarshaledValue / JsonMarshalingRatio);
    uint32_t ValueLenToSend = min(rem_len, MaxValueLength);
    uint32_t AdjustedMarshaledLength = (ValueLenToSend * JsonMarshalingRatio) + MinimumMarshaledLength;
    bool NeedToFragment = (ValueLenToSend < rem_len);

    if ((AdjustedMarshaledLength > AvailMarshaledLength) && (0 != ValueLenToSend))
    {
        EstimatedMarshaledLength = 0;
        return false;
    }

    EstimatedMarshaledLength = AdjustedMarshaledLength;

    if (NeedToFragment || StartingOffset)
    {
        ControlIsFragmented = true;
        _item[F("type")]   = uint32_t(Control::Type::Fragment);
        _item[F("id")]     = id;
        _item[F("offset")] = StartingOffset;
        _item[F("length")] = ValueLenToSend;
        _item[F("total")]  = val_len;
        AllocateNamedJsonObject(item, _item, F("control"));
    }

    item[F("id")] = id;
    Control::Type TempType = (Control::Type::Password == type) ? Control::Type::Text : type;
    item[F("type")] = refresh ? (uint32_t(TempType) + uint32_t(Control::Type::UpdateOffset)) : uint32_t(TempType);
    item[F("label")] = label;

   if (ControlIsFragmented && Control::Type::Password != type) {
        String frag_str;
        frag_str.reserve(ValueLenToSend + 1);
        frag_str.concat(val_ptr + StartingOffset, ValueLenToSend);
        item[F("value")] = frag_str;
    } else if (control_flags & CONTROL_FLAG_NUMERIC) {
        item[F("value")] = (char*)num_buf; // Copied into JsonDocument
    } else if (control_flags & CONTROL_FLAG_PCHAR) {
        item[F("value")] = val_ptr;        // Zero-copy (const char* static literal)
    } else {
        // Dynamic String (like channel info label): cast to (char*) so ArduinoJson
        // copies it safely while ControlsSemaphore is locked!
        item[F("value")] = (char*)val_ptr;
    }

    item[F("visible")] = (bool)(control_flags & CONTROL_FLAG_VISIBLE);
    item[F("color")]   = (int)color;
    item[F("enabled")] = (bool)(control_flags & CONTROL_FLAG_ENABLED);

    if (panelStyle)   { item[F("panelStyle")]   = panelStyle; }
    if (elementStyle) { item[F("elementStyle")] = elementStyle; }

    if (Control::Type::Option == type) {
        if (secondParent > 0) { item[F("secondParent")] = secondParent; }
        if (thirdParent > 0)  { item[F("thirdParent")]  = thirdParent; }
    }

    if (control_flags & CONTROL_FLAG_OPTION_UPDATE) {
        item[F("optionMode")] = 1;
        item[F("dOptionId")]  = dynamic_option_id;
    } else if (control_flags & CONTROL_FLAG_OPTION_ADD) {
        item[F("optionMode")] = 2;
        item[F("dOptionId")]  = dynamic_option_id;
    } else if (control_flags & CONTROL_FLAG_OPTION_REMOVE) {
        item[F("optionMode")] = 3;
        item[F("dOptionId")]  = dynamic_option_id;
    } else if (control_flags & CONTROL_FLAG_TEXTAREA) {
        item[F("textarea")] = true;
        if (rows > 0) item[F("rows")] = rows;
        if (cols > 0) item[F("cols")] = cols;
    } else if ((Control::Type::Option != type) && (inputType)) {
        item[F("inputType")] = inputType;
    }

    if (control_flags & CONTROL_FLAG_WIDE)     { item[F("wide")]     = true; }
    if (control_flags & CONTROL_FLAG_VERTICAL) { item[F("vertical")] = true; }
    if (parentControl != Control::noParent)    { item[F("parentControl")] = parentControl; }

    if (Control::Type::Option == type)
    {
        BasicControl* ParentControl = ESPUI.getControlNoLock(parentControl);
        if (nullptr != ParentControl && ParentControl->numeric_value == numeric_value) {
            item[F("selected")] = F("selected");
        } else {
            item[F("selected")] = "";
        }
    }

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

void BasicControl::onWsEvent(const char* cmd, const char* data)
{
    SetControlChangedId(ESPUI.GetNextControlChangeId());
}


void Control::onWsEvent(const char* cmd, const char* data)
{
    if (!HasCallback() || !cmd || !data)
    {
        return;
    }

    if (strcmp_P(cmd, PSTR("bdown")) == 0)          { SendCallback(B_DOWN); }
    else if (strcmp_P(cmd, PSTR("bup")) == 0)       { SendCallback(B_UP); }
    else if (strcmp_P(cmd, PSTR("pfdown")) == 0)    { SendCallback(P_FOR_DOWN); }
    else if (strcmp_P(cmd, PSTR("pfup")) == 0)      { SendCallback(P_FOR_UP); }
    else if (strcmp_P(cmd, PSTR("pldown")) == 0)    { SendCallback(P_LEFT_DOWN); }
    else if (strcmp_P(cmd, PSTR("plup")) == 0)      { SendCallback(P_LEFT_UP); }
    else if (strcmp_P(cmd, PSTR("prdown")) == 0)    { SendCallback(P_RIGHT_DOWN); }
    else if (strcmp_P(cmd, PSTR("prup")) == 0)      { SendCallback(P_RIGHT_UP); }
    else if (strcmp_P(cmd, PSTR("pbdown")) == 0)    { SendCallback(P_BACK_DOWN); }
    else if (strcmp_P(cmd, PSTR("pbup")) == 0)      { SendCallback(P_BACK_UP); }
    else if (strcmp_P(cmd, PSTR("pcdown")) == 0)    { SendCallback(P_CENTER_DOWN); }
    else if (strcmp_P(cmd, PSTR("pcup")) == 0)      { SendCallback(P_CENTER_UP); }
    else if (strcmp_P(cmd, PSTR("sactive")) == 0)   { numeric_value = 1; SendCallback(S_ACTIVE); }
    else if (strcmp_P(cmd, PSTR("sinactive")) == 0) { numeric_value = 0; SendCallback(S_INACTIVE); }
    else if (strcmp_P(cmd, PSTR("slvalue")) == 0)   { numeric_value = strtol(data, nullptr, 10); SendCallback(SL_VALUE); }
    else if (strcmp_P(cmd, PSTR("nvalue")) == 0)    { numeric_value = strtol(data, nullptr, 10); SendCallback(N_VALUE); }
    else if (strcmp_P(cmd, PSTR("tabvalue")) == 0)  { SendCallback(0); }
    else if (strcmp_P(cmd, PSTR("svalue")) == 0)    { numeric_value = strtol(data, nullptr, 10); SendCallback(S_VALUE); }
    else if (strcmp_P(cmd, PSTR("tvalue")) == 0 || strcmp_P(cmd, PSTR("time")) == 0)
    {
        // Union safety: ensure string_value is a valid heap String* before assigning!
        if ((control_flags & CONTROL_FLAG_PCHAR) || !(control_flags & CONTROL_FLAG_STRING) || !string_value)
        {
            control_flags &= ~CONTROL_FLAG_PCHAR;
            control_flags |= CONTROL_FLAG_STRING;
            string_value = new String(data);
        }
        else
        {
            *string_value = data;
        }
        SendCallback((cmd[1] == 'v') ? T_VALUE : TM_VALUE);
    }
}