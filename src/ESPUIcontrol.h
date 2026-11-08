#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

#ifndef CONTROL_FLAG_ENABLED
#define CONTROL_FLAG_ENABLED       (1 << 0)
#define CONTROL_FLAG_VISIBLE       (1 << 1)
#define CONTROL_FLAG_WIDE          (1 << 2)
#define CONTROL_FLAG_VERTICAL      (1 << 3)
#define CONTROL_FLAG_DELETED       (1 << 4)
#define CONTROL_FLAG_NUMERIC       (1 << 5)
#define CONTROL_FLAG_PCHAR         (1 << 6)
#define CONTROL_FLAG_STRING        (1 << 7)
#define CONTROL_FLAG_TEXTAREA      (1 << 8)
#define CONTROL_FLAG_OPTION_UPDATE (1 << 13)
#define CONTROL_FLAG_OPTION_ADD    (1 << 14)
#define CONTROL_FLAG_OPTION_REMOVE (1 << 15)
#endif

#define BASIC_CONTROL_ID        0x01
#define LABEL_CONTROL_ID        0x02
#define STANDARD_CONTROL_ID     0x03
#define STRING_CONTROL_ID       0x04

#define UI_TITLE            Control::Type::Title
#define UI_LABEL            Control::Type::Label
#define UI_BUTTON           Control::Type::Button
#define UI_SWITCHER         Control::Type::Switcher
#define UI_PAD              Control::Type::Pad
#define UI_CPAD             Control::Type::Cpad
#define UI_SLIDER           Control::Type::Slider
#define UI_NUMBER           Control::Type::Number
#define UI_TEXT_INPUT       Control::Type::Text
#define UI_GRAPH            Control::Type::Graph
#define UI_ADD_GRAPH_POINT  Control::Type::GraphPoint

#define UI_DETAILS          Control::Type::Details

#define UPDATE_LABEL        Control::Type::UpdateLabel
#define UPDATE_SWITCHER     Control::Type::UpdateSwitcher
#define UPDATE_SLIDER       Control::Type::UpdateSlider
#define UPDATE_NUMBER       Control::Type::UpdateNumber
#define UPDATE_TEXT_INPUT   Control::Type::UpdateText
#define CLEAR_GRAPH         Control::Type::ClearGraph

// Colors
#define COLOR_TURQUOISE     Control::Color::Turquoise
#define COLOR_EMERALD       Control::Color::Emerald
#define COLOR_PETERRIVER    Control::Color::Peterriver
#define COLOR_WETASPHALT    Control::Color::Wetasphalt
#define COLOR_SUNFLOWER     Control::Color::Sunflower
#define COLOR_CARROT        Control::Color::Carrot
#define COLOR_ALIZARIN      Control::Color::Alizarin
#define COLOR_DARK          Control::Color::Dark
#define COLOR_NONE          Control::Color::None

// Multiplier for converting a typical controller label or value to a JSON object
#define JsonMarshalingRatio 2
// Marshaled Control overhead length
#define JsonMarshaledOverhead 64

// Single-block contiguous dynamic string (replaces heap-fragmenting String*)
struct DynString
{
    uint16_t capacity;
    uint16_t length;
    char     data[];
};

class BasicControl
{
public:
    typedef uint16_t ControlId_t;

    enum Type : uint8_t
    {
        // fixed Controls
        Title = 0,

        // updatable Controls
        Pad,
        PadWithCenter,
        Button,
        Label,
        Switcher,
        Slider,
        Number,
        Text,
        Graph,
        GraphPoint,
        Tab,
        Select,
        Option,
        Min,
        Max,
        Step,
        Gauge,
        Accel,
        Separator,
        Time,
        FileDisplay,

        ColorPickr,     // = 22,    #RRGGBB text input with an offline Pickr popup
        Details,        // = 23,    HTML details disclosure element

        Fragment = 98,
        Password = 99,
        UpdateOffset = 100,
    };

    enum Color : uint8_t
    {
        Turquoise,
        Emerald,
        Peterriver,
        Wetasphalt,
        Sunflower,
        Carrot,
        Alizarin,
        Dark,
        None = 0xFF
    };

    static constexpr uint16_t noParent = 0xffff;

    const char* label;
    union {
        long numeric_value;
        const char* cstr_value = nullptr;
        DynString* dyn_str;
    };
    uint32_t ControlChangeID = 0;
    union {
        const char* inputType = nullptr;
        int32_t dynamic_option_id;
        struct {
            uint16_t secondParent;
            uint16_t thirdParent;
        };
        struct {
            uint16_t rows;
            uint16_t cols;
        };
    };

    ControlId_t id = noParent;
    uint16_t control_flags = 0;
    Color color;
    Type type = Type::Title;
    ControlId_t parentControl;
    BasicControl* next = nullptr;

    bool detailsExpanded = false;

    BasicControl(ControlId_t id,
                 Type type,
                 const char* label,
                 long value,
                 Color color,
                 bool visible,
                 ControlId_t parentControl);

    BasicControl(const BasicControl&) = delete;
    BasicControl& operator=(const BasicControl&) = delete;

    virtual ~BasicControl();

    void setDynString(const char* str, size_t len);
    void setDynString(const char* str);
    void freeDynString();

    virtual uint32_t getClassId() { return BASIC_CONTROL_ID; }

    virtual void reserve(size_t reserved_size) {}
    virtual void SendCallback(int type) {}
    virtual bool HasCallback() { return false; }
    virtual void setCallback(void (*callback_ptr)(BasicControl*, int, void*)) {}
    virtual void setUserData(void* _userData) {}
    virtual const char* getPanelStyle() const { return nullptr; }
    virtual const char* getElementStyle() const { return nullptr; }

    virtual bool MarshalControl(ArduinoJson::JsonObject& item, bool refresh, uint32_t DataOffset, uint32_t MaxLength, uint32_t& EstimatedUsedSpace);
    void MarshalErrorMessage(ArduinoJson::JsonObject& item);
    virtual void DeleteControl();
    virtual void onWsEvent(const char* cmd, const char* data);

    inline bool ToBeDeleted() const { return (control_flags & CONTROL_FLAG_DELETED) != 0; }
    inline bool SkipSend() const { return ((type == Type::Option) && (control_flags & CONTROL_FLAG_NUMERIC) && (numeric_value == -3)); }
    inline bool NeedsSync(uint32_t lastControlChangeID) const { return (!(control_flags & CONTROL_FLAG_DELETED)) && (lastControlChangeID < ControlChangeID); }
    void SetControlChangedId(uint32_t value) { ControlChangeID = value; }
    inline ControlId_t GetId() const { return id; }
    inline Type GetType() const { return type; }

    virtual long getValueInt();
    virtual const char* getValueCstr() { return nullptr; }
    virtual String getValue()
    {
        const char* s = getValueCstr();
        return s ? String(s) : emptyString;
    }
};

class LabelControl : public BasicControl
{
public:
    const char* panelStyle = nullptr;
    const char* elementStyle = nullptr;

    LabelControl(ControlId_t id,
                 Type type,
                 const char* label,
                 const String& value,
                 Color color,
                 bool visible,
                 uint16_t parentControl);

    LabelControl(ControlId_t id,
                 Type type,
                 const char* label,
                 long value,
                 Color color,
                 bool visible,
                 uint16_t parentControl);

    LabelControl(ControlId_t id,
                 Type type,
                 const char* label,
                 const char* value,
                 Color color,
                 bool visible,
                 uint16_t parentControl);

    uint32_t getClassId() override { return LABEL_CONTROL_ID; }
    void reserve(size_t reserved_size) override;
    const char* getPanelStyle() const override { return panelStyle; }
    const char* getElementStyle() const override { return elementStyle; }
    long getValueInt() override;
    const char* getValueCstr() override;
};

class Control : public LabelControl
{
public:
    void (*callback)(BasicControl*, int, void*) = nullptr;
    void* userData = nullptr;

    Control(ControlId_t id,
            Type type,
            const char* label,
            void (*callback)(BasicControl*, int, void*),
            const String& value,
            Color color,
            bool visible,
            uint16_t parentControl,
            void* userData = nullptr);

    Control(ControlId_t id,
            Type type,
            const char* label,
            void (*callback)(BasicControl*, int, void*),
            long value,
            Color color,
            bool visible,
            uint16_t parentControl,
            void* userData = nullptr);

    Control(ControlId_t id,
            Type type,
            const char* label,
            void (*callback)(BasicControl*, int, void*),
            const char* value,
            Color color,
            bool visible,
            uint16_t parentControl,
            void* userData = nullptr);

    uint32_t getClassId() override { return STANDARD_CONTROL_ID; }

    void setCallback(void (*callback_ptr)(BasicControl*, int, void*)) override { callback = callback_ptr; }
    void setUserData(void* _userData) override { userData = _userData; }
    void SendCallback(int type) override;
    bool HasCallback() override { return (nullptr != callback); }
    void DeleteControl() override;
    void onWsEvent(const char* cmd, const char* data) override;
};