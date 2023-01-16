// WASM_HABI.cpp
#include <stdio.h>
#include <MSFS/MSFS.h>
#include <MSFS/MSFS_WindowsTypes.h>
#include <SimConnect.h>
#include <MSFS/Legacy/gauges.h>

#include <cstring>
#include <vector>
#include <chrono>

#include "A320_Cockpit_WASM.h"
#include <iostream>
 
/// <summary>
/// Nom du module
/// </summary>
const char* MODULE_NAME = "A320_Cockpit";

/// <summary>
/// Nom du data pour la demande de lecture d'une LVar
/// </summary>
const char* DATA_NAME_READ_LVAR = "A320_Cockpit.READ_LVAR";
/// <summary>
/// Nom du data pour le retour de la valeur d'une LVAR 
/// </summary>
const char* DATA_NAME_LVAR_VALUE = "A320_Cockpit.LVAR_VALUE";

/// <summary>
/// ID du data pour la demande de lecture d'une LVar
/// </summary>
const SIMCONNECT_CLIENT_DATA_ID DATA_ID_READ_LVAR = 0;
/// <summary>
/// ID du data pour la réponse de la valeur d'une LVar
/// </summary>
const SIMCONNECT_CLIENT_DATA_ID DATA_ID_VALUE_LVAR = 1;

/// <summary>
/// ID de la définition de la lecture d'une LVar
/// </summary>
const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DEFINITION_ID_READ_LVAR = 0;
/// <summary>
/// ID de la définition de la réponse de la valeur d'une LVar
/// </summary>
const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DEFINITION_VALUE_LVAR = 1;

/// <summary>
/// Les requets ID
/// </summary>
enum RequestID
{
	READ_LVAR,
	VALUE_LVAR
};

/// <summary>
/// Le groupe WASM
/// </summary>
enum WASM_GROUP
{
	GROUP
};

/// <summary>
/// La structure de la valeur de la réponse d'une LVar
/// </summary>
struct Result {
	FLOAT64 value;
};

/// <summary>
/// Client SimConnect
/// </summary>
HANDLE simConnect;




/// <summary>
/// Mise en écoute des requetes du client
/// </summary>
/// <param name="data_id"></param>
/// <param name="request_id"></param>
/// <param name="definition"></param>
bool listen_client_requests(SIMCONNECT_CLIENT_DATA_ID data_id, RequestID request_id, SIMCONNECT_CLIENT_DATA_DEFINITION_ID definition)
{
	// Ecoute des commandes du client
	fprintf(stderr, "%s: Listen client requests data_id:%u, request_id:%u, definition:%u\n", MODULE_NAME, data_id, request_id, definition);
	HRESULT result = SimConnect_RequestClientData(simConnect,
		data_id,
		request_id,
		definition,
		SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET,
		SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT
	);

	if (result != S_OK)
	{
		fprintf(stderr, "%s: SimConnect_RequestClientData failed data_id:%u, request_id:%u, definition:%u\n", MODULE_NAME, data_id, request_id, definition);
		return false;
	}

	return true;
}

/// <summary>
/// Lecture du LVar
/// </summary>
/// <param name="varname"></param>
/// <returns></returns>
FLOAT64 read_lvar(PCSTRINGZ varname)
{
	FLOAT64 value = 0;
	ID id_var = check_named_variable(varname);

	if (id_var == -1)
	{
		fprintf(stderr, "%s: Variable \"%s\" not found", MODULE_NAME, varname);
	}
	else
	{
		value = get_named_variable_value(id_var);
	}

	return value;
}

/// <summary>
/// Envoi un message au client
/// </summary>
/// <param name="data_id"></param>
/// <param name="definition"></param>
/// <param name="size"></param>
/// <param name="data_set"></param>
/// <returns></returns>
bool send_to_client(SIMCONNECT_CLIENT_DATA_ID data_id, SIMCONNECT_CLIENT_DATA_DEFINITION_ID definition, DWORD size, void* data_set)
{
	HRESULT result = SimConnect_SetClientData(
		simConnect,
		data_id,
		definition,
		SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT,
		0,
		size,
		&data_set
	);

	if (result != S_OK)
	{
		fprintf(stderr, "%s: SimConnect_SetClientData failed\n", MODULE_NAME);
		return false;
	}

	return true;
}

/// <summary>
/// Callback des récéptions des messages de SimConnect
/// </summary>
/// <param name="pData"></param>
/// <param name="cbData"></param>
/// <param name="pContext"></param>
void CALLBACK on_request_received(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
	if (pData->dwID == SIMCONNECT_RECV_ID_CLIENT_DATA)
	{
		
		SIMCONNECT_RECV_CLIENT_DATA* recv_data = (SIMCONNECT_RECV_CLIENT_DATA*)pData;

		if (recv_data->dwRequestID == RequestID::READ_LVAR)
		{
			Result exeRes;
			exeRes.value = read_lvar((char*)&recv_data->dwData);
			send_to_client(DATA_ID_VALUE_LVAR, DEFINITION_VALUE_LVAR, sizeof(exeRes), &exeRes);
		}
	}
}

/// <summary>
/// Initialisation et connexion à simconnect
/// </summary>
/// <returns></returns>
bool init_simconnect()
{
	HRESULT result;

	// Connexion à SimConnect
	fprintf(stderr, "%s: Initialize module...\n", MODULE_NAME);

	result = SimConnect_Open(&simConnect, MODULE_NAME, (HWND)NULL, 0, 0, 0);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_Open failed\n", MODULE_NAME);
		return false;
	}

	result = SimConnect_SetNotificationGroupPriority(simConnect, WASM_GROUP::GROUP, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_SetNotificationGroupPriority failed.\n", MODULE_NAME);
		return false;
	}

	result = SimConnect_CallDispatch(simConnect, on_request_received, NULL);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_CallDispatch failed.\n", MODULE_NAME);
		return false;
	}

	return true;
}

/// <summary>
/// Create client data area
/// </summary>
/// <param name="name"></param>
/// <param name="data_id"></param>
/// <param name="definition"></param>
/// <returns></returns>
bool init_client_data_area(const char* name, SIMCONNECT_CLIENT_DATA_ID data_id, SIMCONNECT_CLIENT_DATA_DEFINITION_ID definition, DWORD size)
{
	HRESULT result;

	result = SimConnect_MapClientDataNameToID(simConnect, name, data_id);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_MapClientDataNameToID failed (%s:%u)\n", MODULE_NAME, name, data_id);
		return false;
	}

	// Création des clients data pour l'envoi de commande
	result = SimConnect_CreateClientData(simConnect, data_id, size, SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_CreateClientData failed (%s:%u)\n", MODULE_NAME, name, data_id);
		return false;
	}

	// Ajout des clients data
	result = SimConnect_AddToClientDataDefinition(simConnect, definition, 0, size);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_AddToClientDataDefinition failed (%s:%u)\n", MODULE_NAME, name, data_id);
		return false;
	}

	return true;
}

/// <summary>
/// Initialisation du module WASM
/// </summary>
/// <param name=""></param>
/// <returns></returns>
extern "C" MSFS_CALLBACK void module_init(void)
{
	init_simconnect();
	init_client_data_area(DATA_NAME_READ_LVAR, DATA_ID_READ_LVAR, DEFINITION_ID_READ_LVAR, SIMCONNECT_CLIENTDATA_MAX_SIZE);
	init_client_data_area(DATA_NAME_LVAR_VALUE, DATA_ID_VALUE_LVAR, DEFINITION_VALUE_LVAR, sizeof(Result));

	// Ecoute des commandes du client
	fprintf(stderr, "%s: Waiting client commands\n", MODULE_NAME);
	listen_client_requests(DATA_ID_READ_LVAR, READ_LVAR, DEFINITION_ID_READ_LVAR);
}

/// <summary>
/// Déinitialisation du module WASM
/// </summary>
/// <param name=""></param>
/// <returns></returns>
extern "C" MSFS_CALLBACK void module_deinit(void)
{
	fprintf(stderr, "%s: De-initialization completed\n", MODULE_NAME);
}