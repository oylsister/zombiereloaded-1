/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          zcookies.inc
 *  Type:          Module
 *  Description:   ZCookies module, allows clients to modify their ZR cookies via menu.
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
 * Create commands specific to ZHP.
 */
ZCookiesOnCommandsCreate()
{
    // Register ZCookies command.
    RegConsoleCmd(SAYHOOKS_KEYWORD_ZCOOKIES, ZCookiesCommand, "Toggle all ZR cookies here.");
}

/**
 * Show ZCookies menu to client.
 *
 * @param client    The client index.
 */
ZCookiesMenuMain(client)
{
    // Create menu handle.
    new Handle:zcookies_menu_main = CreateMenu(ZCookiesMenuMainHandle);

    // Make client global translations target.
    SetGlobalTransTarget(client);

    decl String:zhpenabled[MENU_LINE_SMALL_LENGTH];
    decl String:overlayenabled[MENU_LINE_SMALL_LENGTH];

    // Get the current toggle state of the cookies.
    ConfigBoolToSetting(CookiesGetClientCookieBool(client, g_hZHPEnabledCookie), zhpenabled, sizeof(zhpenabled), false, client);
    ConfigBoolToSetting(CookiesGetClientCookieBool(client, g_hOverlayEnabledCookie), overlayenabled, sizeof(overlayenabled), false, client);

    decl String:title[MENU_LINE_TITLE_LENGTH];
    decl String:zhp[MENU_LINE_REG_LENGTH];
    decl String:overlay[MENU_LINE_REG_LENGTH];
    decl String:zmarket[MENU_LINE_REG_LENGTH];

    // Translate each line into client's language.
    Format(title, sizeof(title), "%t\n ", "ZCookies Menu main title");
    Format(zhp, sizeof(zhp), "%t", "ZCookies menu main zhp", zhpenabled);
    Format(overlay, sizeof(overlay), "%t", "ZCookies menu main overlay", overlayenabled);
    Format(zmarket, sizeof(zmarket), "%t", "ZCookies zmarket loadout");

    // Get conditional values for each option.
    new bool:zhpcvar = GetConVarBool(g_hCvarsList.CVAR_ZHP);                                   // For ZHP.
    new bool:overlaytoggle = GetConVarBool(g_hCvarsList.CVAR_CLASSES_OVERLAY_TOGGLE);          // For class overlay.
    new bool:zmarketenabled = GetConVarBool(g_hCvarsList.CVAR_WEAPONS_ZMARKET);                // For ZMarket loadout.

    // Add items to menu.
    SetMenuTitle(zcookies_menu_main, title);
    AddMenuItem(zcookies_menu_main, "zhp", zhp, MenuGetItemDraw(zhpcvar));
    AddMenuItem(zcookies_menu_main, "overlay", overlay, MenuGetItemDraw(overlaytoggle));
    AddMenuItem(zcookies_menu_main, "zmarket", zmarket, MenuGetItemDraw(zmarketenabled));

    // Create a "Back" button to the main menu.
    SetMenuExitBackButton(zcookies_menu_main, true);

    // Display menu to client.
    DisplayMenu(zcookies_menu_main, client, MENU_TIME_FOREVER);
}

/**
 * Menu callback (main)
 * Toggles client cookies.
 *
 * @param menu      The menu handle.
 * @param action    Action client is doing in menu.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public ZCookiesMenuMainHandle(Handle:menu, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
    {
        new bool:resend = true;

        switch(slot)
        {
            // Toggled ZHP.
            case 0:
            {
                ZHPToggle(client);
            }
            // Toggled class overlay.
            case 1:
            {
                // If overlay toggle is disabled, then stop.
                new bool:overlaytoggle = GetConVarBool(g_hCvarsList.CVAR_CLASSES_OVERLAY_TOGGLE);
                if (overlaytoggle)
                {
                    // Toggle current overlay channel, retrieve new value, and update cookie.
                    new bool:overlayenabled = OverlaysClientSetChannelState(client, OVERLAYS_CHANNEL_CLASSES, true, true);
                    CookiesSetClientCookieBool(client, g_hOverlayEnabledCookie, overlayenabled);
                }
            }
            // Opened ZMarket loadout.
            case 2:
            {
                // Show a client their current loadout.
                ZMarketMenuLoadout(client);

                // Don't resend ZCookies.
                resend = false;
            }
        }

        if (resend)
        {
            // Re-send menu.
            ZCookiesMenuMain(client);
        }
    }
    // Client closed the menu.
    if (action == MenuAction_Cancel)
    {
        // Client hit "Back" button.
        if (slot == MenuCancel_ExitBack)
        {
            // Re-open main menu.
            ZMenuMain(client);
        }
    }
    // Client exited menu.
    if (action == MenuAction_End)
    {
        CloseHandle(menu);
    }
}

/**
 * Command callback (zcookies)
 * Toggle all ZR cookies here.
 *
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:ZCookiesCommand(client, argc)
{
    // If client is console, then stop and tell them this feature is for players only.
    if (ZRIsConsole(client))
    {
        TranslationPrintToServer("Must be player");
        return Plugin_Handled;
    }

    // Send ZCookies menu.
    ZCookiesMenuMain(client);

    // This stops the "Unknown command" message in client's console.
    return Plugin_Handled;
}