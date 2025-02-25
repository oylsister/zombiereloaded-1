/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          menu.inc
 *  Type:          Core
 *  Description:   Base menu functions for the plugin.
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
 * @section Some defined lengths of menu lines throughout the plugin.
 */
#define MENU_LINE_SMALL_LENGTH 32
#define MENU_LINE_REG_LENGTH 64
#define MENU_LINE_BIG_LENGTH 128
#define MENU_LINE_HUGE_LENGTH 256

#define MENU_LINE_TITLE_LENGTH MENU_LINE_BIG_LENGTH
/**
 * @endsection
 */

/**
 * Create commands specific to ZMenu.
 */
MenuOnCommandsCreate()
{
    // Register ZMenu command.
    RegConsoleCmd(SAYHOOKS_KEYWORD_ZMENU, ZMenuCommand, "Opens ZR's main menu.");
}

/**
 * Command callback (zmenu)
 * Opens ZR's main menu.
 *
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:ZMenuCommand(client, argc)
{
    // If client is console, then stop and tell them this feature is for players only.
    if (ZRIsConsole(client))
    {
        TranslationPrintToServer("Must be player");
        return Plugin_Handled;
    }

    // Send main menu.
    ZMenuMain(client);

    // This stops the "Unknown command" message in client's console.
    return Plugin_Handled;
}

/**
 * Show main menu to client.
 *
 * @param client    The client index.
 */
ZMenuMain(client)
{
    // Create menu handle.
    new Handle:menu_main = CreateMenu(ZMenuMainHandle);

    SetGlobalTransTarget(client);

    // Initialize menu lines.
    decl String:title[MENU_LINE_TITLE_LENGTH];
    decl String:zadmin[MENU_LINE_HUGE_LENGTH];
    decl String:zclass[MENU_LINE_HUGE_LENGTH];
    decl String:zcookies[MENU_LINE_HUGE_LENGTH];
    decl String:zspawn[MENU_LINE_HUGE_LENGTH];
    decl String:ztele[MENU_LINE_HUGE_LENGTH];
    decl String:zhp[MENU_LINE_HUGE_LENGTH];
    decl String:zmarket[MENU_LINE_HUGE_LENGTH];
    decl String:zvolume[MENU_LINE_HUGE_LENGTH];

    // Translate each line into client's language.
    Format(title, sizeof(title), "%t\n ", "Menu main title", SAYHOOKS_CHAT_PUBLIC_DEFAULT, SAYHOOKS_CHAT_SILENT_DEFAULT);
    Format(zadmin, sizeof(zadmin), "%t", "Menu main zadmin");
    Format(zclass, sizeof(zclass), "%t", "Menu main zclass");
    Format(zcookies, sizeof(zcookies), "%t", "Menu main zcookies");
    Format(zspawn, sizeof(zspawn), "%t", "Menu main zspawn");
    Format(ztele, sizeof(ztele), "%t", "Menu main ztele");
    Format(zhp, sizeof(zhp), "%t", "Menu main zhp");
    Format(zmarket, sizeof(zmarket), "%t", "Menu main zmarket");
    Format(zvolume, sizeof(zvolume), "%t", "Menu main zvolume");

    // Add items to menu.

    SetMenuTitle(menu_main, title);

    // Disable option if client isn't an admin.
    new bool:admin = ZRIsClientAdmin(client);
    AddMenuItem(menu_main, "zadmin", zadmin, MenuGetItemDraw(admin));

    // Decide whether the client can use zclass.
    new zclassdraw = ClassAllowSelection(client) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED;

    AddMenuItem(menu_main, "zclass", zclass, zclassdraw);
    AddMenuItem(menu_main, "zcookies", zcookies);
    AddMenuItem(menu_main, "zspawn", zspawn);
    AddMenuItem(menu_main, "ztele", ztele);
    AddMenuItem(menu_main, "zhp", zhp);
    AddMenuItem(menu_main, "zmarket", zmarket);
    AddMenuItem(menu_main, "zvolume", zvolume);

    // Display menu to client.
    DisplayMenu(menu_main, client, MENU_TIME_FOREVER);
}

/**
 * Menu callback (main)
 * Redirects client to selected option's handle code.
 *
 * @param menu      The menu handle.
 * @param action    Action client is doing in menu.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public ZMenuMainHandle(Handle:menu, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
    {
        // Create variable to possible resend menu later.
        new bool:resend = true;

        switch(slot)
        {
            // Selected ZAdmin.
            case 0:
            {
                // Copy return to resend variable.
                resend = !ZAdminMenu(client);
            }
            // Select ZClass.
            case 1:
            {
                // Send ZClass menu
                ClassMenuMain(client);

                // Don't resend this menu.
                resend = false;
            }
            // Select ZCookies.
            case 2:
            {
                // Send ZCookies menu
                ZCookiesMenuMain(client);

                // Don't resend this menu.
                resend = false;
            }
            // Select ZSpawn.
            case 3:
            {
                // Send ZSpawn command from client.
                ZSpawnClient(client);
            }
            // Select ZTele.
            case 4:
            {
                // Copy return to resend variable.
                resend = !ZTeleClient(client);
            }
            // Select ZHP.
            case 5:
            {
                // Toggle ZHP.
                ZHPToggle(client);
            }
            // Select ZMarket.
            case 6:
            {
                // Send ZMarket menu.
                resend = !ZMarketMenuMain(client);
            }
            // Select ZVolume.
            case 7:
            {
                // Send ZVolume menu.
                VolumeControlMenu(client);

                // Don't resend this menu.
                resend = false;
            }
        }

        // Resend is still true, then resend menu.
        if (resend)
        {
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
 * Shows a list of all clients to a client, different handlers can be used for this, as well as title.
 *
 * @param client    The client index.
 * @param handler   The menu handler.
 * @param team      If true, only clients on a team will be displayed.
 * @param alive     If true, only clients that are alive will be displayed.
 * @param dead      If true, only clients that are dead will be displayed.
 * @param any       Title is a translations phrase.
 */
stock MenuClientList(client, MenuHandler:handler, bool:team = false, bool:alive = false, bool:dead = false, any:...)
{
    // Create menu handle.
    new Handle:menu_clients = CreateMenu(handler);

    // Set client as translation target.
    SetGlobalTransTarget(client);

    // Translate phrase.
    decl String:translation[MENU_LINE_TITLE_LENGTH];
    VFormat(translation, sizeof(translation), "%t", 6);

    // Set menu title to the translated phrase.
    SetMenuTitle(menu_clients, translation);

    decl String:clientoption[MENU_LINE_REG_LENGTH];
    decl String:clientuserid[8];

    new count = 0;

    // x = Client index.
    for (new x = 1; x <= MaxClients; x++)
    {
        // If client isn't in-game, then stop.
        if (!IsClientInGame(x))
        {
            continue;
        }

        // If client isn't on a team, then stop.
        if (team && !ZRIsClientOnTeam(x))
        {
            continue;
        }

        // If client is dead, then stop.
        if (alive && !IsPlayerAlive(x))
        {
            continue;
        }

        // If client is alive, then stop.
        if (dead && IsPlayerAlive(x))
        {
            continue;
        }

        // Get client info.
        GetClientName(x, clientoption, sizeof(clientoption));
        IntToString(GetClientUserId(x), clientuserid, sizeof(clientuserid));

        // Add option to menu.
        AddMenuItem(menu_clients, clientuserid, clientoption);

        // Increment count.
        count++;
    }

    // If there are no clients, add an "(Empty)" line.
    if (count == 0)
    {
        // Set translation language again, because SetMenuTitle above reset
        // this for some reason.
        SetGlobalTransTarget(client);

        decl String:empty[64];
        Format(empty, sizeof(empty), "%t", "Menu empty");

        AddMenuItem(menu_clients, "empty", empty, ITEMDRAW_DISABLED);
    }

    // Create a "Back" button to the main admin menu.
    SetMenuExitBackButton(menu_clients, true);

    // Send menu.
    DisplayMenu(menu_clients, client, MENU_TIME_FOREVER);
}

/**
 * Gets the client index of the selected client in the menu.
 *
 * @param menu  The menu handle.
 * @param slot  The menu slot that was selected.
 * @return      The client index, 0 if the selected client is no longer in the server.
 */
stock MenuGetClientIndex(Handle:menu, slot)
{
    // Get menu slot's information.
    decl String:clientuserid[8];
    GetMenuItem(menu, slot, clientuserid, sizeof(clientuserid));

    // Return the targetted client through their userid which was set into the menu slot's info param.
    return GetClientOfUserId(StringToInt(clientuserid));
}

/**
 * Return itemdraw flag for SM menus.
 *
 * @param condition     If this is true, item will be drawn normally.
 */
stock MenuGetItemDraw(bool:condition)
{
    return condition ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED;
}
