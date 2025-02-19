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

#include "SimconnectClient.h"

/// <summary>
/// Nom du module
/// </summary>
const char* MODULE_NAME = "[A320_Cockpit]";

SimconnectClient client(MODULE_NAME);

/// <summary>
/// Structure d'une LVAR
/// </summary>
struct Lvar
{
	int external_id;
	ID msfs_id;
	PCSTRINGZ name;
	FLOAT64 value;
};

/// <summary>
/// Cannal avec le programme externe pour la souscription d'une LVAR
/// </summary>
SimconnectClient::s_dataArea subscribeLvarArea =
{
	"A320_Cockpit.SUBSCRIBE_LVAR",
	(SIMCONNECT_CLIENT_DATA_ID)0,
	(SIMCONNECT_CLIENT_DATA_DEFINITION_ID)0,
	(SIMCONNECT_DATA_REQUEST_ID)0,
	sizeof(char) * 10
};


/// <summary>
/// Liste des LVAR souscrites
/// </summary>
std::vector<Lvar> lvars;


/// <summary>
/// Callback des récéptions des event de SimConnect et du programme externe
/// 
/// </summary>
/// <param name="pData"></param>
/// <param name="cbData"></param>
/// <param name="pContext"></param>
void CALLBACK on_request_received(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
	fprintf(stderr, "%s: RECEIVED", MODULE_NAME);

	if (pData->dwID == SIMCONNECT_RECV_ID_CLIENT_DATA)
	{
		SIMCONNECT_RECV_CLIENT_DATA* recv_data = (SIMCONNECT_RECV_CLIENT_DATA*)pData;

		fprintf(stderr, "%s: SIMCONNECT_RECV_ID_CLIENT_DATA %d", MODULE_NAME, recv_data->dwRequestID);
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
	fprintf(stdout, "%s: Initialize module...\n", MODULE_NAME);

	client.open();
	client.initDataArea(subscribeLvarArea);
	client.listen(subscribeLvarArea);
	client.dispatch(on_request_received);

	fprintf(stdout, "%s: Module call dispatch...\n", MODULE_NAME);

}

/// <summary>
/// Déinitialisation du module WASM, déconnexion à sim connect
/// </summary>
/// <param name=""></param>
/// <returns></returns>
extern "C" MSFS_CALLBACK void module_deinit(void)
{
	fprintf(stdout, "%s: De-initializing", MODULE_NAME);

	client.close();

	fprintf(stderr, "%s: De-initialization completed", MODULE_NAME);
}