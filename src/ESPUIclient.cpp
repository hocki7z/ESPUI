#include "ESPUI.h"
#include "ESPUIclient.h"
#include "ESPUIcontrol.h"

static size_t max_buffer_len = 0;
// JSONSlave:
// helper to process exact JSON serialization size
// it takes ~2ms on esp8266 and avoid large String reallocation which is really worth the cost
class JSONSlave: public Print
{
public:
    size_t write (uint8_t c) override { counter++; return 1; }
    size_t write (const uint8_t* buf, size_t count) override { counter += count; return count; }
    size_t get_counter () { return counter; }

    static size_t serializedSize (JsonDocument& doc)
    {
        JSONSlave counter;
        serializeJson(doc, counter);
        return counter.get_counter();
    }

    static size_t serialize (JsonDocument& doc, String& str)
    {
        size_t s = serializedSize(doc) + 10; // 10 is paranoid
        str.reserve(s);
        serializeJson(doc, str);
        return s;
    }

    static String toString (JsonDocument& doc)
    {
        String str;
        serialize(doc, str);
        return str;
    }

protected:
    size_t counter = 0;
};

ESPUIclient::ESPUIclient(AsyncWebSocketClient * _client):
    client(_client)
{
    fsm_EspuiClient_state_Idle_imp.SetParent(this);
    fsm_EspuiClient_state_SendingUpdate_imp.SetParent(this);
    fsm_EspuiClient_state_Rebuilding_imp.SetParent(this);
    fsm_EspuiClient_state_Reloading_imp.SetParent(this);

    fsm_EspuiClient_state_Idle_imp.Init();
}

ESPUIclient::ESPUIclient(const ESPUIclient& source):
    client(source.client)
{
    fsm_EspuiClient_state_Idle_imp.SetParent(this);
    fsm_EspuiClient_state_SendingUpdate_imp.SetParent(this);
    fsm_EspuiClient_state_Rebuilding_imp.SetParent(this);
    fsm_EspuiClient_state_Reloading_imp.SetParent(this);

    fsm_EspuiClient_state_Idle_imp.Init();
}

ESPUIclient::~ESPUIclient()
{
}

bool ESPUIclient::CanSend()
{
    if (nullptr != client && client->status() == WS_CONNECTED && !client->queueIsFull())
    {
        return client->canSend();
    }
    return false;
}

void ESPUIclient::FillInHeader(JsonDocument& document)
{
    document[F("type")] = UI_EXTEND_GUI;
    document[F("sliderContinuous")] = ESPUI.sliderContinuous;
    document[F("startindex")] = 0;
    document[F("totalcontrols")] = ESPUIcontrolMgr.GetControlCount();
    JsonArray items = AllocateJsonArray(document, F("controls"));
    JsonObject titleItem = AllocateJsonObject(items);
    titleItem[F("type")] = (int)UI_TITLE;
    titleItem[F("label")] = ESPUI.ui_title;
}

bool ESPUIclient::IsSyncronized()
{
    return ((ClientUpdateType_t::Synchronized == ClientUpdateType) &&
            (&fsm_EspuiClient_state_Idle_imp == pCurrentFsmState));
}

bool ESPUIclient::SendClientNotification(ClientUpdateType_t value)
{
    bool Response = false;

    do // once
    {
        if(!CanSend())
        {
            Serial.println(F("ESPUIclient::SendClientNotification:CannotSend"));
            break;
        }

        #if ARDUINOJSON_VERSION_MAJOR > 6
        JsonDocument document;
#else
        AllocateJsonDocument(document, 256);
#endif
        FillInHeader(document);
        if(ClientUpdateType_t::ReloadNeeded == value)
        {
            // Serial.println(F("ESPUIclient::SendClientNotification:set type to reload"));
            document["type"] = int(UI_RELOAD);
        }
        // dont send any controls

        Response = SendJsonDocToWebSocket(document);
        Serial.println(String("ESPUIclient::SendClientNotification:NotificationSent:Response: ") + String(Response));

    } while (false);
    return Response;
}

void ESPUIclient::NotifyClient(ClientUpdateType_t newState)
{
    SetState(newState);
    pCurrentFsmState->NotifyClient();
}

// Handle Websockets Communication
bool ESPUIclient::onWsEvent(AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    bool Response = false;
    // Serial.println(String("ESPUIclient::OnWsEvent: type: ") + String(type));

    switch (type)
    {
        case WS_EVT_PONG:
        {
            #if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.println(F("ESPUIclient::OnWsEvent:WS_EVT_PONG"));
            }
            #endif
            break;
        }

        case WS_EVT_ERROR:
        {
            #if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.println(F("ESPUIclient::OnWsEvent:WS_EVT_ERROR"));
            }
            #endif
            break;
        }

        case WS_EVT_CONNECT:
        {
    		#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.println(F("ESPUIclient::OnWsEvent:WS_EVT_CONNECT"));
                Serial.println(client->id());
            }
    		#endif

            // Serial.println("ESPUIclient:onWsEvent:WS_EVT_CONNECT: Call NotifyClient: RebuildNeeded");
            NotifyClient(ClientUpdateType_t::RebuildNeeded);
            break;
        }

        case WS_EVT_DATA:
        {
            if (!data || len == 0)
            {
                break;
            }

            char stack_buf[128];
            char* msg_buf = (len < sizeof(stack_buf)) ? stack_buf : (char*)malloc(len + 1);
            if (!msg_buf)
            {
                break;
            }

            memcpy(msg_buf, data, len);
            msg_buf[len] = '\0';

            // Find the first and last ':' in the message.
            // Note: "uiok:0" has ONLY ONE colon (cmd:id), while control events have two (cmd:value:id).
            char* first_colon = strchr(msg_buf, ':');
            if (!first_colon)
            {
                if (msg_buf != stack_buf) free(msg_buf);
                break;
            }

            char* last_colon = strrchr(msg_buf, ':');

            const char* cmd = msg_buf;
            const char* value = "";
            uint16_t id = (uint16_t)strtoul(last_colon + 1, nullptr, 10);

            *first_colon = '\0';
            if (last_colon != first_colon)
            {
                *last_colon = '\0';
                value = first_colon + 1;
            }

            if (strcmp_P(cmd, PSTR("uiok")) == 0)
            {
                pCurrentFsmState->ProcessAck(id, emptyString);
                if (msg_buf != stack_buf) free(msg_buf);
                break;
            }

            if (strcmp_P(cmd, PSTR("uifragmentok")) == 0)
            {
                if (*value != '\0')
                {
                    pCurrentFsmState->ProcessAck(uint16_t(-1), String(value));
                }
                if (msg_buf != stack_buf) free(msg_buf);
                break;
            }

            if (strcmp_P(cmd, PSTR("uiuok")) == 0)
            {
                if (msg_buf != stack_buf) free(msg_buf);
                break;
            }

            if (strcmp_P(cmd, PSTR("uicc")) == 0)
            {
                if (ESPUI.on_ws_custom_cmd)
                {
                    ESPUI.on_ws_custom_cmd(id);
                }
                if (msg_buf != stack_buf) free(msg_buf);
                break;
            }

            BasicControl* control = ESPUI.getControl(id);
            if (nullptr != control)
            {
                control->onWsEvent(cmd, value);
                Response = true;
            }

            if (msg_buf != stack_buf)
            {
                free(msg_buf);
            }
            break;
        }

        default:
        {
            // Serial.println(F("ESPUIclient::OnWsEvent:default"));
            break;
        }
    } // end switch

    return Response;
}

/*
Convert & Transfer Arduino elements to JSON elements. This function sends a chunk of
JSON describing the controls of the UI, starting from the control at index startidx.
If startidx is 0 then a UI_INITIAL_GUI message will be sent, else a UI_EXTEND_GUI.
Both message types contain a list of serialised UI elements. Only a portion of the UI
will be sent in order to avoid websocket buffer overflows. The client will acknowledge
receipt of a partial message by requesting the next chunk of UI.

The protocol is:
SERVER: SendControlsToClient(0):
    "UI_INITIAL_GUI: n serialised UI elements"
CLIENT: controls.js:handleEvent()
    "uiok:n"
SERVER: SendControlsToClient(n):
    "UI_EXTEND_GUI: n serialised UI elements"
CLIENT: controls.js:handleEvent()
    "uiok:2*n"
etc.
    Returns true if all controls have been sent (aka: Done)
*/
bool ESPUIclient::SendControlsToClient(uint16_t startidx, ClientUpdateType_t TransferMode, String FragmentRequest)
{
    bool Response = false;
    // Serial.println(String("ESPUIclient:SendControlsToClient:startidx: ") + String(startidx));
    do // once
    {
        if(!CanSend())
        {
            // Serial.println("ESPUIclient:SendControlsToClient: Cannot Send to clients.");
            break;
        }

        else if ((startidx >= ESPUIcontrolMgr.GetControlCount()) && (emptyString.equals(FragmentRequest)))
        {
            // Serial.println(F("ERROR:ESPUIclient:SendControlsToClient: No more controls to send."));

	    if (ESPUI.on_ws_custom_cmd) {
        	ESPUI.on_ws_custom_cmd(1); 
    	    }
            Response = true;
            break;
        }

        /*AllocateJsonDocument(document, ESPUI.jsonInitialDocumentSize);
        FillInHeader(document);
        document[F("startindex")] = startidx;
        document[F("totalcontrols")] = uint16_t(-1); // ESPUI.controlCount;

        if(0 == startidx)
        {
            document["type"] = (ClientUpdateType_t::RebuildNeeded == TransferMode) ? UI_INITIAL_GUI : UI_EXTEND_GUI;
            CurrentSyncID = NextSyncID;
            NextSyncID = ESPUI.GetNextControlChangeId();
        }*/

        chunkDocument.clear();
        FillInHeader(chunkDocument);
        chunkDocument[F("startindex")] = startidx;
        chunkDocument[F("totalcontrols")] = uint16_t(-1);

        if(0 == startidx)
        {
            chunkDocument["type"] = (ClientUpdateType_t::RebuildNeeded == TransferMode) ? UI_INITIAL_GUI : UI_EXTEND_GUI;
            CurrentSyncID = NextSyncID;
            NextSyncID = ESPUI.GetNextControlChangeId();
        }
        
        if(ESPUIcontrolMgr.prepareJSONChunk(startidx, chunkDocument, ClientUpdateType_t::UpdateNeeded == TransferMode, FragmentRequest, CurrentSyncID))
        {
            #if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity >= Verbosity::VerboseJSON)
                {
                    Serial.println(F("ESPUIclient:SendControlsToClient: Sending elements --------->"));
                    serializeJson(chunkDocument, Serial);
                    Serial.println();
                }
            #endif

            // Serial.println("ESPUIclient:SendControlsToClient: Send message.");
            if(true == SendJsonDocToWebSocket(chunkDocument))
            {
                // Serial.println("ESPUIclient:SendControlsToClient: Sent.");
            }
            else
            {
                // Serial.println("ESPUIclient:SendControlsToClient: Send failed.");
            }
        }
        else
        {
            // Serial.println("ESPUIclient:SendControlsToClient: No elements to send.");
            Response = true;
        }

    } while(false);

    // Serial.println(String("ESPUIclient:SendControlsToClient:Response: ") + String(Response));
    return Response;
}

bool ESPUIclient::SendJsonDocToWebSocket(JsonDocument& document)
{
    bool Response = true;

    do // once
    {
        if (!CanSend())
        {
            Response = false;
            break;
        }

        const size_t len = measureJson(document);
        if (len > max_buffer_len)
            max_buffer_len = len;
        log_i("max_buffer_len %lu", max_buffer_len);

        AsyncWebSocketMessageBuffer* buffer = ESPUI.WebSocket()->makeBuffer(len);
        if (!buffer)
        {
            Response = false;
            break;
        }

        // Pass buffer->get() (uint8_t*) and exact len so ArduinoJson writes raw bytes without a trailing '\0'
        serializeJson(document, buffer->get(), len);
        client->text(buffer);
    } while (false);

    return Response;
}

void ESPUIclient::SetState(ClientUpdateType_t value)
{
    // only a higher priority state request can replace the current state request
    if(uint32_t(ClientUpdateType) < uint32_t(value))
    {
        ClientUpdateType = value;
    }
}

