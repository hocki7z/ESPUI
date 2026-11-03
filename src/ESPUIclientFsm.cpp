#include "ESPUI.h"
#include "ESPUIclient.h"

void fsm_EspuiClient_state::Init()
{
    Parent->pCurrentFsmState = this;
}

bool fsm_EspuiClient_state_Idle::NotifyClient()
{
    bool Response = false;

    ClientUpdateType_t TypeToProcess = Parent->ClientUpdateType;
    Parent->ClientUpdateType = ClientUpdateType_t::Synchronized;

    switch (TypeToProcess)
    {
        case ClientUpdateType_t::Synchronized:
            Response = true;
            break;

        case ClientUpdateType_t::UpdateNeeded:
            Parent->fsm_EspuiClient_state_SendingUpdate_imp.Init();
            Response = Parent->SendClientNotification(ClientUpdateType_t::UpdateNeeded);
            break;

        case ClientUpdateType_t::RebuildNeeded:
            Parent->fsm_EspuiClient_state_Rebuilding_imp.Init();
            Response = Parent->SendClientNotification(ClientUpdateType_t::RebuildNeeded);
            break;

        case ClientUpdateType_t::ReloadNeeded:
            Parent->fsm_EspuiClient_state_Reloading_imp.Init();
            Response = Parent->SendClientNotification(ClientUpdateType_t::ReloadNeeded);
            break;
    }
    return Response;
}

void fsm_EspuiClient_state_Idle::ProcessAck(uint16_t ControlIndex, int32_t FragmentOffset)
{
    if (FragmentOffset >= 0)
    {
        Parent->SendControlsToClient(ControlIndex, ClientUpdateType_t::UpdateNeeded, FragmentOffset);
    }
    else
    {
        Parent->NotifyClient(ClientUpdateType_t::RebuildNeeded);
    }
}

bool fsm_EspuiClient_state_SendingUpdate::NotifyClient()
{
    return true;
}

void fsm_EspuiClient_state_SendingUpdate::ProcessAck(uint16_t ControlIndex, int32_t FragmentOffset)
{
    if (Parent->SendControlsToClient(ControlIndex, ClientUpdateType_t::UpdateNeeded, FragmentOffset))
    {
        Parent->fsm_EspuiClient_state_Idle_imp.Init();
        Parent->fsm_EspuiClient_state_Idle_imp.NotifyClient();
    }
}

void fsm_EspuiClient_state_Rebuilding::Init()
{
    Parent->CurrentSyncID = 0;
    Parent->NextSyncID = 0;
    Parent->pCurrentFsmState = this;
}

bool fsm_EspuiClient_state_Rebuilding::NotifyClient()
{
    return true;
}

void fsm_EspuiClient_state_Rebuilding::ProcessAck(uint16_t ControlIndex, int32_t FragmentOffset)
{
    if (Parent->SendControlsToClient(ControlIndex, ClientUpdateType_t::RebuildNeeded, FragmentOffset))
    {
        Parent->fsm_EspuiClient_state_Idle_imp.Init();
        Parent->fsm_EspuiClient_state_Idle_imp.NotifyClient();
    }
}

void fsm_EspuiClient_state_Reloading::Init()
{
    Parent->CurrentSyncID = 0;
    Parent->NextSyncID = 0;
    Parent->pCurrentFsmState = this;
}

void fsm_EspuiClient_state_Reloading::ProcessAck(uint16_t ControlIndex, int32_t FragmentOffset)
{
    if (FragmentOffset >= 0)
    {
        Parent->SendControlsToClient(ControlIndex, ClientUpdateType_t::UpdateNeeded, FragmentOffset);
    }
}

bool fsm_EspuiClient_state_Reloading::NotifyClient()
{
    return true;
}