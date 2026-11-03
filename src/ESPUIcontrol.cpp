#include "ESPUI.h"

static const char ControlError[] PROGMEM = "*** ESPUI ERROR: Could not transfer control ***";
static uint32_t new_string_counter = 0;

BasicControl::BasicControl(ControlId_t id, Type type, const char* label,
    long value, Color color, bool visible, ControlId_t parentControl)
    : label(label),
      ControlChangeID(1),
      id(id),
      control_flags(CONTROL_FLAG_ENABLED | (visible ? CONTROL_FLAG_VISIBLE : 0)),
      color(color),
      type(type),
      parentControl(parentControl)
{
    switch (type)
    {
        case Option:
        case Number:
        case Select:
        case Switcher:
        case Slider:
        case Min:
        case Max:
        case Step:
        case Gauge:
            control_flags |= CONTROL_FLAG_NUMERIC;
            numeric_value = value;
            break;

        default:
            cstr_value = nullptr;
            break;
    }
}

BasicControl::~BasicControl()
{
    freeDynString();
}

void BasicControl::freeDynString()
{
    if ((control_flags & CONTROL_FLAG_STRING) && dyn_str)
    {
        free(dyn_str);
        dyn_str = nullptr;
    }
    control_flags &= ~CONTROL_FLAG_STRING;
}

void BasicControl::setDynString(const char* str, size_t len)
{
    if (!str)
    {
        str = "";
        len = 0;
    }
    if (len > 0xFFFE)
    {
        len = 0xFFFE;
    }

    if (!(control_flags & CONTROL_FLAG_STRING) || !dyn_str)
    {
        uint16_t cap = (len < 15) ? 15 : (uint16_t)len;
        dyn_str = (DynString*)malloc(sizeof(DynString) + cap + 1);
        if (!dyn_str)
        {
            return;
        }
        dyn_str->capacity = cap;
        control_flags &= ~(CONTROL_FLAG_PCHAR | CONTROL_FLAG_NUMERIC);
        control_flags |= CONTROL_FLAG_STRING;

        new_string_counter++;
        log_i("label %s, new_string_counter = %u, length %u", label ? label : "", new_string_counter, (unsigned)len);
    }
    else if (len > dyn_str->capacity)
    {
        DynString* new_block = (DynString*)realloc(dyn_str, sizeof(DynString) + len + 1);
        if (!new_block)
        {
            return;
        }
        dyn_str = new_block;
        dyn_str->capacity = (uint16_t)len;
    }

    memcpy(dyn_str->data, str, len);
    dyn_str->data[len] = '\0';
    dyn_str->length = (uint16_t)len;
}

void BasicControl::setDynString(const char* str)
{
    setDynString(str, str ? strlen(str) : 0);
}

LabelControl::LabelControl(Control::ControlId_t id, Control::Type type, const char* label,
    const String& value, Control::Color color, bool visible, ControlId_t parentControl)
    : BasicControl(id, type, label, 0, color, visible, parentControl)
{
    switch (type)
    {
        case Option:
        case Number:
        case Select:
        case Switcher:
        case Slider:
        case Min:
        case Max:
        case Step:
        case Gauge:
            control_flags |= CONTROL_FLAG_NUMERIC;
            numeric_value = value.toInt();
            break;

        default:
            setDynString(value.c_str(), value.length());
            break;
    }
}

LabelControl::LabelControl(Control::ControlId_t id, Control::Type type, const char* label,
    long value, Control::Color color, bool visible, ControlId_t parentControl)
    : BasicControl(id, type, label, 0, color, visible, parentControl)
{
    switch (type)
    {
        case Option:
        case Number:
        case Select:
        case Switcher:
        case Slider:
        case Min:
        case Max:
        case Step:
        case Gauge:
            control_flags |= CONTROL_FLAG_NUMERIC;
            numeric_value = value;
            break;

        default:
        {
            char num_buf[16];
            ltoa(value, num_buf, 10);
            setDynString(num_buf);
            break;
        }
    }
}

LabelControl::LabelControl(Control::ControlId_t id, Control::Type type, const char* label,
    const char* value, Control::Color color, bool visible, ControlId_t parentControl)
    : BasicControl(id, type, label, 0, color, visible, parentControl)
{
    switch (type)
    {
        case Option:
        case Number:
        case Select:
        case Switcher:
        case Slider:
        case Min:
        case Max:
        case Step:
        case Gauge:
            control_flags |= CONTROL_FLAG_NUMERIC;
            numeric_value = value ? strtol(value, nullptr, 0) : 0;
            break;

        default:
            control_flags |= CONTROL_FLAG_PCHAR;
            cstr_value = value;
            break;
    }
}

void LabelControl::reserve(size_t reserved_size)
{
    if (reserved_size > 0xFFFE)
    {
        reserved_size = 0xFFFE;
    }

    if (!(control_flags & CONTROL_FLAG_STRING) || !dyn_str)
    {
        dyn_str = (DynString*)malloc(sizeof(DynString) + reserved_size + 1);
        if (!dyn_str)
        {
            return;
        }
        dyn_str->capacity = (uint16_t)reserved_size;
        dyn_str->length = 0;
        dyn_str->data[0] = '\0';
        control_flags &= ~(CONTROL_FLAG_PCHAR | CONTROL_FLAG_NUMERIC);
        control_flags |= CONTROL_FLAG_STRING;
    }
    else if (reserved_size > dyn_str->capacity)
    {
        DynString* new_block = (DynString*)realloc(dyn_str, sizeof(DynString) + reserved_size + 1);
        if (!new_block)
        {
            return;
        }
        dyn_str = new_block;
        dyn_str->capacity = (uint16_t)reserved_size;
    }
}

Control::Control(Control::ControlId_t id, Control::Type type, const char* label, void (*_callback)(BasicControl*, int, void*),
    const String& value, Control::Color color, bool visible, ControlId_t parentControl, void* _userData)
    : LabelControl(id, type, label, value, color, visible, parentControl),
      callback(_callback),
      userData(_userData)
{
}

Control::Control(Control::ControlId_t id, Control::Type type, const char* label, void (*_callback)(BasicControl*, int, void*),
    long value, Control::Color color, bool visible, ControlId_t parentControl, void* _userData)
    : LabelControl(id, type, label, value, color, visible, parentControl),
      callback(_callback),
      userData(_userData)
{
}

Control::Control(Control::ControlId_t id, Control::Type type, const char* label, void (*_callback)(BasicControl*, int, void*),
    const char* value, Control::Color color, bool visible, ControlId_t parentControl, void* _userData)
    : LabelControl(id, type, label, value, color, visible, parentControl),
      callback(_callback),
      userData(_userData)
{
}

void Control::SendCallback(int type)
{
    if (callback)
    {
        callback(this, type, userData);
    }
}

long BasicControl::getValueInt()
{
    if (control_flags & CONTROL_FLAG_NUMERIC)
    {
        return numeric_value;
    }
    return 0;
}

long LabelControl::getValueInt()
{
    if (control_flags & CONTROL_FLAG_NUMERIC)
    {
        return numeric_value;
    }
    const char* s = getValueCstr();
    return s ? strtol(s, nullptr, 10) : 0;
}

const char* LabelControl::getValueCstr()
{
    if (control_flags & CONTROL_FLAG_NUMERIC)
    {
        return nullptr;
    }
    if (control_flags & CONTROL_FLAG_PCHAR)
    {
        return cstr_value;
    }
    if ((control_flags & CONTROL_FLAG_STRING) && dyn_str)
    {
        return dyn_str->data;
    }
    return nullptr;
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

bool BasicControl::MarshalControl(JsonObject& _item,
                                  bool refresh,
                                  uint32_t StartingOffset,
                                  uint32_t AvailMarshaledLength,
                                  uint32_t& EstimatedMarshaledLength)
{
    bool ControlIsFragmented = false;
    JsonObject item = _item;

    char num_buf[16];
    const char* val_ptr = "";
    size_t val_len = 0;

    if (Type::Password == type)
    {
        val_ptr = "--------";
        val_len = 8;
    }
    else if (control_flags & CONTROL_FLAG_NUMERIC)
    {
        ltoa(numeric_value, num_buf, 10);
        val_ptr = num_buf;
        val_len = strlen(num_buf);
    }
    else if (control_flags & CONTROL_FLAG_PCHAR)
    {
        val_ptr = cstr_value ? cstr_value : "";
        val_len = strlen(val_ptr);
    }
    else if ((control_flags & CONTROL_FLAG_STRING) && dyn_str)
    {
        val_ptr = dyn_str->data;
        val_len = dyn_str->length;
    }

    const char* safe_label = label ? label : "";
    uint32_t rem_len = (val_len > StartingOffset) ? (val_len - StartingOffset) : 0;
    uint32_t LabelMarshaledLength   = strlen(safe_label) * JsonMarshalingRatio;
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
    Type TempType = (Type::Password == type) ? Type::Text : type;
    item[F("type")] = refresh ? (uint32_t(TempType) + uint32_t(Type::UpdateOffset)) : uint32_t(TempType);
    item[F("label")] = safe_label;

    if (ControlIsFragmented && Type::Password != type)
    {
        // Copy exact slice directly into JsonDocument without intermediate heap String allocation
        item[F("value")] = ArduinoJson::JsonString(val_ptr + StartingOffset, ValueLenToSend);
    }
    else if (control_flags & CONTROL_FLAG_NUMERIC)
    {
        // Copy stack buffer into JsonDocument
        item[F("value")] = (char*)num_buf;
    }
    else if (control_flags & CONTROL_FLAG_PCHAR)
    {
        // Zero-copy static const char* literal
        item[F("value")] = val_ptr;
    }
    else
    {
        // Dynamic string: copy into JsonDocument while ControlsSemaphore is locked
        item[F("value")] = (char*)val_ptr;
    }

    item[F("visible")] = (bool)(control_flags & CONTROL_FLAG_VISIBLE);
    item[F("color")]   = (int)color;
    item[F("enabled")] = (bool)(control_flags & CONTROL_FLAG_ENABLED);

    const char* pStyle = getPanelStyle();
    const char* eStyle = getElementStyle();
    if (pStyle) { item[F("panelStyle")]   = pStyle; }
    if (eStyle) { item[F("elementStyle")] = eStyle; }

    if (Type::Option == type)
    {
        if (secondParent > 0) { item[F("secondParent")] = secondParent; }
        if (thirdParent > 0)  { item[F("thirdParent")]  = thirdParent; }
    }

    if (control_flags & CONTROL_FLAG_OPTION_UPDATE)
    {
        item[F("optionMode")] = 1;
        item[F("dOptionId")]  = dynamic_option_id;
    }
    else if (control_flags & CONTROL_FLAG_OPTION_ADD)
    {
        item[F("optionMode")] = 2;
        item[F("dOptionId")]  = dynamic_option_id;
    }
    else if (control_flags & CONTROL_FLAG_OPTION_REMOVE)
    {
        item[F("optionMode")] = 3;
        item[F("dOptionId")]  = dynamic_option_id;
    }
    else if (control_flags & CONTROL_FLAG_TEXTAREA)
    {
        item[F("textarea")] = true;
        if (rows > 0) item[F("rows")] = rows;
        if (cols > 0) item[F("cols")] = cols;
    }
    else if ((Control::Type::Option != type) && (inputType))
    {
        item[F("inputType")] = inputType;
    }

    if (control_flags & CONTROL_FLAG_WIDE)     { item[F("wide")]     = true; }
    if (control_flags & CONTROL_FLAG_VERTICAL) { item[F("vertical")] = true; }
    if (parentControl != Control::noParent)    { item[F("parentControl")] = parentControl; }

    if (Type::Option == type)
    {
        BasicControl* ParentControl = ESPUI.getControlNoLock(parentControl);
        if (nullptr != ParentControl && ParentControl->numeric_value == numeric_value)
        {
            item[F("selected")] = F("selected");
        }
        else
        {
            item[F("selected")] = "";
        }
    }

    return ControlIsFragmented;
}

void BasicControl::MarshalErrorMessage(JsonObject& item)
{
    item[F("id")]      = id;
    item[F("type")]    = uint32_t(Control::Type::Label);
    item[F("label")]   = FPSTR(ControlError);
    item[F("value")]   = FPSTR(ControlError);
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
#ifdef ESP32
        xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
        setDynString(data);
#ifdef ESP32
        xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
        SendCallback((cmd[1] == 'v') ? T_VALUE : TM_VALUE);
    }
}