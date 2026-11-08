#pragma once

#include <Arduino.h>

class ESPUIclient;

class fsm_EspuiClient_state
{
public:
    fsm_EspuiClient_state() {}
    virtual ~fsm_EspuiClient_state() {}

    void Init();
    virtual bool NotifyClient() = 0;
    virtual void ProcessAck(uint16_t id, int32_t FragmentOffset = -1) = 0;
    virtual const __FlashStringHelper* GetStateName() = 0;
    void SetParent(ESPUIclient* value) { Parent = value; }

protected:
    ESPUIclient* Parent = nullptr;
};

class fsm_EspuiClient_state_Idle : public fsm_EspuiClient_state
{
public:
    fsm_EspuiClient_state_Idle() {}
    virtual ~fsm_EspuiClient_state_Idle() {}

    bool NotifyClient() override;
    void ProcessAck(uint16_t id, int32_t FragmentOffset = -1) override;
    const __FlashStringHelper* GetStateName() override { return F("Idle"); }
};

class fsm_EspuiClient_state_SendingUpdate : public fsm_EspuiClient_state
{
public:
    fsm_EspuiClient_state_SendingUpdate() {}
    virtual ~fsm_EspuiClient_state_SendingUpdate() {}

    bool NotifyClient() override;
    void ProcessAck(uint16_t id, int32_t FragmentOffset = -1) override;
    const __FlashStringHelper* GetStateName() override { return F("Sending Update"); }
};

class fsm_EspuiClient_state_Rebuilding : public fsm_EspuiClient_state
{
public:
    fsm_EspuiClient_state_Rebuilding() {}
    virtual ~fsm_EspuiClient_state_Rebuilding() {}

    void Init();
    bool NotifyClient() override;
    void ProcessAck(uint16_t id, int32_t FragmentOffset = -1) override;
    const __FlashStringHelper* GetStateName() override { return F("Sending Rebuild"); }
};

class fsm_EspuiClient_state_Reloading : public fsm_EspuiClient_state
{
public:
    fsm_EspuiClient_state_Reloading() {}
    virtual ~fsm_EspuiClient_state_Reloading() {}

    void Init();
    bool NotifyClient() override;
    void ProcessAck(uint16_t id, int32_t FragmentOffset = -1) override;
    const __FlashStringHelper* GetStateName() override { return F("Reloading"); }
};