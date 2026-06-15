#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

#define CONTROL_FLAG_DELETED    (1 << 4)
#define CONTROL_FLAG_NUMERIC    (1 << 5)
#define CONTROL_FLAG_PCHAR      (1 << 6)
#define CONTROL_FLAG_STRING     (1 << 7)


#define BASIC_CONTROL_ID        0x01
#define LABEL_CONTROL_ID	0x02
#define STANDARD_CONTROL_ID     0x03
#define STRING_CONTROL_ID	0x04


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

// multiplier for converting a typical controller label or value to a Json object
    #define JsonMarshalingRatio 5
    // Marshaed Control overhead length
    #define JsonMarshaledOverhead 64


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
    const char *cstr_value = nullptr;
    String *string_value;
};
    uint32_t ControlChangeID = 0;
union {
    const char *inputType = nullptr;
    int32_t dynamic_option_id;
struct {
    uint16_t secondParent;
    uint16_t thirdParent;
};
};

    ControlId_t id = noParent;
    uint16_t control_flags;
    Color color;
    Type type = Type::Title;
    ControlId_t parentControl;
    BasicControl *next = nullptr;
    

    BasicControl(ControlId_t id,
            Type type,
            const char* label,
            long value,
            Color color,
            bool visible,
            ControlId_t parentControl);

     BasicControl(const BasicControl& BasicControl);

    virtual uint32_t getClassId() {return BASIC_CONTROL_ID;}

    virtual void reserve(size_t reserved_size) {};
    virtual void SendCallback(int type) {};
    virtual bool HasCallback() { return false; }
    virtual void setCallback(void (*callback_ptr)(BasicControl*, int, void*)) {}
    virtual bool MarshalControl(ArduinoJson::JsonObject& item, bool refresh, uint32_t DataOffset, uint32_t MaxLength, uint32_t & EstimmatedUsedSpace);
    void MarshalErrorMessage(ArduinoJson::JsonObject& item);
    virtual void DeleteControl();
    virtual void onWsEvent(String& cmd, String& data);
    inline bool ToBeDeleted() { return control_flags & CONTROL_FLAG_DELETED; }
    inline bool SkipSend() { return ((type == Type::Option) && (control_flags & CONTROL_FLAG_NUMERIC) && (numeric_value == -3));}
    inline bool NeedsSync(uint32_t lastControlChangeID) {return (!(control_flags & CONTROL_FLAG_DELETED)) && (lastControlChangeID < ControlChangeID);}
    void SetControlChangedId(uint32_t value) {ControlChangeID = value;}
    inline ControlId_t GetId() {return id;}
    inline Type        GetType() {return type;}
    virtual long getValueInt();
    virtual const char *getValueCstr() {return nullptr;}
    virtual const String &getValue() {return emptyString;}


private:
};

class LabelControl: public BasicControl {

public:


    const char *panelStyle = nullptr;
    const char *elementStyle = nullptr;

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

    LabelControl(const LabelControl& Control);

    uint32_t getClassId() override {return LABEL_CONTROL_ID;}
    void reserve(size_t reserved_size) override;
    bool MarshalControl(ArduinoJson::JsonObject& item, bool refresh, uint32_t DataOffset, uint32_t MaxLength, uint32_t & EstimmatedUsedSpace) override;
    long getValueInt() override;
    const char *getValueCstr() override;
    const String &getValue() override;

private:
};


class Control: public LabelControl {

public:

    void (*callback)(BasicControl*, int, void *) = nullptr;
    void *userData = nullptr;


    Control(ControlId_t id,
            Type type,
            const char* label,
            //std::function<void(BasicControl*, int)> callback,
            void (*callback)(BasicControl*, int, void*),
            const String& value,
            Color color,
            bool visible,
            uint16_t parentControl,
            void *userData = nullptr);

    Control(ControlId_t id,
            Type type,
            const char* label,
            //std::function<void(BasicControl*, int)> callback,
	    void (*callback)(BasicControl*, int, void*),
            long value,
            Color color,
            bool visible,
            uint16_t parentControl,
            void *userData = nullptr);

    Control(ControlId_t id,
            Type type,
            const char* label,
            //std::function<void(BasicControl*, int)> callback,
            void (*callback)(BasicControl*, int, void*),
            const char* value,
            Color color,
            bool visible,
            uint16_t parentControl,
            void *userData = nullptr);

    Control(const Control& Control);

   uint32_t getClassId() override {return STANDARD_CONTROL_ID;}


    void setCallback(void (*callback_ptr)(BasicControl*, int, void*)) override { callback = callback_ptr; }
    void SendCallback(int type) override;
    bool HasCallback() override { return (nullptr != callback); }
    //bool MarshalControl(ArduinoJson::JsonObject& item, bool refresh, uint32_t DataOffset, uint32_t MaxLength, uint32_t & EstimmatedUsedSpace) override;
    void DeleteControl() override;
    void onWsEvent(String& cmd, String& data) override;
    //long getValueInt() override;
    //const char *getValueCstr() override;
    //const String &getValue() override;

private:
};

