#pragma once

#include <Arduino.h>
#include <PsychicHttp.h>
#include <atomic>
#include <ArduinoJson.h>
#include "ESPUIclientFsm.h"
#include "ESPUIcontrol.h"

enum class ESPUIWebSocketEvent { Connected, Data };

class ESPUIclient
{
public:
    enum ClientUpdateType_t
    { // this is an orderd list. highest number is highest priority
        Synchronized    = 0,
        UpdateNeeded    = 1,
        RebuildNeeded   = 2,
        ReloadNeeded    = 3,
    };

protected:
    // bool HasBeenNotified      = false;  // Set when a notification has been sent and we are waiting for a reply
    // bool DelayedNotification  = false;  // set if a delayed notification is needed

    ClientUpdateType_t ClientUpdateType = ClientUpdateType_t::RebuildNeeded;

    int socket = -1;
    std::atomic<bool> connected{true};
    std::atomic<bool> synchronized{false};

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

    time_t      EspuiClientEndTime = 0;

    // bool        NeedsNotification() { return pCurrentFsmState != &fsm_EspuiClient_state_Idle_imp; }

    bool        CanSend();
    void        FillInHeader(ArduinoJson::JsonDocument& document);
    bool        SendControlsToClient(uint16_t startidx, ClientUpdateType_t TransferMode, String FragmentRequest);

    bool        SendClientNotification(ClientUpdateType_t value);

private:
    uint32_t    CurrentSyncID = 0;
    uint32_t    NextSyncID = 0;

public:
                explicit ESPUIclient(int socket);
                ESPUIclient(const ESPUIclient & source) = delete;
    virtual     ~ESPUIclient();
    void        NotifyClient(ClientUpdateType_t value);
    bool        onWsEvent(ESPUIWebSocketEvent type, const uint8_t* data, size_t len);
    bool        IsSyncronized();
    void        MarkPending() { synchronized.store(false); }
    void        Disconnect() { connected.store(false); synchronized.store(false); }
    uint32_t    id() { return static_cast<uint32_t>(socket); }
    void        SetState(ClientUpdateType_t value);
    bool        SendJsonDocToWebSocket(ArduinoJson::JsonDocument& document);

};
