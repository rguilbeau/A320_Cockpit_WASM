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
/// Nom du data pour l'envoi d'évenement
/// </summary>
const char* DATA_NAME_SEND_EVENT = "A320_Cockpit.SEND_EVENT";

/// <summary>
/// Nom du data pour le retour de la valeur d'une LVAR 
/// </summary>
const char* DATA_NAME_LVAR_VALUE = "A320_Cockpit.LVAR_VALUE";
/// <summary>
/// Nom du data pour le retour d'erreur
/// </summary>
const char* DATA_NAME_ERROR = "A320_Cockpit.ERROR";

/// <summary>
/// ID du data pour la demande de lecture d'une LVar
/// </summary>
const SIMCONNECT_CLIENT_DATA_ID DATA_ID_READ_LVAR = 0;
/// <summary>
/// ID du data pour l'envoi d'evenement
/// </summary>
const SIMCONNECT_CLIENT_DATA_ID DATA_ID_SEND_EVENT = 3;
/// <summary>
/// ID du data pour la réponse de la valeur d'une LVar
/// </summary>
const SIMCONNECT_CLIENT_DATA_ID DATA_ID_VALUE_LVAR = 1;
/// <summary>
/// ID du data pour le retour d'erreur
/// </summary>
const SIMCONNECT_CLIENT_DATA_ID DATA_ID_ERROR = 2;

/// <summary>
/// ID de la définition de la lecture d'une LVar
/// </summary>
const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DEFINITION_ID_READ_LVAR = 0;
/// <summary>
/// ID de la définition de l'envoi d'evenement
/// </summary>
const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DEFINITION_ID_SEND_EVENT = 3;
/// <summary>
/// ID de la définition de la réponse de la valeur d'une LVar
/// </summary>
const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DEFINITION_VALUE_LVAR = 1;
/// <summary>
/// ID de la définition pour le retour d'erreur
/// </summary>
const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DEFINITION_ERROR = 2;

/// <summary>
/// Les requets ID
/// </summary>
enum RequestID
{
	READ_LVAR,
	VALUE_LVAR,
	ERROR,
	SEND_EVENT
};

/// <summary>
/// Le groupe WASM
/// </summary>
enum WASM_GROUP
{
	GROUP
};

/// <summary>
/// La structure de la valeur de la réponse d'une 
/// LVar pour le retour vers le client
/// </summary>
struct ResponseLvar {
	FLOAT64 value;
};

/// <summary>
/// La structure du code d'erreur 
/// pour le retour vers le client
/// </summary>
struct ResponseError {
	INT code_error;
};

/// <summary>
/// Liste des erreurs retournés au client par le module WASM
/// </summary>
enum ErrorCode {
	LVAR_NOT_FOUND = 1
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
		data_set
	);

	if (result != S_OK)
	{
		fprintf(stderr, "%s: SimConnect_SetClientData failed\n", MODULE_NAME);
		return false;
	}

	return true;
}

/// <summary>
/// Lecture du LVar
/// </summary>
/// <param name="varname"></param>
/// <returns></returns>
void read_lvar(PCSTRINGZ varname)
{
	ID id_var = check_named_variable(varname);

	if (id_var == -1)
	{
		fprintf(stderr, "%s: Variable \"%s\" not found", MODULE_NAME, varname);
		ResponseError response;
		response.code_error = (INT)ErrorCode::LVAR_NOT_FOUND;
		send_to_client(DATA_ID_ERROR, DEFINITION_ERROR, sizeof(response), &response);
	}
	else
	{
		ResponseLvar response;
		response.value = get_named_variable_value(id_var);
		send_to_client(DATA_ID_VALUE_LVAR, DEFINITION_VALUE_LVAR, sizeof(response), &response);
	}
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
			read_lvar((char*)&recv_data->dwData);
		}
		else if (recv_data->dwRequestID == RequestID::SEND_EVENT)
		{
			fprintf(stderr, "%s: Event= %s", MODULE_NAME, (PCSTRINGZ)&recv_data->dwData);
			execute_calculator_code((PCSTRINGZ)&recv_data->dwData, nullptr, nullptr, nullptr);
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
void init_client_data_area(const char* name, SIMCONNECT_CLIENT_DATA_ID data_id, SIMCONNECT_CLIENT_DATA_DEFINITION_ID definition, DWORD size)
{
	HRESULT result;

	result = SimConnect_MapClientDataNameToID(simConnect, name, data_id);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_MapClientDataNameToID failed (%s:%u)\n", MODULE_NAME, name, data_id);
	}

	// Création des clients data pour l'envoi de commande
	result = SimConnect_CreateClientData(simConnect, data_id, size, SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_CreateClientData failed (%s:%u)\n", MODULE_NAME, name, data_id);
	}

	// Ajout des clients data
	result = SimConnect_AddToClientDataDefinition(simConnect, definition, 0, size);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_AddToClientDataDefinition failed (%s:%u)\n", MODULE_NAME, name, data_id);
	}
}

/// <summary>
/// Initialisation du module WASM, connexion à sim connect, création des clients data area et mise
/// en écoute des requêtes client
/// </summary>
/// <param name=""></param>
/// <returns></returns>
extern "C" MSFS_CALLBACK void module_init(void)
{
	init_simconnect();
	init_client_data_area(DATA_NAME_READ_LVAR, DATA_ID_READ_LVAR, DEFINITION_ID_READ_LVAR, SIMCONNECT_CLIENTDATA_MAX_SIZE);
	init_client_data_area(DATA_NAME_LVAR_VALUE, DATA_ID_VALUE_LVAR, DEFINITION_VALUE_LVAR, sizeof(ResponseLvar));
	init_client_data_area(DATA_NAME_ERROR, DATA_ID_ERROR, DEFINITION_ERROR, sizeof(ResponseError));
	init_client_data_area(DATA_NAME_SEND_EVENT, DATA_ID_SEND_EVENT, DEFINITION_ID_SEND_EVENT, SIMCONNECT_CLIENTDATA_MAX_SIZE);

	// Ecoute des commandes du client
	fprintf(stderr, "%s: Waiting client commands\n", MODULE_NAME);

	listen_client_requests(DATA_ID_READ_LVAR, READ_LVAR, DEFINITION_ID_READ_LVAR);
	listen_client_requests(DATA_ID_SEND_EVENT, SEND_EVENT, DEFINITION_ID_SEND_EVENT);
}

/// <summary>
/// Déinitialisation du module WASM, déconnexion à sim connect
/// </summary>
/// <param name=""></param>
/// <returns></returns>
extern "C" MSFS_CALLBACK void module_deinit(void)
{
	fprintf(stderr, "%s: De-initializing", MODULE_NAME);

	if (!simConnect)
	{
		fprintf(stderr, "%s: SimConnect handle was not valid.\n", MODULE_NAME);
		return;
	}

	HRESULT hr = SimConnect_Close(simConnect);
	if (hr != S_OK)
	{
		fprintf(stderr, "%s: SimConnect_Close failed.\n", MODULE_NAME);
		return;
	}

	fprintf(stderr, "%s: De-initialization completed", MODULE_NAME);
}