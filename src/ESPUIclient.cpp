#include "ESPUI.h"
#include "ESPUIclient.h"
#include "ESPUIcontrol.h"

static size_t max_buffer_len = 0;

ESPUIclient::ESPUIclient(AsyncWebSocketClient* _client)
    : client(_client)
{
    fsm_EspuiClient_state_Idle_imp.SetParent(this);
    fsm_EspuiClient_state_SendingUpdate_imp.SetParent(this);
    fsm_EspuiClient_state_Rebuilding_imp.SetParent(this);
    fsm_EspuiClient_state_Reloading_imp.SetParent(this);

    fsm_EspuiClient_state_Idle_imp.Init();
}

ESPUIclient::ESPUIclient(const ESPUIclient& source)
    : client(source.client)
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
        if (!CanSend())
        {
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.println(F("ESPUIclient::SendClientNotification:CannotSend"));
            }
#endif
            break;
        }

        chunkDocument.clear();
        FillInHeader(chunkDocument);
        if (ClientUpdateType_t::ReloadNeeded == value)
        {
            chunkDocument[F("type")] = int(UI_RELOAD);
        }

        Response = SendJsonDocToWebSocket(chunkDocument);
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.printf_P(PSTR("ESPUIclient::SendClientNotification:NotificationSent:Response: %d\n"), Response);
        }
#endif
    } while (false);

    return Response;
}

void ESPUIclient::NotifyClient(ClientUpdateType_t newState)
{
    SetState(newState);
    pCurrentFsmState->NotifyClient();
}

bool ESPUIclient::onWsEvent(AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    bool Response = false;

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
                pCurrentFsmState->ProcessAck(id, -1);
                if (msg_buf != stack_buf) free(msg_buf);
                break;
            }

            if (strcmp_P(cmd, PSTR("uifragmentok")) == 0)
            {
                // Format from controls.js: "0: {\"id\":12,\"offset\":500}"
                const char* id_ptr  = strstr_P(value, PSTR("\"id\":"));
                const char* off_ptr = strstr_P(value, PSTR("\"offset\":"));
                if (id_ptr && off_ptr)
                {
                    uint16_t frag_id  = (uint16_t)strtoul(id_ptr + 5, nullptr, 10);
                    int32_t  frag_off = (int32_t)strtoul(off_ptr + 9, nullptr, 10);
                    pCurrentFsmState->ProcessAck(frag_id, frag_off);
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
            break;
    }

    return Response;
}

bool ESPUIclient::SendControlsToClient(uint16_t startidx, ClientUpdateType_t TransferMode, int32_t FragmentOffset)
{
    bool Response = false;

    do // once
    {
        if (!CanSend())
        {
            break;
        }
        else if ((startidx >= ESPUIcontrolMgr.GetControlCount()) && (FragmentOffset < 0))
        {
            if (ESPUI.on_ws_custom_cmd)
            {
                ESPUI.on_ws_custom_cmd(1);
            }
            Response = true;
            break;
        }

        chunkDocument.clear();
        FillInHeader(chunkDocument);
        chunkDocument[F("startindex")] = (FragmentOffset >= 0) ? 0 : startidx;
        chunkDocument[F("totalcontrols")] = uint16_t(-1);

        if (0 == startidx && FragmentOffset < 0)
        {
            chunkDocument[F("type")] = (ClientUpdateType_t::RebuildNeeded == TransferMode) ? UI_INITIAL_GUI : UI_EXTEND_GUI;
            CurrentSyncID = NextSyncID;
            NextSyncID = ESPUI.GetNextControlChangeId();
        }

        if (ESPUIcontrolMgr.prepareJSONChunk(startidx,
                                             chunkDocument,
                                             ClientUpdateType_t::UpdateNeeded == TransferMode,
                                             FragmentOffset,
                                             CurrentSyncID))
        {
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity >= Verbosity::VerboseJSON)
            {
                Serial.println(F("ESPUIclient:SendControlsToClient: Sending elements --------->"));
                serializeJson(chunkDocument, Serial);
                Serial.println();
            }
#endif
            SendJsonDocToWebSocket(chunkDocument);
        }
        else
        {
            Response = true;
        }
    } while (false);

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
        {
            max_buffer_len = len;
        }
        log_i("max_buffer_len %lu", (unsigned long)max_buffer_len);

        AsyncWebSocketMessageBuffer* buffer = ESPUI.WebSocket()->makeBuffer(len);
        if (!buffer)
        {
            Response = false;
            break;
        }

        serializeJson(document, buffer->get(), len);
        client->text(buffer);
    } while (false);

    return Response;
}

void ESPUIclient::SetState(ClientUpdateType_t value)
{
    if (uint32_t(ClientUpdateType) < uint32_t(value))
    {
        ClientUpdateType = value;
    }
}