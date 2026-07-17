#include "ESPUI.h"

#include <functional>

#include <ESPAsyncWebServer.h>

#include "dataControlsJS.h"
#include "dataGraphJS.h"
#include "dataIndexHTML.h"
#include "dataNormalizeCSS.h"
#include "dataSliderJS.h"
#include "dataStyleCSS.h"
#include "dataTabbedcontentJS.h"
#include "dataZeptoJS.h"

#if ESP8266
#include <umm_malloc/umm_heap_select.h>
#endif
#include "ESPUIcontrolMgr.h"

static const char* emptyCstring = "";


// Optional user-defined JavaScript to be included in the UI.
// Served at /js/custom.js, which is automatically included in index.htm.
// js: JavaScript code as a C-string. Must remain valid for the lifetime of the ESPUIClass instance.
static const char* customJS = nullptr;

// Optional user-defined CSS to be included in the UI.
// Served at /css/custom.css, which is automatically included in index.htm.
// css: CSS code as a C-string. Must remain valid for the lifetime of the ESPUIClass instance.
static const char* customCSS = nullptr;

// Set custom JavaScript to be included in the UI.
// js: JavaScript code as a C-string. Must remain valid for the lifetime of the ESPUIClass instance.
void ESPUIClass::setCustomJS(const char* js)
{
    customJS = js;
    //log_i("%s\n\r%s",customJS, js); 
}

// Set custom CSS to be included in the UI.
// css: CSS code as a C-string. Must remain valid for the lifetime of the ESPUIClass instance.
void ESPUIClass::setCustomCSS(const char* css)
{
    customCSS = css;
}


static String heapInfo(const __FlashStringHelper* mode)
{
    String result;
#if ESP8266

    uint32_t hfree;
    uint16_t hmax;
    uint8_t hfrag;
    result.reserve(128);

#ifdef UMM_HEAP_IRAM
    // here esp8266 is configurerd to use an extra 16KB (i)ram
    {
        HeapSelectIram useInstructionRamHere;
        ESP.getHeapStats(&hfree, &hmax, &hfrag);
    }
    result += F("IRAM: free: ");
    result += hfree;
    result += F(" max: ");
    result += hmax;
    result += F(" frag: ");
    result += hfrag;
    result += "%\n";
#endif // !UMM_HEAP_IRAM
    {
        HeapSelectDram useRegularRamHere;
        ESP.getHeapStats(&hfree, &hmax, &hfrag);
    }
    result += F("DRAM: free: ");
    result += hfree;
    result += F(" max: ");
    result += hmax;
    result += F(" frag: ");
    result += hfrag;
    result += "%\n";

#else // !ESP8266

    result += ESP.getFreeHeap();
    result += ' ';

#endif // !ESP8266

    result += mode;

    return result;
}

// ################# LITTLEFS functions
#if defined(ESP32)
void listDir(const char* dirname, uint8_t levels)
{
#if defined(DEBUG_ESPUI)
    if (ESPUI.verbosity)
    {
        Serial.printf_P(PSTR("Listing directory: %s\n"), dirname);
    }
#endif

    File root = ESPUI.EspuiLittleFS.open(dirname);
    if (!root)
    {
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("Failed to open directory"));
        }
#endif

        return;
    }

    if (!root.isDirectory())
    {
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("Not a directory"));
        }
#endif

        return;
    }

    File file = root.openNextFile();

    while (file)
    {
        if (file.isDirectory())
        {
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.print(F("  DIR : "));
                Serial.println(file.name());
            }
#endif

            if (levels)
            {
#if (ESP_IDF_VERSION_MAJOR == 4 && ESP_IDF_VERSION_MINOR >= 4) || ESP_IDF_VERSION_MAJOR > 4
                listDir(file.path(), levels - 1);
#else
                listDir(file.name(), levels - 1);
#endif
            }
        }
        else
        {
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.print(F("  FILE: "));
                Serial.print(file.name());
                Serial.print(F("  SIZE: "));
                Serial.println(file.size());
            }
#endif
        }

        file = root.openNextFile();
    }
}
#else

void listDir(const char* dirname, uint8_t levels)
{
#if defined(DEBUG_ESPUI)
    if (ESPUI.verbosity)
    {
        Serial.printf_P(PSTR("Listing directory: %s\n"), dirname);
    }
#endif

    Dir dir = ESPUI.EspuiLittleFS.openDir(dirname);

    while (dir.next())
    {
        if (dir.isDirectory())
        {
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.print(F("  DIR : "));
                Serial.println(dir.fileName());
            }
#endif
            if (levels)
            {
                File file = dir.openFile("r");
                listDir(file.fullName(), levels - 1);
                file.close();
            }
        }
        else
        {
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.print(F("  FILE: "));
                Serial.print(dir.fileName());
                Serial.print(F("  SIZE: "));
                Serial.println(dir.fileSize());
            }
#endif
        }
    }
}

#endif

void ESPUIClass::list()
{
    if (!EspuiLittleFS.begin())
    {
        Serial.println(F("Espui LittleFS Mount Failed"));
        return;
    }

    listDir("/", 1);

#if defined(ESP32)
    Serial.print(F("Total KB: "));
    Serial.println(EspuiLittleFS.totalBytes() / 1024);
    Serial.print(F("Used KB: "));
    Serial.println(EspuiLittleFS.usedBytes() / 1024);
#else
    FSInfo fs_info;
    EspuiLittleFS.info(fs_info);

    Serial.print(F("Total KB: "));
    Serial.println(fs_info.totalBytes / 1024);
    Serial.print(F("Used KB: "));
    Serial.println(fs_info.usedBytes / 1024);
#endif // !defined(ESP32)
}

void deleteFile(const char* path)
{
    bool exists = ESPUI.EspuiLittleFS.exists(path);
    if (!exists)
    {
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.printf_P(PSTR("File: %s does not exist, not deleting\n"), path);
        }
#endif

        return;
    }

#if defined(DEBUG_ESPUI)
    if (ESPUI.verbosity)
    {
        Serial.printf_P(PSTR("Deleting file: %s\n"), path);
    }
#endif

    bool didRemove = ESPUI.EspuiLittleFS.remove(path);
    if (didRemove)
    {
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("File deleted"));
        }
#endif
    }
    else
    {
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("Delete failed"));
        }
#endif
    }
}

void ESPUIClass::writeFile(const char* path, const char* data)
{
#if defined(DEBUG_ESPUI)
    if (ESPUI.verbosity)
    {
        Serial.printf_P(PSTR("Writing file: %s\n"), path);
    }
#endif

    File file = EspuiLittleFS.open(path, FILE_WRITING);
    if (!file)
    {
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("Failed to open file for writing"));
        }
#endif

        return;
    }

#if defined(ESP32)
    if (file.print(data))
#else
    if (file.print(FPSTR(data)))
#endif // !defined(ESP32)

    {
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("File written"));
        }
    }
    else
    {
        if (ESPUI.verbosity)
        {
            Serial.println(F("Write failed"));
        }
#endif
    }
    file.close();
}

// end LITTLEFS functions

void ESPUIClass::prepareFileSystem(bool format)
{
    // this function should only be used once

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("About to prepare filesystem..."));
    }
#endif

#if defined(ESP32)
    if (!EspuiLittleFS.begin(false)) // Test for an already formatted LittleFS by a mount failure
    {
        if (!EspuiLittleFS.begin(true)) // Attempt to format LittleFS
        {
#else
    if (!EspuiLittleFS.begin()) // Test for an already formatted LittleFS by a mount failure
    {
        if (EspuiLittleFS.format()) // Attempt to format LittleFS
        {
#endif // !defined(ESP32)

#if defined(DEBUG_ESPUI)
            if (verbosity)
            {
                Serial.println(F("LittleFS Format Failed"));
            }
#endif
            return;
        }
    }
    else if (format)
    {
        EspuiLittleFS.format();

#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("LittleFS Formatted"));
        }
#endif
    }

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        listDir("/", 1);
        Serial.println(F("LittleFS Mount ESP32 Done"));
    }
#endif

    deleteFile("/index.htm");

    deleteFile("/css/style.css");
    deleteFile("/css/normalize.css");

    deleteFile("/js/zepto.min.js");
    deleteFile("/js/controls.js");
    deleteFile("/js/slider.js");
    deleteFile("/js/graph.js");
    deleteFile("/js/tabbedcontent.js");

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("Cleanup done"));
    }
#endif

    // Now write
#ifdef ESP32
    writeFile("/index.htm", HTML_INDEX);
    EspuiLittleFS.mkdir("/css");
    writeFile("/css/style.css", CSS_STYLE);
    writeFile("/css/normalize.css", CSS_NORMALIZE);
    EspuiLittleFS.mkdir("/js");
    writeFile("/js/zepto.min.js", JS_ZEPTO);
    writeFile("/js/controls.js", JS_CONTROLS);
    writeFile("/js/slider.js", JS_SLIDER);
    writeFile("/js/graph.js", JS_GRAPH);

    writeFile("/js/tabbedcontent.js", JS_TABBEDCONTENT);

#else
    writeFile("/index.htm", HTML_INDEX);

    writeFile("/css/style.css", CSS_STYLE);
    writeFile("/css/normalize.css", CSS_NORMALIZE);

    writeFile("/js/zepto.min.js", JS_ZEPTO);
    writeFile("/js/controls.js", JS_CONTROLS);
    writeFile("/js/slider.js", JS_SLIDER);
    writeFile("/js/graph.js", JS_GRAPH);

    writeFile("/js/tabbedcontent.js", JS_TABBEDCONTENT);
#endif

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("Done Initializing filesystem :-)"));
    }
#endif

#if defined(ESP32)

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        listDir("/", 1);
    }
#endif

#endif

    EspuiLittleFS.end();
}

// Handle Websockets Communication
void ESPUIClass::onWsEvent(
    AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    // Serial.println(String("ESPUIClass::OnWsEvent: type: ") + String(type));
    ESPUIcontrolMgr.RemoveToBeDeletedControls();

    if (WS_EVT_DISCONNECT == type)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("WS_EVT_DISCONNECT"));
        }
#endif

        if (MapOfClients.end() != MapOfClients.find(client->id()))
        {
            // Serial.println("Delete client.");
            delete MapOfClients[client->id()];
            MapOfClients.erase(client->id());
        }
    }
    else
    {
        if(type == WS_EVT_CONNECT)
        {
            ws->cleanupClients();
        }

        if (MapOfClients.end() == MapOfClients.find(client->id()))
        {
            // Serial.println("ESPUIClass::OnWsEvent:Create new client.");
            MapOfClients[client->id()] = new ESPUIclient(client);
        }

        if(MapOfClients[client->id()]->onWsEvent(type, arg, data, len))
        {
            // Serial.println("ESPUIClass::OnWsEvent:notify the clients that they need to be updated.");
            NotifyClients(ESPUIclient::UpdateNeeded);
        }
    }

    return;
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label)
{
    return addControl(type, label, (long)0);
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const String& value)
{
    return addControl(type, label, value, Control::Color::Turquoise);
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const char* value)
{
    return addControl(type, label, value, Control::Color::Turquoise);
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, long value)
{
    return addControl(type, label, value, Control::Color::Turquoise);
}


Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const String& value, Control::Color color)
{
    return addControl(type, label, value, color, Control::noParent);
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const char* value, Control::Color color)
{
    return addControl(type, label, value, color, Control::noParent);
}


Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, long value, Control::Color color)
{
    return addControl(type, label, value, color, Control::noParent);
}


Control::ControlId_t ESPUIClass::addControl(Control::Type type, 
const char* label, const String& value, Control::Color color, Control::ControlId_t parentControlId)
{
    return addControl(type, label, value, color, parentControlId, (void (*)(BasicControl*, int, void*))nullptr);
}


Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, long value, Control::Color color, Control::ControlId_t parentControlId)
{
    return addControl(type, label, value, color, parentControlId, (void (*)(BasicControl*, int, void*))nullptr);
}


Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const char* value, Control::Color color, Control::ControlId_t parentControlId)
{
    return addControl(type, label, value, color, parentControlId, (void (*)(BasicControl*, int, void*))nullptr);
}


Control::ControlId_t ESPUIClass::addControl(Control::Type type,
    const char* label, const String& value, Control::Color color,
    Control::ControlId_t parentControlId, void (*callback)(BasicControl*, int, void*))
{
    return addControl(type, label, value, color, parentControlId, callback, nullptr);
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type,
    const char* label, long value, Control::Color color,
    Control::ControlId_t parentControlId, void (*callback)(BasicControl*, int, void*))
{
return addControl(type, label, value, color, parentControlId, callback, nullptr);
    
}


Control::ControlId_t ESPUIClass::addControl(Control::Type type,
    const char* label, const char* value, Control::Color color,
    Control::ControlId_t parentControlId, void (*callback)(BasicControl*, int, void*))
{
    return addControl(type, label, value, color, parentControlId, callback, nullptr);
}



Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const String& value, Control::Color color, Control::ControlId_t parentControl, void (*callback)(BasicControl*, int, void*), void* userData)
    {
      Control::ControlId_t id = ESPUIcontrolMgr.addControl(type, label, value, color, parentControl, true, callback, userData);
    NotifyClients(ClientUpdateType_t::RebuildNeeded);
    return id;
    }

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, long value, Control::Color color, Control::ControlId_t parentControl, void (*callback)(BasicControl*, int, void*), void* userData)
    {
    Control::ControlId_t id = ESPUIcontrolMgr.addControl(type, label, value, color, parentControl, true, callback, userData);
    if (type == Control::Type::Option) {
      getControl(id)->SetControlChangedId(GetNextControlChangeId());
      NotifyClients(ClientUpdateType_t::UpdateNeeded);
    }
    else
      NotifyClients(ClientUpdateType_t::RebuildNeeded);
    return id;

    }

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const char* value, Control::Color color, Control::ControlId_t parentControl, void (*callback)(BasicControl*, int, void*), void* userData)
    {
    Control::ControlId_t id = ESPUIcontrolMgr.addControl(type, label, value, color, parentControl, true, callback, userData);
    NotifyClients(ClientUpdateType_t::RebuildNeeded);
    return id;

    }


bool ESPUIClass::removeControl(Control::ControlId_t id, bool force_rebuild_ui)
{
    bool Response = ESPUIcontrolMgr.removeControl(id);
    if (force_rebuild_ui)
    {
        ESPUI.jsonReload();
    }
    else
    {
        ESPUI.NotifyClients(ClientUpdateType_t::RebuildNeeded);
    }
    return Response;
} // removeControl


uint16_t ESPUIClass::removeSelectOptions(Control::ControlId_t select_id,  Control::ControlId_t skip_id, bool force_rebuild_ui)
{
    uint16_t Response = ESPUIcontrolMgr.removeSelectOptions(select_id, skip_id);
    if (force_rebuild_ui)
    {
        ESPUIcontrolMgr.RemoveToBeDeletedControls();
	//ESPUI.jsonReload();
    }
    else
    {
        ESPUI.NotifyClients(ClientUpdateType_t::RebuildNeeded);

    }
    return Response;
} // removeSelectOptions


/*
Control::ControlId_t ESPUIClass::label(const char* label, Control::Color color, const String& value)
{
    return addControl(Control::Type::Label, label, value, color);
}

Control::ControlId_t ESPUIClass::graph(const char* label, Control::Color color)
{
    return addControl(Control::Type::Graph, label, "", color);
}

Control::ControlId_t ESPUIClass::slider(
    const char* label, void (*callback)(BasicControl*, int), Control::Color color, int value, int min, int max)
{
    Control::ControlId_t sliderId
        = addControl(Control::Type::Slider, label, value, color, Control::noParent, callback);
    addControl(Control::Type::Min, label, min, Control::Color::None, sliderId);
    addControl(Control::Type::Max, label, max, Control::Color::None, sliderId);
    return sliderId;
}

Control::ControlId_t ESPUIClass::button(const char* label, void (*callback)(BasicControl*, int, void*), Control::Color color, const String& value)
{
    return addControl(Control::Type::Button, label, value, color, Control::noParent, callback);
}

Control::ControlId_t ESPUIClass::switcher(const char* label, void (*callback)(BasicControl*, int, void*), Control::Color color, bool startState)
{
    return addControl(Control::Type::Switcher, label, startState ? "1" : "0", color, Control::noParent, callback);
}

Control::ControlId_t ESPUIClass::pad(const char* label, void (*callback)(BasicControl*, int), Control::Color color)
{
    return addControl(Control::Type::Pad, label, "", color, Control::noParent, callback);
}

Control::ControlId_t ESPUIClass::padWithCenter(const char* label, void (*callback)(BasicControl*, int), Control::Color color)
{
    return addControl(Control::Type::PadWithCenter, label, "", color, Control::noParent, callback);
}

Control::ControlId_t ESPUIClass::number(
    const char* label, void (*callback)(BasicControl*, int), Control::Color color, int number, int min, int max)
{
    Control::ControlId_t numberId = addControl(Control::Type::Number, label, number, color, Control::noParent, callback);
    addControl(Control::Type::Min, label, min, Control::Color::None, numberId);
    addControl(Control::Type::Max, label, max, Control::Color::None, numberId);
    return numberId;
}

Control::ControlId_t ESPUIClass::gauge(const char* label, Control::Color color, int number, int min, int max)
{
    Control::ControlId_t numberId = addControl(Control::Type::Gauge, label, number, color, Control::noParent);
    addControl(Control::Type::Min, label, min, Control::Color::None, numberId);
    addControl(Control::Type::Max, label, max, Control::Color::None, numberId);
    return numberId;
}

Control::ControlId_t ESPUIClass::separator(const char* label)
{
    return addControl(Control::Type::Separator, label, (const char*)emptyCstring, Control::Color::Alizarin, Control::noParent);
}

Control::ControlId_t ESPUIClass::fileDisplay(const char* label, Control::Color color, String filename)
{
    return addControl(Control::Type::FileDisplay, label, filename, color, Control::noParent);
}

Control::ControlId_t ESPUIClass::accelerometer(const char* label, void (*callback)(BasicControl*, int), Control::Color color)
{
    return addControl(Control::Type::Accel, label, "", color, Control::noParent, callback);
}

Control::ControlId_t ESPUIClass::text(const char* label, void (*callback)(BasicControl*, int), Control::Color color, const String& value)
{
    return addControl(Control::Type::Text, label, value, color, Control::noParent, callback);
}
*/
BasicControl* ESPUIClass::getControl(Control::ControlId_t id) {return ESPUIcontrolMgr.getControl(id);}
BasicControl* ESPUIClass::getControlNoLock(Control::ControlId_t id) {return ESPUIcontrolMgr.getControlNoLock(id);}
BasicControl* ESPUIClass::getFirstOptionId(Control::ControlId_t selector, long value)
{return ESPUIcontrolMgr.getFirstOptionId(selector, value);}


void ESPUIClass::updateControl(BasicControl* control, int)
{
    if (!control)
    {
        return;
    }
    // tell the control it has been updated
    control->SetControlChangedId(ESPUI.GetNextControlChangeId());
    NotifyClients(ClientUpdateType_t::UpdateNeeded);
}

uint32_t ESPUIClass::GetNextControlChangeId()
{
    if(uint32_t(-1) == ControlChangeID)
    {
        // force a reload which resets the counters
        jsonReload();
    }
    return ++ControlChangeID;
}

/*void ESPUIClass::setPanelStyle(Control::ControlId_t id, const String& style, int clientId)
{
    Control* control = getControl(id);
    if (control)
    {
        control->panelStyle = style;
        updateControl(control, clientId);
    }
}*/
void ESPUIClass::setPanelStyle(Control::ControlId_t id, const char *style, int clientId)
{
    BasicControl* basic_control = getControl(id);
    if ((basic_control) && (basic_control->getClassId() != BASIC_CONTROL_ID))
    {
        Control *control = static_cast<Control *>(basic_control);
        control->panelStyle = style;
        updateControl(control, clientId);
    }
}

void ESPUIClass::setElementStyle(Control::ControlId_t id, const char *style, int clientId)
{
    BasicControl* basic_control = getControl(id);
    if ((basic_control) && (basic_control->getClassId() != BASIC_CONTROL_ID))
    {
        Control *control = static_cast<Control *>(basic_control);
        control->elementStyle = style;
        updateControl(control, clientId);
    }
}

void ESPUIClass::setInputType(Control::ControlId_t id, const char *type, int clientId)
{
    BasicControl* basic_control = getControl(id);
    if (basic_control)
    {
        basic_control->inputType = type;
        updateControl(basic_control, clientId);
    }
}

void ESPUIClass::setPanelWide(Control::ControlId_t id, bool wide)
{
    BasicControl* control = getControl(id);
    if (control)
    {
        if (wide)
          control->control_flags |= CONTROL_FLAG_WIDE;
        else
          control->control_flags &= ~CONTROL_FLAG_WIDE;

    }
}

void ESPUIClass::setEnabled(Control::ControlId_t id, bool enabled, int clientId)
{
    BasicControl* control = getControl(id);
    if (control)
    {
        // Serial.println(String("CreateAllowed: id: ") + String(clientId) + " State: " + String(enabled));
        if (enabled)
          control->control_flags |= CONTROL_FLAG_ENABLED;
        else
          control->control_flags &= ~CONTROL_FLAG_ENABLED;
        updateControl(control, clientId);
    }
}

void ESPUIClass::setVertical(Control::ControlId_t id, bool vert)
{
    BasicControl* control = getControl(id);
    if (control)
    {
        if (vert)
          control->control_flags |= CONTROL_FLAG_VERTICAL;
        else
          control->control_flags &= ~CONTROL_FLAG_VERTICAL;
    }
}

void ESPUIClass::updateControl(BasicControl::ControlId_t id, int clientId)
{
    BasicControl* control = getControl(id);

    if (!control)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.printf_P(PSTR("Error: Update Control: There is no control with ID %d\n"), id);
        }
#endif
        return;
    }

    updateControl(control, clientId);
}

void ESPUIClass::updateControlValue(BasicControl* basic_control, const String& value, int clientId)
{
    if (!basic_control)
    {
        return;
    }
     
    if (basic_control->control_flags & CONTROL_FLAG_NUMERIC)
     basic_control->numeric_value = value.toInt();
    else {

      if (basic_control->getClassId() != BASIC_CONTROL_ID) {

        //Control *control = static_cast<Control *>(basic_control);
        basic_control->control_flags &= ~CONTROL_FLAG_PCHAR;
        (*basic_control->string_value) = value;
       }
    }
    updateControl(basic_control, clientId);
}

void ESPUIClass::updateControlValue(Control::ControlId_t id, const String& value, int clientId)
{
    BasicControl* basic_control = getControl(id);

    if (!basic_control)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.printf_P(PSTR("Error: updateControlValue Control: There is no control with ID %d\n"), id);
        }
#endif
        return;
    }
    updateControlValue(basic_control, value, clientId);
}

void ESPUIClass::updateControlValue(BasicControl* basic_control, long value, int clientId)
{
    if (!basic_control)
    {
        return;
    }
    
    if (basic_control->control_flags & CONTROL_FLAG_NUMERIC) {

     basic_control->numeric_value = value;
     updateControl(basic_control, clientId);
    }
    else {
     
       (*(basic_control->string_value)) = value;
       updateControl(basic_control, clientId);
    }
}

void ESPUIClass::updateControlValue(Control::ControlId_t id, const long value, int clientId)
{
    BasicControl* basic_control = getControl(id);

    if (!basic_control)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.printf_P(PSTR("Error: updateControlValue Control: There is no control with ID %d\n"), id);
        }
#endif
        return;
    }

    updateControlValue(basic_control, value, clientId);
}

void ESPUIClass::updateControlValue(BasicControl* control, const char* value, int clientId)
{
    if (!control)
    {
        return;
    }
     
    if (control->control_flags & CONTROL_FLAG_NUMERIC)
     control->numeric_value = strtol(value, nullptr, 0);
    else {
      control->control_flags |= CONTROL_FLAG_PCHAR;
      control->cstr_value = value;
    }
    updateControl(control, clientId);
}

void ESPUIClass::updateControlValue(Control::ControlId_t id, const char* value, int clientId)
{
    BasicControl* control = getControl(id);

    if (!control)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.printf_P(PSTR("Error: updateControlValue Control: There is no control with ID %d\n"), id);
        }
#endif
        return;
    }

    updateControlValue(control, value, clientId);
}


void ESPUIClass::updateControlLabel(Control::ControlId_t id, const char* value, int clientId)
{
    updateControlLabel(getControl(id), value, clientId);
}

void ESPUIClass::updateControlLabel(BasicControl* control, const char* value, int clientId)
{
    if (!control)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.printf_P(PSTR("Error: updateControlLabel Control: There is no control with the requested ID \n"));
        }
#endif
        return;
    }
    control->label = value;
    updateControl(control, clientId);
}


void ESPUIClass::updateOption(
      Control::ControlId_t control, const char * label, long value, long dynamic_option_id, 
      uint8_t operation) {


  BasicControl* option_control = getControl(control);

  if (option_control) {
  
    option_control->label = label;
    option_control->control_flags |= CONTROL_FLAG_NUMERIC;
    option_control->numeric_value = value;
    option_control->dynamic_option_id = dynamic_option_id;
    
    switch (operation) {

      case 1:
        option_control->control_flags |= CONTROL_FLAG_OPTION_UPDATE;
      break;

      case 2:
        option_control->control_flags |= CONTROL_FLAG_OPTION_ADD;
      break;

      case 3:
        option_control->control_flags |= CONTROL_FLAG_OPTION_REMOVE;
      break;
    }
    updateControl(option_control, -1);

  }
}



void ESPUIClass::updateVisibility(Control::ControlId_t id, bool visibility, int clientId)
{
    BasicControl* control = getControl(id);
    if (control)
    {
        if (visibility)
          control->control_flags |= CONTROL_FLAG_VISIBLE;
        else
          control->control_flags &= ~CONTROL_FLAG_VISIBLE;
        updateControl(control, clientId);
    }
}

void ESPUIClass::print(Control::ControlId_t id, const String& value)
{
    updateControlValue(id, value);
}

void ESPUIClass::updateLabel(Control::ControlId_t id, const String& value)
{
    updateControlValue(id, value);
}

void ESPUIClass::updateLabel(Control::ControlId_t id, const char* value)
{
    updateControlValue(id, value);
}

void ESPUIClass::updateButton(Control::ControlId_t id, const String& value)
{
    updateControlValue(id, value);
}

void ESPUIClass::updateSlider(Control::ControlId_t id, int nValue, int clientId)
{
    updateControlValue(id, nValue, clientId);
}

void ESPUIClass::updateSwitcher(Control::ControlId_t id, bool nValue, int clientId)
{
    updateControlValue(id, nValue ? 1 : 0, clientId);
}

void ESPUIClass::updateNumber(Control::ControlId_t id, int number, int clientId)
{
    updateControlValue(id, number, clientId);
}

void ESPUIClass::updateText(Control::ControlId_t id, const String& text, int clientId)
{
    updateControlValue(id, text, clientId);
}

/*void ESPUIClass::updateSelect(Control::ControlId_t id, const String& text, int clientId)
{
    updateControlValue(id, text, clientId);
}*/

void ESPUIClass::updateSelect(Control::ControlId_t id, int value, int clientId)
{
    updateControlValue(id, value, clientId);
}


void ESPUIClass::updateGauge(Control::ControlId_t id, int number, int clientId)
{
    updateControlValue(id, number, clientId);
}

void ESPUIClass::updateTime(Control::ControlId_t id, int clientId)
{
    updateControl(id, clientId);
}

void ESPUIClass::clearGraph(Control::ControlId_t id, int clientId)
{
    do // once
    {
        BasicControl* control = getControl(id);
        if (!control)
        {
            break;
        }

        AllocateJsonDocument(document, jsonUpdateDocumentSize);
        JsonObject root = document.to<JsonObject>();

        root[F("type")] = (int)Control::Type::Graph + Control::Type::UpdateOffset;
        root[F("value")] = 0;
        root[F("id")] = control->GetId();

        SendJsonDocToWebSocket(document, clientId);

    } while (false);
}

void ESPUIClass::addGraphPoint(Control::ControlId_t id, int nValue, int clientId)
{
    do // once
    {
        BasicControl* control = getControl(id);
        if (!control)
        {
            break;
        }

        AllocateJsonDocument(document, jsonUpdateDocumentSize);
        JsonObject root = document.to<JsonObject>();

        root[F("type")] = (int)Control::Type::GraphPoint;
        root[F("value")] = nValue;
        root[F("id")] = control->GetId();

        SendJsonDocToWebSocket(document, clientId);

    } while (false);
}

bool ESPUIClass::SendJsonDocToWebSocket(ArduinoJson::JsonDocument& document, uint16_t clientId)
{
    bool Response = false;

    if (0 > clientId)
    {
        if (MapOfClients.end() != MapOfClients.find(clientId))
        {
            Response = MapOfClients[clientId]->SendJsonDocToWebSocket(document);
        }
    }
    else
    {
        for (auto CurrentClient : MapOfClients)
        {
            Response |= CurrentClient.second->SendJsonDocToWebSocket(document);
        }
    }

    return Response;
}

void ESPUIClass::jsonDom(uint16_t, AsyncWebSocketClient*, bool)
{
    NotifyClients(ClientUpdateType_t::RebuildNeeded);
}

// Tell all of the clients that they need to ask for an upload of the control data.
void ESPUIClass::NotifyClients(ClientUpdateType_t newState)
{
    for (auto& CurrentClient : MapOfClients)
    {
        CurrentClient.second->NotifyClient(newState);
    }
}

void ESPUIClass::jsonReload()
{
    for (auto& CurrentClient : MapOfClients)
    {
        // Serial.println("Requesting Reload");
        CurrentClient.second->NotifyClient(ClientUpdateType_t::ReloadNeeded);
    }
}

void ESPUIClass::beginSPIFFS(const char* _title, const char* username, const char* password, uint16_t port)
{
    // Backwards compatibility wrapper
    beginLITTLEFS(_title, username, password, port);
}

void ESPUIClass::beginLITTLEFS(const char* _title, const char* username, const char* password, uint16_t port)
{
    ui_title = _title;
    basicAuthUsername = username;
    basicAuthPassword = password;

    if (username == nullptr && password == nullptr)
    {
        basicAuth = false;
    }
    else
    {
        basicAuth = true;
    }

    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket("/ws");

    bool fsBegin = EspuiLittleFS.begin();
    if (!fsBegin)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("LITTLEFS Mount Failed, PLEASE CHECK THE README ON HOW TO "
                             "PREPARE YOUR ESP!!!!!!!"));
        }
#endif

        return;
    }

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        listDir("/", 1);
    }
#endif

    bool indexExists = EspuiLittleFS.exists("/index.htm");
    if (!indexExists)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("Please read the README!!!!!!!, Make sure to "
                             "prepareFileSystem() once in an empty sketch"));
        }
#endif

        return;
    }

    ws->onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data,
                    size_t len) { ESPUI.onWsEvent(server, client, type, arg, data, len); });
    server->addHandler(ws);

    if (basicAuth)
    {
        if (WS_AUTHENTICATION)
        {
            ws->setAuthentication(basicAuthUsername, basicAuthPassword);
        }
        server->serveStatic("/", EspuiLittleFS, "/").setDefaultFile("index.htm").setAuthentication(username, password);
    }
    else
    {
        server->serveStatic("/", EspuiLittleFS, "/").setDefaultFile("index.htm");
    }

    // Heap for general Servertest
    server->on("/heap", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        request->send(200, "text/plain", heapInfo(F("In LITTLEFS mode")));
    });

    server->onNotFound([this](AsyncWebServerRequest* request) {
        if (captivePortal)
        {
            request->redirect("/");
        }
        else
        {
            request->send(404);
        }
    });


   server->on("/js/custom.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        request->send(200, "application/javascript", customJS ? customJS : "");
	//log_i("%s", customJS);
    });

    server->on("/css/custom.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        request->send(200, "text/css", customCSS ? customCSS : "");
    });


    server->begin();

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("UI Initialized"));
    }
#endif
}

void ESPUIClass::begin(const char* _title, const char* username, const char* password, uint16_t port)
{
    basicAuthUsername = username;
    basicAuthPassword = password;

    if (username != nullptr && password != nullptr)
    {
        basicAuth = true;
    }
    else
    {
        basicAuth = false;
    }

    ui_title = _title;

    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket("/ws");

    ws->onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data,
                    size_t len) { ESPUI.onWsEvent(server, client, type, arg, data, len); });

    server->addHandler(ws);

    if (basicAuth && WS_AUTHENTICATION)
        ws->setAuthentication(username, password);

    server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", HTML_INDEX);
        request->send(response);
    });

    // Javascript files

    server->on("/js/zepto.min.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        AsyncWebServerResponse* response
            = request->beginResponse(200, "application/javascript", JS_ZEPTO_GZIP, sizeof(JS_ZEPTO_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/js/controls.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        AsyncWebServerResponse* response
            = request->beginResponse(200, "application/javascript", JS_CONTROLS_GZIP, sizeof(JS_CONTROLS_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/js/slider.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        AsyncWebServerResponse* response
            = request->beginResponse(200, "application/javascript", JS_SLIDER_GZIP, sizeof(JS_SLIDER_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/js/graph.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        AsyncWebServerResponse* response
            = request->beginResponse(200, "application/javascript", JS_GRAPH_GZIP, sizeof(JS_GRAPH_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/js/tabbedcontent.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        AsyncWebServerResponse* response = request->beginResponse(
            200, "application/javascript", JS_TABBEDCONTENT_GZIP, sizeof(JS_TABBEDCONTENT_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // Stylesheets

    server->on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        AsyncWebServerResponse* response
            = request->beginResponse(200, "text/css", CSS_STYLE_GZIP, sizeof(CSS_STYLE_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/css/normalize.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        AsyncWebServerResponse* response
            = request->beginResponse(200, "text/css", CSS_NORMALIZE_GZIP, sizeof(CSS_NORMALIZE_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // Heap for general Servertest
    server->on("/heap", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        request->send(200, "text/plain", heapInfo(F("In Memorymode")));
    });

    server->on("/js/custom.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        request->send(200, "application/javascript", customJS ? customJS : "");
	//if (customJS)
	 //log_i("%s", customJS);
	//else log_i("empty customJS");
    });

    server->on("/css/custom.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }

        request->send(200, "text/css", customCSS ? customCSS : "");
    });

    server->onNotFound([this](AsyncWebServerRequest* request) {
        if (captivePortal)
        {
            AsyncResponseStream *response = request->beginResponseStream("text/html");
            String responseText;
            responseText.reserve(1024);
            responseText += F("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
            responseText += ("<p>If site does not re-direct click here <a href='http://" +  WiFi.softAPIP().toString() + "'>this link</a></p>");
            responseText += ("</body></html><head><meta http-equiv=\"Refresh\" content=\"0; URL='http://" +  WiFi.softAPIP().toString() + "'\" /></head>");
            response->write(responseText.c_str(), responseText.length());
            request->send(response);
            request->redirect("/");
        }
        else
        {
            request->send(404);
        }
        yield();
    });

    server->begin();

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("UI Initialized"));
    }
#endif
}

void ESPUIClass::setVerbosity(Verbosity v)
{
    verbosity = v;
}

ESPUIClass ESPUI;
