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
/// Client SimConnect
/// </summary>
HANDLE simConnect;

/// <summary>
/// Structure d'une LVAR souscrite
/// </summary>
struct SubscribeLvar
{
	ID id;
	PCSTRINGZ name;
	FLOAT64 value;
};

/// <summary>
/// 
/// </summary>
struct DataArea
{
	const char* name;
	SIMCONNECT_CLIENT_DATA_ID data_id;
	SIMCONNECT_CLIENT_DATA_DEFINITION_ID definition_id;
	SIMCONNECT_DATA_REQUEST_ID request_id;
	DWORD size;
};

/// <summary>
/// Cannal avec le programme externe pour la souscription d'une LVAR
/// </summary>
DataArea subscribe_lvar_area = {
	"A320_Cockpit.SUBSCRIBE_LVAR",
	(SIMCONNECT_CLIENT_DATA_ID)0,
	(SIMCONNECT_CLIENT_DATA_DEFINITION_ID)0,
	(SIMCONNECT_DATA_REQUEST_ID)0,
	SIMCONNECT_CLIENTDATA_MAX_SIZE
};

/// <summary>
/// Cannal avec le programme externe pour la réponse lors d'un changement de valeur d'une LVAR
/// </summary>
DataArea response_lvar_area = {
	"A320_Cockpit.RESPONSE_LVAR",
	(SIMCONNECT_CLIENT_DATA_ID)1,
	(SIMCONNECT_CLIENT_DATA_DEFINITION_ID)1,
	(SIMCONNECT_DATA_REQUEST_ID)1,
	sizeof(SubscribeLvar)
};

/// <summary>
/// Cannal avec le programme externe pour la récéption d'event (execute_calculator_code) venant du programme externe
/// </summary>
DataArea send_event_area = {
	"A320_Cockpit.SEND_EVENT",
	(SIMCONNECT_CLIENT_DATA_ID)2,
	(SIMCONNECT_CLIENT_DATA_DEFINITION_ID)2,
	(SIMCONNECT_DATA_REQUEST_ID)2,
	SIMCONNECT_CLIENTDATA_MAX_SIZE
};

/// <summary>
/// Liste des LVAR souscrites
/// </summary>
std::vector<SubscribeLvar> subscribed_lvar;

/// <summary>
/// Create client data area
/// </summary>
/// <param name="name"></param>
/// <param name="data_id"></param>
/// <param name="definition"></param>
/// <returns></returns>
void init_client_data_area(DataArea dataAera)
{
	HRESULT result;

	result = SimConnect_MapClientDataNameToID(simConnect, dataAera.name, dataAera.data_id);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_MapClientDataNameToID failed (%s:%u)\n", MODULE_NAME, dataAera.name, dataAera.data_id);
	}

	// Création des clients data pour l'envoi de commande
	result = SimConnect_CreateClientData(simConnect, dataAera.data_id, dataAera.size, SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_CreateClientData failed (%s:%u)\n", MODULE_NAME, dataAera.name, dataAera.data_id);
	}

	// Ajout des clients data
	result = SimConnect_AddToClientDataDefinition(simConnect, dataAera.definition_id, 0, dataAera.size);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_AddToClientDataDefinition failed (%s:%u)\n", MODULE_NAME, dataAera.name, dataAera.data_id);
	}
}

/// <summary>
/// Mise en écoute des requetes du client
/// </summary>
/// <param name="data_id"></param>
/// <param name="request_id"></param>
/// <param name="definition"></param>
bool listen_client_requests(DataArea dataArea)
{
	// Ecoute des commandes du client
	fprintf(
		stderr,
		"%s: Listen client requests data_id:%u, request_id:%u, definition:%u\n",
		MODULE_NAME,
		dataArea.data_id,
		dataArea.request_id,
		dataArea.definition_id
	);

	HRESULT result = SimConnect_RequestClientData(simConnect,
		dataArea.data_id,
		dataArea.request_id,
		dataArea.definition_id,
		SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET,
		SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT
	);

	if (result != S_OK)
	{
		fprintf(
			stderr,
			"%s: SimConnect_RequestClientData failed data_id:%u, request_id:%u, definition:%u\n",
			MODULE_NAME,
			dataArea.data_id,
			dataArea.request_id,
			dataArea.definition_id
		);
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
bool send_to_client(DataArea dataArea, void* data_set)
{
	HRESULT result = SimConnect_SetClientData(simConnect,
		dataArea.data_id,
		dataArea.definition_id,
		SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT,
		0,
		dataArea.size,
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
/// Callback des récéptions des event de SimConnect et du programme externe
/// </summary>
/// <param name="pData"></param>
/// <param name="cbData"></param>
/// <param name="pContext"></param>
void CALLBACK on_request_received(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
	if (pData->dwID == SIMCONNECT_RECV_ID_EVENT_FRAME) {
		// A chaque frame, on lit les variables de l'avion
		for (SubscribeLvar &lvar : subscribed_lvar)
		{
			bool force_send = false;

			// Si la LVAR n'a pas d'ID, elle n'a jamais été lu
			if (lvar.id == -1)
			{
				ID id_var = check_named_variable(lvar.name);
				if (id_var == -1)
				{
					fprintf(stderr, "%s: %s not found", MODULE_NAME, lvar.name);
					continue;
				}
				else 
				{
					force_send = true; // On force l'envoi pour la 1ere lecture
					lvar.id = id_var;
				}
			}

			// Lecture + envoi de la LVAR au programme externe
			FLOAT64 value = get_named_variable_value(lvar.id);
			if (force_send || lvar.value != value) {
				lvar.value = value;
				fprintf(stderr, "%s: %s: %f", MODULE_NAME, lvar.name, lvar.value);
			}
		}
	} 
	else if (pData->dwID == SIMCONNECT_RECV_ID_CLIENT_DATA)
	{
		SIMCONNECT_RECV_CLIENT_DATA* recv_data = (SIMCONNECT_RECV_CLIENT_DATA*)pData;

		// Le programme externe veut sourscrire une LVAR
		if (recv_data->dwRequestID == subscribe_lvar_area.request_id)
		{
			SubscribeLvar lvar = {
				-1,
				(char*)&recv_data->dwData,
				0
			};
			subscribed_lvar.push_back(lvar);
		}

		// Le programme externe lance un event
		else if (recv_data->dwRequestID == send_event_area.request_id)
		{
			fprintf(stderr, "%s: Event= %s", MODULE_NAME, (PCSTRINGZ)&recv_data->dwData);
			execute_calculator_code((PCSTRINGZ)&recv_data->dwData, nullptr, nullptr, nullptr);
		}
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
	HRESULT result;

	// Connexion à SimConnect
	fprintf(stderr, "%s: Initialize module...\n", MODULE_NAME);

	// DEBUG
	SubscribeLvar lvar1 = {-1, "A32NX_AUTOPILOT_SPEED_SELECTED", 0};
	subscribed_lvar.push_back(lvar1);

	SubscribeLvar lvar2 = {-1, "A32NX_FCU_SPD_MANAGED_DOT", 0 };
	subscribed_lvar.push_back(lvar2);

	SubscribeLvar lvar3 = {-1, "A32NX_TRK_FPA_MODE_ACTIVE", 0 };
	subscribed_lvar.push_back(lvar3);
	// END DEBUG

	result = SimConnect_Open(&simConnect, MODULE_NAME, (HWND)NULL, 0, 0, 0);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_Open failed\n", MODULE_NAME);
		return;
	}

	result = SimConnect_SetNotificationGroupPriority(simConnect, (SIMCONNECT_NOTIFICATION_GROUP_ID)0, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_SetNotificationGroupPriority failed.\n", MODULE_NAME);
		return;
	}

	result = SimConnect_CallDispatch(simConnect, on_request_received, NULL);
	if (result != S_OK) {
		fprintf(stderr, "%s: SimConnect_CallDispatch failed.\n", MODULE_NAME);
		return;
	}

	// Initialisation des canneaux de communication avec le programme externe
	init_client_data_area(subscribe_lvar_area);
	init_client_data_area(response_lvar_area);
	init_client_data_area(send_event_area);

	// Ecoute des demandes venant du programme externe
	listen_client_requests(subscribe_lvar_area);
	listen_client_requests(send_event_area);

	// Souscrition de l'event "on each frame" de MSFS pour lire toutes les variables de l'avion
	SimConnect_SubscribeToSystemEvent(simConnect, (SIMCONNECT_CLIENT_EVENT_ID)10, "Frame");
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