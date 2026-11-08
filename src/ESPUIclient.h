#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "ESPUIclientFsm.h"
#include "ESPUIcontrol.h"

class ESPUIclient
{
public:
    enum ClientUpdateType_t
    {
        Synchronized  = 0,
        UpdateNeeded  = 1,
        RebuildNeeded = 2,
        ReloadNeeded  = 3,
    };

protected:
    ClientUpdateType_t ClientUpdateType = ClientUpdateType_t::RebuildNeeded;
    AsyncWebSocketClient* client = nullptr;

    friend class fsm_EspuiClient_state_Idle;
    friend class fsm_EspuiClient_state_SendingUpdate;
    friend class fsm_EspuiClient_state_Rebuilding;
    friend class fsm_EspuiClient_state_WaitForAck;
    friend class fsm_EspuiClient_state_Reloading;
    friend class fsm_EspuiClient_state;

    fsm_EspuiClient_state_Idle          fsm_EspuiClient_state_Idle_imp;
    fsm_EspuiClient_state_SendingUpdate fsm_EspuiClient_state_SendingUpdate_imp;
    fsm_EspuiClient_state_Rebuilding    fsm_EspuiClient_state_Rebuilding_imp;
    fsm_EspuiClient_state_Reloading     fsm_EspuiClient_state_Reloading_imp;
    fsm_EspuiClient_state* pCurrentFsmState = &fsm_EspuiClient_state_Idle_imp;

    bool CanSend();
    void FillInHeader(ArduinoJson::JsonDocument& document);
    bool SendControlsToClient(uint16_t startidx, ClientUpdateType_t TransferMode, int32_t FragmentOffset = -1);
    bool SendClientNotification(ClientUpdateType_t value);

private:
    uint32_t CurrentSyncID = 0;
    uint32_t NextSyncID = 0;

    JsonDocument chunkDocument;

public:
    ESPUIclient(AsyncWebSocketClient* _client);
    ESPUIclient(const ESPUIclient& source);
    virtual ~ESPUIclient();

    void NotifyClient(ClientUpdateType_t value);
    bool onWsEvent(AwsEventType type, void* arg, uint8_t* data, size_t len);
    bool IsSyncronized();
    uint32_t id() { return client->id(); }
    void SetState(ClientUpdateType_t value);
    bool SendJsonDocToWebSocket(ArduinoJson::JsonDocument& document);
};