#pragma once

#include <MSFS/MSFS_WindowsTypes.h>
#include <MSFS/Legacy/gauges.h>
#include <SimConnect.h>

/// <summary>
/// Structure d'un éspace d'échange SimConnect
/// </summary>
struct s_dataArea
{
	const char* name;
	SIMCONNECT_CLIENT_DATA_ID data_id;
	SIMCONNECT_CLIENT_DATA_DEFINITION_ID definition_id;
	SIMCONNECT_DATA_REQUEST_ID request_id;
	DWORD size;
};

/// <summary>
/// Structure de la donnée de l'éspace d'échange pour la souscription des lvar "A320_Cockpit.SUBSCRIBE_LVAR"
/// </summary>
struct s_subsrcibeLvar
{
	int externalId;
	char name[100];
};

/// <summary>
/// Structure de la donnée de l'éspace d'échange pour les valeurs des lvar "A320_Cockpit.DATA_LVAR"
/// </summary>
struct s_dataLvar
{
	int externalId;
	FLOAT64 value;
};