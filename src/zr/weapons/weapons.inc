/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          weapons.inc
 *  Type:          Core
 *  Description:   API for all weapon-related functions.
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
 * Maximum length of a weapon name string
 */
#define WEAPONS_MAX_LENGTH 32

/**
 * Number of REAL weapon slots (For CS:S)
 */
#define WEAPONS_SLOTS_MAX 5

/**
 * @section start weapons.
 */
#define WEAPONS_SPAWN_CSS_T_WEAPON "weapon_glock"
#define WEAPONS_SPAWN_CSS_CT_WEAPON "weapon_usp"
#define WEAPONS_SPAWN_CSGO_T_WEAPON "weapon_glock"
#define WEAPONS_SPAWN_CSGO_CT_WEAPON "weapon_hkp2000"
/**
 * @endsection
 */

/**
 * Weapon config data indexes.
 */
enum WeaponsData
{
    WEAPONS_DATA_NAME = 0,
    WEAPONS_DATA_ENTITY,
    WEAPONS_DATA_TYPE,
    WEAPONS_DATA_SLOT,
    WEAPONS_DATA_RESTRICTDEFAULT,
    WEAPONS_DATA_TOGGLEABLE,
    WEAPONS_DATA_AMMOTYPE,
    WEAPONS_DATA_AMMOPRICE,
    WEAPONS_DATA_KNOCKBACK,
    WEAPONS_DATA_ZMARKETNAME,
    WEAPONS_DATA_ZMARKETPRICE,
    WEAPONS_DATA_ZMARKETPURCHASEMAX,
    WEAPONS_DATA_ZMARKETCOMMAND,
    WEAPONS_DATA_RESTRICTED,
}

/**
 * @endsection
 */

/**
 * Variable to store active weapon offset value.
 */
new g_iToolsActiveWeapon;

/**
 * Variable to store buyzone offset value.
 */
new g_iToolsInBuyZone;

/**
 * Weapon slots.
 */
enum WeaponsSlot
{
    Slot_Invalid        = -1,   /** Invalid weapon (slot). */
    Slot_Primary        = 0,    /** Primary weapon slot. */
    Slot_Secondary      = 1,    /** Secondary weapon slot. */
    Slot_Melee          = 2,    /** Melee (knife) weapon slot. */
    Slot_Projectile     = 3,    /** Projectile (grenades, flashbangs, etc) weapon slot. */
    Slot_Explosive      = 4,    /** Explosive (c4) weapon slot. */
    Slot_NVGs           = 5,    /** NVGs (fake) equipment slot. */
    Slot_DangerZone     = 11,   /** Dangerzone equipment slot. (CSGO only) */
    Slot_MAXSIZE
}

/**
 * Array handle to store weapon config data.
 */
new Handle:arrayWeapons = INVALID_HANDLE;

#include "restrict"
#include "weaponammo"
#include "zmarket"
#include "menu_weapons"

/**
 * Weapons module init function.
 */
WeaponsInit()
{
    // Forward event to sub-modules.
    RestrictInit();
}

/**
 * All plugins have finished loading.
 */
WeaponsOnAllPluginsLoaded()
{
    // Forward event to sub-modules.
    WeaponAmmoOnAllPluginsLoaded();
}

/**
 * Find active weapon-specific offsets here.
 */
WeaponsOnOffsetsFound()
{
    // If offset "m_hActiveWeapon" can't be found, then stop the plugin.
    g_iToolsActiveWeapon = FindSendPropInfo("CBasePlayer", "m_hActiveWeapon");
    if (g_iToolsActiveWeapon == -1)
    {
        LogEvent(false, LogType_Fatal, LOG_CORE_EVENTS, LogModule_Weapons, "Offsets", "Offset \"CBasePlayer::m_hActiveWeapon\" was not found.");
    }

    // If offset "m_bInBuyZone" can't be found, then stop the plugin.
    g_iToolsInBuyZone = FindSendPropInfo("CCSPlayer", "m_bInBuyZone");
    if (g_iToolsInBuyZone == -1)
    {
        LogEvent(false, LogType_Fatal, LOG_CORE_EVENTS, LogModule_Weapons, "Offsets", "Offset \"CCSPlayer::m_bInBuyZone\" was not found.");
    }

    // Forward event to sub-modules
    WeaponAmmoOnOffsetsFound();
}

/**
 * Create commands related to weapons here.
 */
WeaponsOnCommandsCreate()
{
    // Forward event to sub-modules.
    RestrictOnCommandsCreate();
    ZMarketOnCommandsCreate();
}

/**
 * Create weapon-related cookies here.
 */
WeaponsOnCookiesCreate()
{
    // Forward event to sub-modules.
    ZMarketOnCookiesCreate();
}

/**
 * Loads weapon data from file.
 */
WeaponsLoad()
{
    // Register config file.
    ConfigRegisterConfig(File_Weapons, Structure_Keyvalue, CONFIG_FILE_ALIAS_WEAPONS);

    // If module is disabled, then stop.
    new bool:weapons = GetConVarBool(g_hCvarsList.CVAR_WEAPONS);
    if (!weapons)
    {
        return;
    }

    // Get weapons config path.
    decl String:pathweapons[PLATFORM_MAX_PATH];
    new bool:exists = ConfigGetCvarFilePath(g_hCvarsList.CVAR_CONFIG_PATH_WEAPONS, pathweapons);

    // If file doesn't exist, then log and stop.
    if (!exists)
    {
        // Log failure.
        LogEvent(false, LogType_Error, LOG_CORE_EVENTS, LogModule_Weapons, "Config Validation", "Missing weapons config file: %s", pathweapons);

        return;
    }

    // Set the path to the config file.
    ConfigSetConfigPath(File_Weapons, pathweapons);

    // Load config from file and create array structure.
    new bool:success = ConfigLoadConfig(File_Weapons, arrayWeapons);

    // Unexpected error, stop plugin.
    if (!success)
    {
        LogEvent(false, LogType_Fatal, LOG_CORE_EVENTS, LogModule_Weapons, "Config Validation", "Unexpected error encountered loading: %s", pathweapons);
    }

    // Validate weapons config.
    new size = GetArraySize(arrayWeapons);
    if (!size)
    {
        LogEvent(false, LogType_Error, LOG_CORE_EVENTS, LogModule_Weapons, "Config Validation", "No usable data found in weapons config file: %s", pathweapons);
    }

    // Now copy data to array structure.
    WeaponsCacheData();

    // Set config data.
    ConfigSetConfigLoaded(File_Weapons, true);
    ConfigSetConfigReloadFunc(File_Weapons, GetFunctionByName(GetMyHandle(), "WeaponsOnConfigReload"));
    ConfigSetConfigHandle(File_Weapons, arrayWeapons);

    // Forward event to sub-modules
    RestrictLoad();
    ZMarketLoad();
}

/**
 * Caches weapon data from file into arrays.
 * Make sure the file is loaded before (ConfigLoadConfig) to prep array structure.
 */
WeaponsCacheData()
{
    // Get config's file path.
    decl String:pathweapons[PLATFORM_MAX_PATH];
    ConfigGetConfigPath(File_Weapons, pathweapons, sizeof(pathweapons));

    new Handle:kvWeapons;
    new bool:success = ConfigOpenConfigFile(File_Weapons, kvWeapons);

    if (!success)
    {
        LogEvent(false, LogType_Fatal, LOG_CORE_EVENTS, LogModule_Weapons, "Config Validation", "Unexpected error caching data from weapons config file: %s", pathweapons);
    }

    decl String:weaponname[WEAPONS_MAX_LENGTH];

    // x = array index
    new size = GetArraySize(arrayWeapons);
    for (new x = 0; x < size; x++)
    {
        WeaponsGetName(x, weaponname, sizeof(weaponname));
        KvRewind(kvWeapons);
        if (!KvJumpToKey(kvWeapons, weaponname))
        {
            LogEvent(false, LogType_Error, LOG_CORE_EVENTS, LogModule_Weapons, "Config Validation", "Couldn't cache weapon data for: %s (check weapons config)", weaponname);
            continue;
        }

        // Get config data.

        decl String:weaponentity[CONFIG_MAX_LENGTH];
        decl String:weapontype[CONFIG_MAX_LENGTH];
        decl String:ammotype[CONFIG_MAX_LENGTH];
        decl String:zmarketname[CONFIG_MAX_LENGTH];
        decl String:zmarketcommand[CONFIG_MAX_LENGTH];

        // General
        KvGetString(kvWeapons, "weaponentity", weaponentity, sizeof(weaponentity));
        KvGetString(kvWeapons, "weapontype", weapontype, sizeof(weapontype));
        new WeaponsSlot:weaponslot = WeaponsSlot:KvGetNum(kvWeapons, "weaponslot", -1);

        // Restrict (core)
        new bool:restrictdefault = ConfigKvGetStringBool(kvWeapons, "restrictdefault", "no");
        new bool:toggleable = ConfigKvGetStringBool(kvWeapons, "toggleable", "yes");

        // Weapon Ammo (core)
        KvGetString(kvWeapons, "ammotype", ammotype, sizeof(ammotype));
        new ammoprice = KvGetNum(kvWeapons, "ammoprice", -1);

        // Knockback (module)
        new Float:knockback = KvGetFloat(kvWeapons, "knockback", 1.0);

        // ZMarket (module)
        KvGetString(kvWeapons, "zmarketname", zmarketname, sizeof(zmarketname), "");
        new zmarketprice = KvGetNum(kvWeapons, "zmarketprice", -1);
        new zmarketpurchasemax = KvGetNum(kvWeapons, "zmarketpurchasemax", -1);
        KvGetString(kvWeapons, "zmarketcommand", zmarketcommand, sizeof(zmarketcommand), "");

        new Handle:arrayWeapon = GetArrayCell(arrayWeapons, x);

        // Push data into array.
        PushArrayString(arrayWeapon, weaponentity);         // Index: 1
        PushArrayString(arrayWeapon, weapontype);           // Index: 2
        PushArrayCell(arrayWeapon, weaponslot);             // Index: 3
        PushArrayCell(arrayWeapon, restrictdefault);        // Index: 4
        PushArrayCell(arrayWeapon, toggleable);             // Index: 5
        PushArrayString(arrayWeapon, ammotype);             // Index: 6
        PushArrayCell(arrayWeapon, ammoprice);              // Index: 7
        PushArrayCell(arrayWeapon, knockback);              // Index: 8
        PushArrayString(arrayWeapon, zmarketname);          // Index: 9
        PushArrayCell(arrayWeapon, zmarketprice);           // Index: 10
        PushArrayCell(arrayWeapon, zmarketpurchasemax);     // Index: 11
        PushArrayString(arrayWeapon, zmarketcommand);       // Index: 12

        // Initialize other stored weapon info here.
        PushArrayCell(arrayWeapon, restrictdefault);        // Index: 13
    }

    // We're done with this file now, so we can close it.
    CloseHandle(kvWeapons);
}

/**
 * Called when config is being reloaded.
 */
public WeaponsOnConfigReload()
{
    // Reload weapons config.
    WeaponsLoad();
}

/**
 * Client is joining the server.
 *
 * @param client    The client index.
 */
WeaponsClientInit(client)
{
    // Forward event to sub-modules.
    RestrictClientInit(client);
    ZMarketClientInit(client);
}

/**
 * Called once a client's saved cookies have been loaded from the database.
 *
 * @param client    The client index.
 */
WeaponsOnCookiesCached(client)
{
    // Forward event to sub-modules.
    ZMarketOnCookiesCached(client);
}

/**
 * Client is leaving the server.
 *
 * @param client    The client index.
 */
WeaponsOnClientDisconnect(client)
{
    // Forward event to sub-modules.
    RestrictOnClientDisconnect(client);
    ZMarketOnClientDisconnect(client);
}

/**
 * Client is spawning into the game.
 *
 * @param client    The client index.
 */
WeaponsOnClientSpawn(client)
{
    // Forward event to sub-modules.
    RestrictOnClientSpawn(client);
}

/**
 * Client is spawning into the game. *Post
 *
 * @param client    The client index.
 */
WeaponsOnClientSpawnPost(client)
{
    // Refresh all weapons on clients to cleanup any rendering errors.
    WeaponsRefreshAllClientWeapons(client);

    // Forward event to sub-modules.
    ZMarketOnClientSpawnPost(client);
}

/**
 * The round is ending.
 */
WeaponsOnRoundEnd()
{
    // Forward event to sub-modules.
    RestrictOnRoundEnd();
}

/**
 * Weapon data reading API.
 */

/**
 * Clear cache for a given weapon.
 *
 * @param index     The weapon index.
 */
stock WeaponsClearCache(index)
{
    // Get array handle of weapon at given index.
    new Handle:hWeapon = GetArrayCell(arrayWeapons, index);

    // Clear array.
    ClearArray(hWeapon);
}

/**
 * Find the index at which the weapon's name is at.
 *
 * @param weapon    The weapon name.
 * @return          The array index containing the given weapon name.
 */
stock WeaponsNameToIndex(const String:weapon[])
{
    decl String:weaponname[WEAPONS_MAX_LENGTH];

    // x = Array index.
    new size = GetArraySize(arrayWeapons);
    for (new x = 0; x < size; x++)
    {
        WeaponsGetName(x, weaponname, sizeof(weaponname));

        // If names match, then return index.
        if (StrEqual(weapon, weaponname, false))
        {
            return x;
        }
    }

    // Name doesn't exist.
    return -1;
}

/**
 * Takes a weapon's entity name and returns the display name in weapons config file.
 *
 * @param entityname    The entity to find the display of.
 * @param display       Buffer to store display name in.
 * @param displaymaxlen The max length of the display name.
 * @param noprefix      If this is true, the entity name will be compared without the weapon_/item_ prefix.
 * @return              The index of the weapon found.
 */
stock WeaponsEntityToDisplay(const String:entityname[], String:display[], displaymaxlen, noprefix = false)
{
    // Check if weapon data is loaded.
    if (arrayWeapons == INVALID_HANDLE)
    {
        return -1;
    }

    decl String:weaponentity[WEAPONS_MAX_LENGTH];

    // Initialize string to null.
    strcopy(display, sizeof(displaymaxlen), "");

    // x = Array index.
    new size = GetArraySize(arrayWeapons);
    for (new x = 0; x < size; x++)
    {
        // If entity names don't match, then stop.
        WeaponsGetEntity(x, weaponentity, sizeof(weaponentity));

        // If noprefix is true, then strip the weapon_/item_ prefix.
        if (noprefix)
        {
            ReplaceString(weaponentity, sizeof(weaponentity), "weapon_", "");
            ReplaceString(weaponentity, sizeof(weaponentity), "item_", "");
        }

        if (!StrEqual(entityname, weaponentity, false))
        {
            continue;
        }

        // The entity names match, so return display.
        WeaponsGetName(x, display, displaymaxlen, (g_Game == Game_CSGO));

        // Return the weapon index.
        return x;
    }

    // Nothing was found.
    return -1;
}

/**
 * Checks if a weapon is valid. (E.G. listed in weapons.txt)
 * @param weapon    The weapon name.
 * @return          Returns true if valid, false it not.
 */
stock bool:WeaponsIsWeaponValid(const String:weapon[])
{
    return (WeaponsNameToIndex(weapon) != -1);
}

/**
 * Gets the name of a weapon at a given index.
 * @param index     The weapon index.
 * @param weapon    The string to return name in.
 * @param maxlen    The max length of the string.
 */
stock WeaponsGetName(index, String:weapon[], maxlen, bool zmarket = false)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    new String:weaponName[WEAPONS_MAX_LENGTH];
    new String:marketName[WEAPONS_MAX_LENGTH];

    // Get weapon name.
    GetArrayString(arrayWeapon, _:WEAPONS_DATA_NAME, weaponName, maxlen);

    // Get weapon zmarket name.
    if (zmarket)
        GetArrayString(arrayWeapon, _:WEAPONS_DATA_ZMARKETNAME, marketName, maxlen);

    // Are we in zmarket, and is our name valid?
    if (zmarket && strlen(marketName))
        strcopy(weapon, maxlen, marketName);
    else
        strcopy(weapon, maxlen, weaponName);
}

/**
 * Gets the entity of a weapon at a given index.
 * @param index     The weapon index.
 * @param type      The string to return entity in.
 * @param maxlen    The max length of the string.
 */
stock WeaponsGetEntity(index, String:type[], maxlen)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Get weapon type.
    GetArrayString(arrayWeapon, _:WEAPONS_DATA_ENTITY, type, maxlen);
}

/**
 * Gets the type of a weapon at a given index.
 * @param index     The weapon index.
 * @param type      The string to return type in.
 * @param maxlen    The max length of the string.
 */
stock WeaponsGetType(index, String:type[], maxlen)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Get weapon type.
    GetArrayString(arrayWeapon, _:WEAPONS_DATA_TYPE, type, maxlen);
}

/**
 * Gets the slot index of a weapon at a given index.
 * @param index     The weapon index.
 * @return          The slot index of the weapon.
 */
stock WeaponsSlot:WeaponsGetSlot(index)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Return default restriction status.
    return WeaponsSlot:GetArrayCell(arrayWeapon, _:WEAPONS_DATA_SLOT);
}

/**
 * Gets if a weapon is restricted by default.
 * @param index     The weapon index.
 * @return          True if the weapon is restricted by default, false if not.
 */
stock bool:WeaponsGetRestrictDefault(index)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Return default restriction status.
    return bool:GetArrayCell(arrayWeapon, _:WEAPONS_DATA_RESTRICTDEFAULT);
}

/**
 * Gets if a weapon's restriction status is toggleable.
 * @param index     The weapon index.
 * @return          True if the weapon restriction can be toggled, false if not.
 */
stock bool:WeaponsGetToggleable(index)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Return if weapon is toggleable.
    return bool:GetArrayCell(arrayWeapon, _:WEAPONS_DATA_TOGGLEABLE);
}

/**
 * Gets the ammo type of a weapon at a given index.
 * @param index     The weapon index.
 * @param ammotype  The string to return ammotype in.
 * @param maxlen    The max length of the string.
 */
stock WeaponsGetAmmoType(index, String:ammotype[], maxlen)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Get ammo type of the weapon.
    GetArrayString(arrayWeapon, _:WEAPONS_DATA_AMMOTYPE, ammotype, maxlen);
}

/**
 * Gets the price of ammo for the weapon.
 * @param index     The weapon index.
 * @return          The ammo price.
 */
stock WeaponsGetAmmoPrice(index)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Return ammo price of the weapon.
    return GetArrayCell(arrayWeapon, _:WEAPONS_DATA_AMMOPRICE);
}

/**
 * Gets the knockback multiplier for the weapon.
 * @param index     The weapon index.
 * @return          The weapon knockback multiplier.
 */
stock Float:WeaponsGetKnockback(index)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Return knockback multiplier of the weapon.
    return Float:GetArrayCell(arrayWeapon, _:WEAPONS_DATA_KNOCKBACK);
}

/**
 * Gets the ZMarket name for the weapon.
 * @param index     The weapon index.
 * @param weapon    The string to return name in.
 * @param maxlen    The max length of the string.
 */
stock WeaponsGetZMarketName(index, String:weapon[], maxlen)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Get the ZMarket name of the weapon.
    GetArrayString(arrayWeapon, _:WEAPONS_DATA_ZMARKETNAME, weapon, maxlen);
}

/**
 * Gets the ZMarket price for the weapon.
 * @param index     The weapon index.
 * @return          The ZMarket price.
 */
stock WeaponsGetZMarketPrice(index)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Return the ZMarket price of the weapon.
    return GetArrayCell(arrayWeapon, _:WEAPONS_DATA_ZMARKETPRICE);
}

/**
 * Gets the max purchases from ZMarket per round per client of a weapon.
 * @param index     The weapon index.
 * @return          The max purchases of the weapon.
 */
stock WeaponsGetZMarketPurchaseMax(index)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Return the ZMarket price of the weapon.
    return GetArrayCell(arrayWeapon, _:WEAPONS_DATA_ZMARKETPURCHASEMAX);
}

/**
 * Gets the name of the command to purchase this weapon through zmarket.
 * @param index     The weapon index.
 * @param command   The string to return zmarketcommand in.
 * @param maxlen    The max length of the string.
 * @return          The max purchases of the weapon.
 */
stock WeaponsGetZMarketCommand(index, String:command[], maxlen)
{
    // Get array handle of weapon at given index.
    new Handle:arrayWeapon = GetArrayCell(arrayWeapons, index);

    // Return the ZMarket price of the weapon.
    GetArrayString(arrayWeapon, _:WEAPONS_DATA_ZMARKETCOMMAND, command, maxlen);
}

/**
 * General weapon API.
 */

/**
 * Checks if a client has a specific weapon.
 *
 * @param client    The client index.
 * @param weapon    The weapon classname.
 */
stock bool:WeaponsClientHasWeapon(client, const String:weapon[])
{
    // Get all of client's current weapons.
    new weapons[Slot_MAXSIZE];
    WeaponsGetClientWeapons(client, weapons);

    decl String:classname[64];

    // x = slot index
    for (new x = 0; x < WEAPONS_SLOTS_MAX; x++)
    {
        // If slot is empty, then stop.
        if (weapons[x] == -1)
        {
            continue;
        }

        // If the weapon's classname matches, then return true.
        GetEdictClassname(weapons[x], classname, sizeof(classname));
        if (StrEqual(weapon, classname, false))
        {
            return true;
        }
    }

    return false;
}

/**
 * Return an array that contains all client's weapon indexes.
 *
 * @param client    The client index.
 * @param weapons   The weapon index array.
 *                  -1 if no weapon in slot.
 */
stock void WeaponsGetClientWeapons(int client, int weapons[Slot_MAXSIZE])
{
    // x = Weapon slot.
    for (new x = 0; x < WEAPONS_SLOTS_MAX; x++)
    {
        weapons[x] = GetPlayerWeaponSlot(client, x);
    }
}

/**
 * Returns weapon index of the client's active weapon.
 *
 * @param client    The client index.
 * @return          The weapon index of the active weapon.
 *                  -1 if no weapon is active.
 */
stock WeaponsGetActiveWeaponIndex(client)
{
    // Return the client's active weapon.
    return GetEntDataEnt2(client, g_iToolsActiveWeapon);
}

/**
 * Returns slot of client's active weapon.
 *
 * @param client    The client index.
 * @return          The slot number of active weapon.
 */
stock WeaponsSlot:WeaponsGetActiveWeaponSlot(client)
{
    // Get all client's weapon indexes.
    new weapons[Slot_MAXSIZE];
    WeaponsGetClientWeapons(client, weapons);

    // Get client's active weapon.
    new activeweapon = WeaponsGetActiveWeaponIndex(client);

    // If client has no active weapon, then stop.
    if (activeweapon == -1)
    {
        return Type_Invalid;
    }

    // x = weapon slot.
    for (new x = 0; x < WEAPONS_SLOTS_MAX; x++)
    {
        if (weapons[x] == activeweapon)
        {
            return WeaponsSlot:x;
        }
    }

    return Type_Invalid;
}

/**
 * Forces player to drop weapon index. (Simplified wrapper)
 *
 * @param client    The client index.
 * @param weapon    The weapon index to force client to drop.
 */
stock WeaponsForceClientDrop(client, weapon)
{
    // Force client to drop weapon.
    CS_DropWeapon(client, weapon, true, false);
}

/**
 * Used to explicitly remove projectile weapons from a client.
 *
 * @param client    The client index.
 * @param weaponsdrop   True to force drop on all weapons, false to strip.
 */
stock WeaponsClearClientWeaponSlot(client, WeaponsSlot:slot, bool:weaponsdrop)
{
    // This while-structure is a stupid sloppy annoying workaround to stop the "unintended assignment" warning. I hate those.
    // While GetPlayerWeaponSlot returns a valid weapon, remove it and then test again.
    new weapon = GetPlayerWeaponSlot(client, _:slot);
    while (weapon != -1)
    {
        // Check if we drop or strip the weapon.
        if (weaponsdrop)
        {
            WeaponsForceClientDrop(client, weapon);
        }
        else
        {
            RemovePlayerItem(client, weapon);
            AcceptEntityInput(weapon, "Kill");
        }

        // Find next weapon.
        weapon = GetPlayerWeaponSlot(client, _:slot);
    }
}

/**
 * Refresh a weapon by taking it and giving it back.
 *
 * @param client        The client index.
 * @param slot          The weapon slot to refresh. (see enum WeaponsSlot)
 */
stock WeaponsRefreshClientWeapon(client, WeaponsSlot:slot)
{
    new weaponindex = GetPlayerWeaponSlot(client, _:slot);

    // If weapon is invalid, then stop.
    if (weaponindex == -1)
    {
        return;
    }

    // Get the classname of the weapon to re-give.
    decl String:entityname[WEAPONS_MAX_LENGTH];
    GetEdictClassname(weaponindex, entityname, sizeof(entityname));

    // Refresh weapon.
    RemovePlayerItem(client, weaponindex);
    AcceptEntityInput(weaponindex, "Kill");
    GivePlayerItem(client, entityname);
}

/**
 * Remove all weapons, except knife, on a client, with options.
 *
 * @param client        The client index.
 * @param weaponsdrop   True to force drop on all weapons, false to strip.
 */
stock WeaponsRemoveAllClientWeapons(client, bool:weaponsdrop)
{
    // Get a list of all client's weapon indexes.
    new weapons[Slot_MAXSIZE];
    WeaponsGetClientWeapons(client, weapons);

    // Loop through array slots and force drop.
    // x = weapon slot.
    for (new x = 0; x < WEAPONS_SLOTS_MAX; x++)
    {
        // If weapon is invalid, then stop.
        if (weapons[x] == -1)
        {
            continue;
        }

        // If this is the knife slot, then strip it and stop.
        if (WeaponsSlot:x == Slot_Melee)
        {
            // Strip knife.
            RemovePlayerItem(client, weapons[x]);
            AcceptEntityInput(weapons[x], "Kill");
            continue;
        }

        if (weaponsdrop)
        {
            // Force client to drop weapon.
            WeaponsForceClientDrop(client, weapons[x]);
        }
        else
        {
            // Strip weapon.
            RemovePlayerItem(client, weapons[x]);
            AcceptEntityInput(weapons[x], "Kill");
        }
    }

    // Remove left-over knifes.
    WeaponsClearClientWeaponSlot(client, Slot_Melee, false);

    // Remove left-over projectiles.
    WeaponsClearClientWeaponSlot(client, Slot_Projectile, weaponsdrop);

    // Remove left-over explosives.
    WeaponsClearClientWeaponSlot(client, Slot_Explosive, weaponsdrop);

    // These only exist on CSGO.
    if (g_Game == Game_CSGO)
    {
        // Remove left-over dangerzone items.
        WeaponsClearClientWeaponSlot(client, Slot_DangerZone, false);
    }

    // Give zombie a new knife. (If you leave the old one there will be glitches with the knife positioning)
    GivePlayerItem(client, "weapon_knife");
}

/**
 * Refresh a weapon by taking it and giving it back.
 *
 * @param client        The client index.
 */
stock WeaponsRefreshAllClientWeapons(client)
{
    // Get a list of all client's weapon indexes.
    new weapons[Slot_MAXSIZE];
    WeaponsGetClientWeapons(client, weapons);

    // Loop through array slots and force drop.
    // x = weapon slot.
    for (new x = 0; x < WEAPONS_SLOTS_MAX; x++)
    {
        // If weapon is invalid, then stop.
        if (weapons[x] == -1)
        {
            continue;
        }

        WeaponsRefreshClientWeapon(client, WeaponsSlot:x);
    }
}

/**
 * Checks if a client is in a buyzone.
 *
 * @param client    The client index.
 */
stock bool:WeaponsIsClientInBuyZone(client)
{
    // Return if client is in buyzone.
    return bool:GetEntData(client, g_iToolsInBuyZone);
}
