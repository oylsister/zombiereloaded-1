/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          classevents.inc
 *  Type:          Core
 *  Description:   Functions for handling class related events.
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
 * Keeps track of if a client has been authorized as an admin.
 */
new bool:g_bAdminChecked[MAXPLAYERS + 1];

/* ------------------------------------
 *
 * GAME EVENTS
 *
 * ------------------------------------
 */

/**
 * Create class-related cookies here.
 */
ClassOnCookiesCreate()
{
    // Forward event to sub-modules.
    ClassOverlayOnCookiesCreate();

    // Create cookie handles only if they don't exist.
    if (g_hClassCookieClassSelected[ZR_CLASS_TEAM_HUMANS] == INVALID_HANDLE)
    {
        g_hClassCookieClassSelected[ZR_CLASS_TEAM_HUMANS] = RegClientCookie("zr_humanclass", "The last human class selected.", CookieAccess_Protected);
    }
    if (g_hClassCookieClassSelected[ZR_CLASS_TEAM_ZOMBIES] == INVALID_HANDLE)
    {
        g_hClassCookieClassSelected[ZR_CLASS_TEAM_ZOMBIES] = RegClientCookie("zr_zombieclass", "The last zombie class selected.", CookieAccess_Protected);
    }
    if (g_hClassCookieClassSelected[ZR_CLASS_TEAM_ADMINS] == INVALID_HANDLE)
    {
        g_hClassCookieClassSelected[ZR_CLASS_TEAM_ADMINS] = RegClientCookie("zr_adminclass", "The last admin mode class selected.", CookieAccess_Protected);
    }
}

/**
 * Called when all modules are done loading.
 */
ClassOnModulesLoaded()
{
    // Set default classes on all player slots.
    ClassClientSetDefaultIdentifiers();
}

/**
 * Called when map is loading, before configs are loaded. Used for
 * initializing class module.
 */
ClassOnMapStart()
{
    // Clear multipliers.
    ClassResetMultiplierCache();

    // Prepare hp regeneration module.
    ClassHealthRegenInit();
}

/**
 * Called when all configs are executed.
 */
ClassOnConfigsExecuted()
{
    new ClassSpeedMethods:speedMethod = ClassGetSpeedMethod();

    if (speedMethod != ClassSpeed_Invalid)
    {
        // Set speed method.
        ClassSpeedMethod = speedMethod;
    }
    else
    {
        // Fall back on default to avoid errors.
        ClassSpeedMethod = ClassSpeed_Prop;
        LogEvent(false, LogType_Error, LOG_CORE_EVENTS, LogModules:LogModule_Playerclasses, "Config validation", "Warning: Invalid value in zr_classes_speed_method. Using default value.");
    }
}

/**
 * Client has just connected to the server.
 */
ClassOnClientConnected(client)
{
    // Unhook "PreThinkPost" on the client.
    SDKUnhook(client, SDKHook_PreThinkPost, ClassPreThinkPost);

    // Initialize the admin checked variable.
    g_bAdminChecked[client] = false;
}

/**
 * Called when a client connects to the server (OnClientPutInServer).
 */
ClassClientInit(client)
{
    // Hook "PreThinkPost" on the client.
    SDKHook(client, SDKHook_PreThinkPost, ClassPreThinkPost);

    // Reset spawn flag.
    ClassPlayerSpawned[client] = false;
}

/**
 * Called once a client is authorized and fully in-game, and
 * after all post-connection authorizations have been performed.
 *
 * This callback is gauranteed to occur on all clients, and always
 * after each OnClientPutInServer() call.
 *
 * @param client        Client index.
 * @noreturn
 */
ClassOnClientPostAdminCheck(client)
{
    // Client has been checked.
    g_bAdminChecked[client] = true;

    // Below this depends on client cookies.
    if (!AreClientCookiesCached(client))
        return;

    // Check if classes are loaded successfully and the client is valid.
    if (ClassValidated)
    {
        // Set default class indexes on the player.
        ClassClientSetDefaultIdentifiers(client);
    }
}

/**
 * Called once a client's saved cookies have been loaded from the database.
 *
 * @param client        Client index.
 */
ClassOnCookiesCached(client)
{
    // Check if classes are loaded successfully.
    if (ClassValidated)
    {
        // Forward event to sub-modules.
        ClassOverlayOnCookiesCached(client);
    }

    // Below this depends on client authorization.
    if (!g_bAdminChecked[client])
        return;

    // Check if classes are loaded successfully and the client is valid.
    if (ClassValidated)
    {
        // Set default class indexes on the player.
        ClassClientSetDefaultIdentifiers(client);
    }
}

/**
 * Called a client disconnects.
 */
ClassOnClientDisconnect(client)
{
    // Disable class attributes with timers.
    ClassHealthRegenStop(client);

    // Reset previously selected class indexes.
    ClassResetNextIndexes(client);
}

/**
 * Client is spawning into the game.
 *
 * @param client    The client index.
 */
ClassOnClientSpawn(client)
{
    decl String:originalmodel[PLATFORM_MAX_PATH];
    decl String:classname[64];
    ClassFilter filter;

    // Check if the player is dead. Spawning into the game is also a event in
    // the connection process.
    if (!IsPlayerAlive(client))
    {
        return;
    }

    // Check if there are no valid classes. Block this event if classes aren't
    // done loading.
    if (!ClassValidated)
    {
        return;
    }

    // Reset attributes by triggering death event.
    ClassOnClientDeath(client);

    // Restore class indexes to be selected on spawn, if available.
    ClassRestoreNextIndexes(client);

    // Cache original player model.
    GetClientModel(client, originalmodel, sizeof(originalmodel));
    strcopy(ClassOriginalPlayerModel[client], PLATFORM_MAX_PATH, originalmodel);

    // Check if the player should spawn in admin mode.
    if (ClassPlayerInAdminMode[client])
    {
        // Mark player as in admin mode.
        ClassPlayerInAdminMode[client] = true;

        // TODO: This is the place to initialize admin mode stuff like no-block
        //       and other stuff.
    }
    else
    {
        // Mark player as not in admin mode.
        ClassPlayerInAdminMode[client] = false;

        // Get random class setting.
        new bool:randomclass = GetConVarBool(g_hCvarsList.CVAR_CLASSES_RANDOM);

        // Assign random classes if enabled. Always do it for bots.
        if (randomclass || IsFakeClient(client))
        {
            // Setup filtering
            // ---------------

            // Exclude special class flags like mother zombies and admin classes.
            filter.ClassFilter_DenyFlags = ZR_CLASS_SPECIALFLAGS;

            // Allow admin classes if admin.
            filter.ClassFilter_DenyFlags -= ZRIsClientAdmin(client) ? ZR_CLASS_FLAG_ADMIN_ONLY : 0;

            // Specify client for checking group permissions.
            filter.ClassFilter_Client = client;

            // Get classes
            // -----------

            // Get random classes for each type.
            new randomzombie = ClassGetRandomClass(ZR_CLASS_TEAM_ZOMBIES, filter);
            new randomhuman = ClassGetRandomClass(ZR_CLASS_TEAM_HUMANS, filter);

            // Set selected zombie class index.
            ClassSelected[client][ZR_CLASS_TEAM_ZOMBIES] = randomzombie;
            ClassGetName(randomzombie, classname, sizeof(classname), ZR_CLASS_TEAM_ZOMBIES);
            TranslationPrintToChat(client, "Classes random assignment", classname);

            // Set selected human class index.
            ClassSelected[client][ZR_CLASS_TEAM_HUMANS] = randomhuman;
            ClassGetName(randomhuman, classname, sizeof(classname), ZR_CLASS_TEAM_HUMANS);
            TranslationPrintToChat(client, "Classes random assignment", classname);
        }

        // Display class menu if either menu cvar is set.
        new bool:menuspawn = GetConVarBool(g_hCvarsList.CVAR_CLASSES_MENU_SPAWN);
        new bool:menujoin = GetConVarBool(g_hCvarsList.CVAR_CLASSES_MENU_JOIN);
        if (menuspawn || (menujoin && !ClassPlayerSpawned[client]))
        {
            ClassPlayerSpawned[client] = true;
            ClassMenuMain(client);
        }
    }

    // Load class attributes for the active class.
    ClassReloadPlayerCache(client, ClassGetActiveIndex(client));

    // Note: Class attributes are applied in ClassOnClientSpawnPost.

    // Check if instant class change cvar is set.
    new Float:instantspawn = GetConVarFloat(g_hCvarsList.CVAR_CLASSES_CHANGE_TIMELIMIT);
    if (instantspawn > 0)
    {
        // Allow instant class change.
        ClassAllowInstantChange[client] = true;

        // Create timer to disable instant change.
        CreateTimer(instantspawn, Event_ClassDisableInstantSpawn, client, TIMER_FLAG_NO_MAPCHANGE);
    }
    else
    {
        // Make sure instant change is not allowed.
        ClassAllowInstantChange[client] = false;
    }
}

/**
 * Client have just spawned (delayed event).
 *
 * @param client    The client index.
 */
ClassOnClientSpawnPost(client)
{
    // Check if there are no valid classes. Block this event if classes aren't
    // done loading.
    if (!ClassValidated)
    {
        return;
    }

    ClassApplyAttributes(client);
}

/**
 * Client died. Stops timers and reset certain attributes. Call this event to
 * clean up class related stuff.
 *
 * @param client    The client index.
 */
ClassOnClientDeath(client)
{
    // Disable class attributes with timers.
    ClassHealthRegenStop(client);

    // Set client's FOV back to normal.
    ToolsSetClientDefaultFOV(client, 90);

    // Forward event to sub-modules.
    ClassOverlayOnClientDeath(client);
}

/**
 * Client got infected. Reloads class attributes.
 *
 * @param client    The client index.
 */
ClassOnClientInfected(client, bool:motherzombie = false)
{
    new classindex = ClassGetActiveIndex(client);
    new isadmin;
    new motherindex;
    ClassFilter filter;

    decl String:motherzombiesetting[64];

    // Disable class attributes with timers.
    ClassHealthRegenStop(client);

    // Make sure the player is not allowed to instantly change class.
    ClassAllowInstantChange[client] = false;

    // Check if it's a mother zombie.
    if (motherzombie)
    {
        // Set admin flag if client is admin, so it's removed in special class
        // flags.
        isadmin = ZRIsClientAdmin(client) ? ZR_CLASS_FLAG_ADMIN_ONLY : 0;

        // Get default mother zombie setting.
        GetConVarString(g_hCvarsList.CVAR_CLASSES_DEFAULT_M_ZOMB, motherzombiesetting, sizeof(motherzombiesetting));

        if (StrEqual(motherzombiesetting, "disabled", false))
        {
            // Do nothing. Keep current class.
        }
        else if (StrEqual(motherzombiesetting, "random", false))
        {
            // Setup filtering
            // ---------------

            // Exclude special class flags.
            filter.ClassFilter_DenyFlags = ZR_CLASS_SPECIALFLAGS;

            // Allow admin classes if admin.
            filter.ClassFilter_DenyFlags -= isadmin;

            // Specify client for checking group permissions.
            filter.ClassFilter_Client = client;

            // Get class
            // ---------

            // Get random regular zombie class. Remove admin flag if admin.
            motherindex = ClassGetRandomClass(ZR_CLASS_TEAM_ZOMBIES, filter);

            // Validate index. Do not change class if it's invalid.
            if (ClassValidateIndex(motherindex))
            {
                // Save active class index to be restored next spawn.
                ClassSelectedNext[client][ZR_CLASS_TEAM_ZOMBIES] = classindex;

                // Change class.
                classindex = motherindex;
            }
        }
        else if (StrEqual(motherzombiesetting, "motherzombies", false))
        {
            // Setup filtering
            // ---------------

            // Exclude special class flags except mother zombies.
            filter.ClassFilter_DenyFlags = ZR_CLASS_SPECIALFLAGS - ZR_CLASS_FLAG_MOTHER_ZOMBIE;

            // Require mother zombie class flag.
            filter.ClassFilter_RequireFlags = ZR_CLASS_FLAG_MOTHER_ZOMBIE;

            // Allow admin classes if admin.
            filter.ClassFilter_DenyFlags -= isadmin;

            // Specify client for checking group permissions.
            filter.ClassFilter_Client = client;

            // Get class
            // ---------

            // Get random mother zombie class. Include admin classes if admin.
            motherindex = ClassGetRandomClass(ZR_CLASS_TEAM_ZOMBIES, filter);

            // Validate index. Do not change class if it's invalid.
            if (ClassValidateIndex(motherindex))
            {
                // This is a mother zombie class. Reset mother zombie setting
                // so class skills aren't improved.
                motherzombie = false;

                // Save active class index to be restored next spawn.
                ClassSelectedNext[client][ZR_CLASS_TEAM_ZOMBIES] = classindex;

                // Change class.
                classindex = motherindex;
            }
        }
        else
        {
            // Assume it's a class name. Get index for the specified class name.
            motherindex = ClassGetIndexByIdentifier(motherzombiesetting);

            // Validate index.
            if (ClassValidateIndex(motherindex))
            {
                // Save active class index to be restored next spawn.
                ClassSelectedNext[client][ZR_CLASS_TEAM_ZOMBIES] = classindex;

                // Change class.
                classindex = motherindex;
            }
        }
    }

    // Update the player's selected class index.
    ClassSelected[client][ZR_CLASS_TEAM_ZOMBIES] = classindex;

    // Restore next indexes, if available. But don't restore the zombie index.
    ClassRestoreNextIndexes(client, ZR_CLASS_TEAM_ZOMBIES);

    // Update the player's cache with zombie attributes.
    ClassReloadPlayerCache(client, classindex);

    // Apply the new attributes.
    ClassApplyAttributes(client, motherzombie);
}

/**
 * Timer callback for disabling instant class change setting on a client.
 */
public Action:Event_ClassDisableInstantSpawn(Handle:timer, any:client)
{
    // Disable instant class change.
    ClassAllowInstantChange[client] = false;
}
