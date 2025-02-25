/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          overlays.inc
 *  Type:          Core
 *  Description:   Overlay system, separating different types into "overlay channels."
 *
 *  Copyright (C) 2009-2013  Greyscale, Richard Helgeby
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================================
 */

/**
 * Maximum amount of overlay channels.
 */
#define OVERLAYS_CHANNEL_MAX 2

/**
 * All possible overlay channels, in order of priority.
 */
enum OverlaysChannel
{
    OVERLAYS_CHANNEL_NONE = -1,      /** Client has no overlay */
    OVERLAYS_CHANNEL_ROUNDEND = 0,  /** Round win overlay */
    OVERLAYS_CHANNEL_CLASSES = 1,   /** Class overlay */
    OVERLAYS_CHANNEL_MAXSIZE
}

/**
 * Global variable to store a convar query cookie
 */
new QueryCookie:mat_dxlevel;

/**
 * The DirectX level of a client.
 */
new g_iOverlaysDXL[MAXPLAYERS + 1];

/**
 * Array to track overlay channel state on each client.
 */
new bool:g_bOverlayChannel[MAXPLAYERS + 1][OVERLAYS_CHANNEL_MAXSIZE];

/**
 * Array to store overlay path for each channel.
 */
new String:g_strOverlayPath[MAXPLAYERS + 1][OVERLAYS_CHANNEL_MAXSIZE][PLATFORM_MAX_PATH];

/**
 * Create variable to store global timer handle.
 */
new Handle:tOverlays = INVALID_HANDLE;

/**
 * Map is starting.
 */
OverlaysOnMapStart()
{
    // Reset timer handle.
    tOverlays = INVALID_HANDLE;
}

/**
 * Client is joining the server.
 *
 * @param client    The client index.
 */
OverlaysClientInit(client)
{
    // x = channel index.
    for (new x = 0; x < OVERLAYS_CHANNEL_MAX; x++)
    {
        // Disable all channels, and reset.
        OverlaysClientSetChannelState(client, OverlaysChannel:x, false, false, false, true);
    }

    // Get client's DX level.
    OverlaysGetClientDXLevel(client);
}

/**
 * Finds DX level of a client.
 *
 * @param client    The client index.
 */
OverlaysGetClientDXLevel(client)
{
    // If client is fake (or bot), then stop.
    if (IsFakeClient(client))
    {
        return;
    }

    // Query mat_dxlevel on client.
    mat_dxlevel = QueryClientConVar(client, "mat_dxlevel", OverlaysQueryClientDXLevel);
}

/**
 * Query callback function.
 *
 * @param cookie    Unique cookie of the query.
 * @param client    The client index.
 * @param result    The result of the query (see console.inc enum ConVarQueryResult)
 * @param cvarName  Name of the cvar.
 * @param cvarValue Value of the cvar.
 */
public OverlaysQueryClientDXLevel(QueryCookie:cookie, client, ConVarQueryResult:result, const String:cvarName[], const String:cvarValue[])
{
    // If query cookie does not match cookie given by mat_dxlevel query, then stop, this isn't our query.
    if (cookie != mat_dxlevel)
    {
        return;
    }

    // Reset dxLevel.
    g_iOverlaysDXL[client] = 0;

    // If result is any other than ConVarQuery_Okay, then stop.
    if (result != ConVarQuery_Okay)
    {
        return;
    }

    // Copy cvar value to dxLevel array.
    g_iOverlaysDXL[client] = StringToInt(cvarValue);
}

/**
 * The round is starting.
 */
OverlaysOnRoundStart()
{
    // If timer is running, kill it.
    if (tOverlays != INVALID_HANDLE)
    {
        KillTimer(tOverlays);
        tOverlays = INVALID_HANDLE;
    }

    // If antistick is disabled, then stop.
    new Float:overlaysupdate = GetConVarFloat(g_hCvarsList.CVAR_OVERLAYS_UPDATE_TIME);
    if (overlaysupdate <= 0.0)
    {
        return;
    }

    // Start repeating timer.
    tOverlays = CreateTimer(overlaysupdate, OverlaysTimer, _, TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE);
}

/**
 * Update overlay on a client. (Displays highest priority overlays first, if enabled.)
 *
 * @param client        The client index.
 * @param channel       (Optional) The channel overlay to update.
 */
OverlaysClientUpdateOverlay(client, OverlaysChannel:channel = OVERLAYS_CHANNEL_NONE)
{
    // Find highest priority, enabled, overlay channel of client.
    if (channel == OVERLAYS_CHANNEL_NONE)
    {
        channel = OverlaysClientFindChannel(client);
    }

    // Stop here if client has no overlay channel enabled.
    if (channel == OVERLAYS_CHANNEL_NONE)
    {
        // Clear any existing overlay from screen.
        ClientCommand(client, "r_screenoverlay \"\"");
        return;
    }

    // If channel we are updating is disabled, then stop.
    if (!g_bOverlayChannel[client][channel])
    {
        return;
    }

    // If dxLevel is 0, then query on client failed, so try again, then stop.
    if (!g_iOverlaysDXL[client])
    {
        // Query dxlevel cvar, again.
        OverlaysGetClientDXLevel(client);
        return;
    }

    // If client doesn't meet DXLevel requirement, then tell client and stop.
    new minDxLevel = GetConVarInt(g_hCvarsList.CVAR_OVERLAYS_MIN_DXLEVEL);
    if (g_iOverlaysDXL[client] < minDxLevel)
    {
        TranslationPrintCenterText(client, "Overlays not supported", g_iOverlaysDXL[client], minDxLevel);
        return;
    }

    // Display overlay to client.
    ClientCommand(client, "r_screenoverlay \"%s\"", g_strOverlayPath[client][channel]);
}

OverlaysChannel:OverlaysClientFindChannel(client)
{
    // x = channel index.
    for (new x = 0; x < OVERLAYS_CHANNEL_MAX; x++)
    {
        // Convert to OverlaysChannel datatype.
        new OverlaysChannel:channel = OverlaysChannel:x;

        if (OverlaysClientGetChannelState(client, channel))
        {
            // Return channel.
            return channel;
        }
    }

    return OVERLAYS_CHANNEL_NONE;
}

/**
 * Get current value of a channel state of a client.
 *
 * @param client    The client index.
 * @param channel   The channel to get state of.
 */
bool:OverlaysClientGetChannelState(client, OverlaysChannel:channel)
{
    // Return current value.
    return g_bOverlayChannel[client][channel];
}

/**
 * Toggle or set new value to a channel state of a client.
 *
 * @param client    The client index.
 * @param channel   The channel to change state of.
 * @param update    (Optional) Update the overlay when this function is called.
 * @param toggle    (Optional) Set to true to toggle state, false to use value param.
 * @param value     (Optional) New value of the state, only used if toggle is false.
 * @param reset     (Optional) Resets the channel path.
 * @return          The overlay's new state.
 */
bool:OverlaysClientSetChannelState(client, OverlaysChannel:channel, bool:update = false, bool:toggle = true, bool:value = false, bool:reset = false)
{
    // Toggle or set new state to channel of a client.
    g_bOverlayChannel[client][channel] = toggle ? !g_bOverlayChannel[client][channel] : value;

    if (update)
    {
        // Update client overlay.
        OverlaysClientUpdateOverlay(client);
    }

    if (reset)
    {
        OverlaysClientSetChannelPath(client, channel, "");
    }

    // Return new value.
    return g_bOverlayChannel[client][channel];
}

/**
 * Set overlay path for a channel.
 *
 * @param client    The client index.
 * @param channel   The channel to set path on.
 * @param path      Path to overlay.
 */
OverlaysClientSetChannelPath(client, OverlaysChannel:channel, const String:path[])
{
    // Copy path to the overlay channel's path string.
    strcopy(g_strOverlayPath[client][channel], PLATFORM_MAX_PATH, path);
}

/**
 * Timer callback, updates overlay on each client.
 *
 * @param timer     The timer handle.
 */
public Action:OverlaysTimer(Handle:timer)
{
    // x = client index
    for (new x = 1; x <= MaxClients; x++)
    {
        // If client isn't in game, then stop.
        if (!IsClientInGame(x))
        {
            continue;
        }

        // If no overlay is on the client's screen, then stop.
        if (OverlaysClientFindChannel(x) == OVERLAYS_CHANNEL_NONE)
        {
            continue;
        }

        // Update client's overlay.
        OverlaysClientUpdateOverlay(x);
    }
}
