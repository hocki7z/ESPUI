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
#include "dataPickrJS.h"
#include "dataPickrCSS.h"

#if ESP8266
#include <umm_malloc/umm_heap_select.h>
#endif
#include "ESPUIcontrolMgr.h"

static const char* emptyCstring = "";
static const char* customJS = nullptr;
static const char* customCSS = nullptr;

void ESPUIClass::setCustomJS(const char* js)
{
    customJS = js;
}

void ESPUIClass::setCustomCSS(const char* css)
{
    customCSS = css;
}

static void sendHeapInfo(AsyncWebServerRequest* request, PGM_P mode)
{
    char buf[160];
#if ESP8266
    uint32_t hfree;
    uint16_t hmax;
    uint8_t hfrag;
    {
        HeapSelectDram useRegularRamHere;
        ESP.getHeapStats(&hfree, &hmax, &hfrag);
    }
    snprintf_P(buf, sizeof(buf), PSTR("DRAM: free: %u max: %u frag: %u%%\n%S"), hfree, hmax, hfrag, mode);
#else
    snprintf_P(buf, sizeof(buf), PSTR("%u %S"), (unsigned)ESP.getFreeHeap(), mode);
#endif
    request->send(200, "text/plain", buf);
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
#endif
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
#endif
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

void ESPUIClass::prepareFileSystem(bool format)
{
#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("About to prepare filesystem..."));
    }
#endif

#if defined(ESP32)
    if (!EspuiLittleFS.begin(false))
    {
        if (!EspuiLittleFS.begin(true))
        {
#else
    if (!EspuiLittleFS.begin())
    {
        if (EspuiLittleFS.format())
        {
#endif
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
#if defined(ESP32)
    if (verbosity)
    {
        listDir("/", 1);
    }
#endif
#endif

    EspuiLittleFS.end();
}

void ESPUIClass::onWsEvent(
    AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    ESPUIcontrolMgr.RemoveToBeDeletedControls();

    if (WS_EVT_DISCONNECT == type)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("WS_EVT_DISCONNECT"));
        }
#endif
        auto it = MapOfClients.find(client->id());
        if (MapOfClients.end() != it)
        {
            delete it->second;
            MapOfClients.erase(it);
        }
    }
    else
    {
        if (type == WS_EVT_CONNECT)
        {
            ws->cleanupClients();
            if (on_ws_connect)
            {
                on_ws_connect();
            }
        }

        ESPUIclient* uiClient = nullptr;
        auto it = MapOfClients.find(client->id());
        if (MapOfClients.end() == it)
        {
            uiClient = new ESPUIclient(client);
            MapOfClients[client->id()] = uiClient;
        }
        else
        {
            uiClient = it->second;
        }

        if (uiClient->onWsEvent(type, arg, data, len))
        {
            NotifyClients(ESPUIclient::UpdateNeeded);
        }
    }
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label)
{
    switch (type)
    {
        case Control::Type::Option:
        case Control::Type::Number:
        case Control::Type::Select:
        case Control::Type::Switcher:
        case Control::Type::Slider:
        case Control::Type::Min:
        case Control::Type::Max:
        case Control::Type::Step:
        case Control::Type::Gauge:
            return addControl(type, label, (long)0);

        default:
            return addControl(type, label, emptyCstring);
    }
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

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const String& value, Control::Color color, Control::ControlId_t parentControlId)
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

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const String& value, Control::Color color, Control::ControlId_t parentControlId, void (*callback)(BasicControl*, int, void*))
{
    return addControl(type, label, value, color, parentControlId, callback, nullptr);
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, long value, Control::Color color, Control::ControlId_t parentControlId, void (*callback)(BasicControl*, int, void*))
{
    return addControl(type, label, value, color, parentControlId, callback, nullptr);
}

Control::ControlId_t ESPUIClass::addControl(Control::Type type, const char* label, const char* value, Control::Color color, Control::ControlId_t parentControlId, void (*callback)(BasicControl*, int, void*))
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
    if (type == Control::Type::Option)
    {
        BasicControl* ctrl = getControl(id);
        if (ctrl)
        {
            ctrl->SetControlChangedId(GetNextControlChangeId());
        }
        NotifyClients(ClientUpdateType_t::UpdateNeeded);
    }
    else
    {
        NotifyClients(ClientUpdateType_t::RebuildNeeded);
    }
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
}

bool ESPUIClass::removeOptionControl(Control::ControlId_t id)
{
    bool Response = ESPUIcontrolMgr.removeControl(id);
    ESPUI.NotifyClients(ClientUpdateType_t::UpdateNeeded);
    return Response;
}

uint16_t ESPUIClass::removeSelectOptions(Control::ControlId_t select_id, Control::ControlId_t skip_id, bool force_rebuild_ui)
{
    uint16_t Response = ESPUIcontrolMgr.removeSelectOptions(select_id, skip_id);
    if (force_rebuild_ui)
    {
        ESPUIcontrolMgr.RemoveToBeDeletedControls();
    }
    else
    {
        ESPUI.NotifyClients(ClientUpdateType_t::UpdateNeeded);
    }
    return Response;
}

BasicControl* ESPUIClass::getControl(Control::ControlId_t id) { return ESPUIcontrolMgr.getControl(id); }
BasicControl* ESPUIClass::getControlNoLock(Control::ControlId_t id) { return ESPUIcontrolMgr.getControlNoLock(id); }
BasicControl* ESPUIClass::getFirstOptionId(Control::ControlId_t selector, long value) { return ESPUIcontrolMgr.getFirstOptionId(selector, value); }
BasicControl* ESPUIClass::getNextOptionId(Control::ControlId_t selector, long value, Control::ControlId_t prev_option_id) { return ESPUIcontrolMgr.getNextOptionId(selector, value, prev_option_id); }

void ESPUIClass::updateControl(BasicControl* control, int)
{
    if (!control)
    {
        return;
    }
    control->SetControlChangedId(ESPUI.GetNextControlChangeId());
    NotifyClients(ClientUpdateType_t::UpdateNeeded);
}

uint32_t ESPUIClass::GetNextControlChangeId()
{
    if (uint32_t(-1) == ControlChangeID)
    {
        jsonReload();
    }
    return ++ControlChangeID;
}

void ESPUIClass::setPanelStyle(Control::ControlId_t id, const char* style, int clientId)
{
    BasicControl* basic_control = getControl(id);
    if ((basic_control) && (basic_control->getClassId() != BASIC_CONTROL_ID))
    {
#ifdef ESP32
        xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
        LabelControl* control = static_cast<LabelControl*>(basic_control);
        control->panelStyle = style;
#ifdef ESP32
        xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
        updateControl(control, clientId);
    }
}

void ESPUIClass::setElementStyle(Control::ControlId_t id, const char* style, int clientId)
{
    BasicControl* basic_control = getControl(id);
    if ((basic_control) && (basic_control->getClassId() != BASIC_CONTROL_ID))
    {
#ifdef ESP32
        xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
        LabelControl* control = static_cast<LabelControl*>(basic_control);
        control->elementStyle = style;
#ifdef ESP32
        xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
        updateControl(control, clientId);
    }
}

void ESPUIClass::setInputType(Control::ControlId_t id, const char* type, int clientId)
{
    BasicControl* basic_control = getControl(id);
    if (basic_control)
    {
#ifdef ESP32
        xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
        basic_control->inputType = type;
#ifdef ESP32
        xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
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

#ifdef ESP32
    xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
    if (basic_control->control_flags & CONTROL_FLAG_NUMERIC)
    {
        basic_control->numeric_value = value.toInt();
    }
    else if (basic_control->getClassId() != BASIC_CONTROL_ID)
    {
        basic_control->setDynString(value.c_str(), value.length());
    }
#ifdef ESP32
    xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
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

#ifdef ESP32
    xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
    if (basic_control->control_flags & CONTROL_FLAG_NUMERIC)
    {
        basic_control->numeric_value = value;
    }
    else if (basic_control->getClassId() != BASIC_CONTROL_ID)
    {
        char num_buf[16];
        ltoa(value, num_buf, 10);
        basic_control->setDynString(num_buf);
    }
#ifdef ESP32
    xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
    updateControl(basic_control, clientId);
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

#ifdef ESP32
    xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
    if (control->control_flags & CONTROL_FLAG_NUMERIC)
    {
        control->numeric_value = value ? strtol(value, nullptr, 0) : 0;
    }
    else
    {
        control->freeDynString();
        control->control_flags |= CONTROL_FLAG_PCHAR;
        control->cstr_value = value;
    }
#ifdef ESP32
    xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
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
            Serial.printf_P(PSTR("Error: updateControlLabel Control: There is no control with the requested ID\n"));
        }
#endif
        return;
    }
#ifdef ESP32
    xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
    control->label = value;
#ifdef ESP32
    xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
    updateControl(control, clientId);
}

void ESPUIClass::updateOption(
    Control::ControlId_t control, const char* label, long value, long dynamic_option_id, uint8_t operation)
{
    BasicControl* option_control = getControl(control);

    if (option_control)
    {
#ifdef ESP32
        xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
        option_control->label = label;
        option_control->freeDynString();
        option_control->control_flags &= ~(CONTROL_FLAG_PCHAR | CONTROL_FLAG_OPTION_UPDATE | CONTROL_FLAG_OPTION_ADD | CONTROL_FLAG_OPTION_REMOVE);
        option_control->control_flags |= CONTROL_FLAG_NUMERIC;
        option_control->numeric_value = value;
        option_control->dynamic_option_id = dynamic_option_id;

        switch (operation)
        {
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
#ifdef ESP32
        xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif
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
    updateControlValue(id, (long)nValue, clientId);
}

void ESPUIClass::updateSwitcher(Control::ControlId_t id, bool nValue, int clientId)
{
    updateControlValue(id, (long)(nValue ? 1 : 0), clientId);
}

void ESPUIClass::updateNumber(Control::ControlId_t id, int number, int clientId)
{
    updateControlValue(id, (long)number, clientId);
}

void ESPUIClass::updateText(Control::ControlId_t id, const String& text, int clientId)
{
    updateControlValue(id, text, clientId);
}

void ESPUIClass::updateSelect(Control::ControlId_t id, int value, int clientId)
{
    updateControlValue(id, (long)value, clientId);
}

void ESPUIClass::updateGauge(Control::ControlId_t id, int number, int clientId)
{
    updateControlValue(id, (long)number, clientId);
}

void ESPUIClass::updateTime(Control::ControlId_t id, int clientId)
{
    updateControl(id, clientId);
}

void ESPUIClass::updateDetails(Control::ControlId_t id, bool expanded, int clientId)
{
    BasicControl* control = getControl(id);

    if ((control != nullptr) &&
        (control->GetType() == Control::Type::Details))
    {
#ifdef ESP32
        xSemaphoreTake(ESPUIcontrolMgr.ControlsSemaphore, portMAX_DELAY);
#endif
        control->detailsExpanded = expanded;
#ifdef ESP32
        xSemaphoreGive(ESPUIcontrolMgr.ControlsSemaphore);
#endif

        updateControl(control, clientId);
    }
}

void ESPUIClass::clearGraph(Control::ControlId_t id, int clientId)
{
    BasicControl* control = getControl(id);
    if (!control || !ws)
    {
        return;
    }

    char buf[48];
    int len = snprintf_P(buf, sizeof(buf), PSTR("{\"type\":%u,\"value\":0,\"id\":%u}"),
                         (unsigned)(Control::Type::Graph + Control::Type::UpdateOffset),
                         (unsigned)control->GetId());
    if (len > 0 && len < (int)sizeof(buf))
    {
        if (clientId >= 0)
            ws->text((uint32_t)clientId, buf, (size_t)len);
        else
            ws->textAll(buf, (size_t)len);
    }
}

void ESPUIClass::addGraphPoint(Control::ControlId_t id, int nValue, int clientId)
{
    BasicControl* control = getControl(id);
    if (!control || !ws)
    {
        return;
    }

    char buf[48];
    int len = snprintf_P(buf, sizeof(buf), PSTR("{\"type\":%u,\"value\":%d,\"id\":%u}"),
                         (unsigned)Control::Type::GraphPoint,
                         nValue,
                         (unsigned)control->GetId());
    if (len > 0 && len < (int)sizeof(buf))
    {
        if (clientId >= 0)
            ws->text((uint32_t)clientId, buf, (size_t)len);
        else
            ws->textAll(buf, (size_t)len);
    }
}

bool ESPUIClass::SendJsonDocToWebSocket(ArduinoJson::JsonDocument& document, int clientId)
{
    bool Response = false;

    if (clientId >= 0)
    {
        auto it = MapOfClients.find((uint32_t)clientId);
        if (MapOfClients.end() != it)
        {
            Response = it->second->SendJsonDocToWebSocket(document);
        }
    }
    else
    {
        for (auto& CurrentClient : MapOfClients)
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

void ESPUIClass::NotifyClients(ClientUpdateType_t newState)
{
    for (auto& CurrentClient : MapOfClients)
    {
        CurrentClient.second->NotifyClient(newState);
    }
}

bool ESPUIClass::isSynchronized()
{
    bool result = true;

    for (auto& CurrentClient : MapOfClients)
    {
        result = result && CurrentClient.second->IsSyncronized();
    }
    return result;
}

void ESPUIClass::jsonReload()
{
    for (auto& CurrentClient : MapOfClients)
    {
        CurrentClient.second->NotifyClient(ClientUpdateType_t::ReloadNeeded);
    }
}

void ESPUIClass::beginSPIFFS(const char* _title, const char* username, const char* password, uint16_t port)
{
    beginLITTLEFS(_title, username, password, port);
}

void ESPUIClass::beginLITTLEFS(const char* _title, const char* username, const char* password, uint16_t port)
{
    ui_title = _title;
    basicAuthUsername = username;
    basicAuthPassword = password;
    basicAuth = (username != nullptr && password != nullptr);

    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket("/ws");

    bool fsBegin = EspuiLittleFS.begin();
    if (!fsBegin)
    {
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("LITTLEFS Mount Failed, PLEASE CHECK THE README ON HOW TO PREPARE YOUR ESP!!!!!!!"));
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
            Serial.println(F("Please read the README!!!!!!!, Make sure to prepareFileSystem() once in an empty sketch"));
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

    server->on("/heap", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        sendHeapInfo(request, PSTR("In LITTLEFS mode"));
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
    basicAuth = (username != nullptr && password != nullptr);
    ui_title = _title;

    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket("/ws");

    ws->onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data,
                   size_t len) { ESPUI.onWsEvent(server, client, type, arg, data, len); });

    server->addHandler(ws);

    if (basicAuth && WS_AUTHENTICATION)
    {
        ws->setAuthentication(username, password);
    }

    server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", HTML_INDEX);
        request->send(response);
    });

    server->on("/js/zepto.min.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response =
            request->beginResponse(200, "application/javascript", JS_ZEPTO_GZIP, sizeof(JS_ZEPTO_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/js/controls.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response =
            request->beginResponse(200, "application/javascript", JS_CONTROLS_GZIP, sizeof(JS_CONTROLS_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/js/slider.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response =
            request->beginResponse(200, "application/javascript", JS_SLIDER_GZIP, sizeof(JS_SLIDER_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/js/graph.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response =
            request->beginResponse(200, "application/javascript", JS_GRAPH_GZIP, sizeof(JS_GRAPH_GZIP));
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

    server->on("/js/pickr.min.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response = request->beginResponse(
            200, "application/javascript", JS_PICKR_GZIP, sizeof(JS_PICKR_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/css/pickr.min.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response =
            request->beginResponse(200, "text/css", CSS_PICKR_GZIP, sizeof(CSS_PICKR_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response =
            request->beginResponse(200, "text/css", CSS_STYLE_GZIP, sizeof(CSS_STYLE_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/css/normalize.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        AsyncWebServerResponse* response =
            request->beginResponse(200, "text/css", CSS_NORMALIZE_GZIP, sizeof(CSS_NORMALIZE_GZIP));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server->on("/heap", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
        {
            return request->requestAuthentication();
        }
        sendHeapInfo(request, PSTR("In Memorymode"));
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
            IPAddress ip = WiFi.softAPIP();
            char buf[280];
            snprintf_P(buf, sizeof(buf),
                PSTR("<!DOCTYPE html><html><head><title>Captive Portal</title>"
                     "<meta http-equiv=\"Refresh\" content=\"0; URL='http://%u.%u.%u.%u'\" />"
                     "</head><body><p>If site does not re-direct click here <a href='http://%u.%u.%u.%u'>this link</a></p></body></html>"),
                ip[0], ip[1], ip[2], ip[3],
                ip[0], ip[1], ip[2], ip[3]);
            request->send(200, "text/html", buf);
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