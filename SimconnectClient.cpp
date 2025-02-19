
#include "SimconnectClient.h"

/// <summary>
/// Create Simconnect Wrapper
/// </summary>
/// <param name="sName"></param>
SimconnectClient::SimconnectClient(const char* sName)
{
	m_sName = sName;
}

/// <summary>
/// Open connection
/// </summary>
void SimconnectClient::open()
{
	fprintf(stdout, "%s: Open client...\n", m_sName);

	HRESULT result;

	result = SimConnect_Open(&m_simConnect, m_sName, (HWND)NULL, 0, 0, 0);
	if (FAILED(result))
	{
		fprintf(stderr, "%s: SimConnect_Open failed\n", m_sName);
		return;
	}

	result = SimConnect_SetNotificationGroupPriority(m_simConnect, (SIMCONNECT_NOTIFICATION_GROUP_ID)0, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
	if (FAILED(result))
	{
		fprintf(stderr, "%s: SimConnect_SetNotificationGroupPriority failed\n", m_sName);
		return;
	}
}

/// <summary>
/// Close Simconnect connection
/// </summary>
void SimconnectClient::close()
{
	fprintf(stdout, "%s: Close client\n", m_sName);

	if (!m_simConnect)
	{
		fprintf(stderr, "%s: SimConnect handle was not valid.\n", m_sName);
		return;
	}

	HRESULT result = SimConnect_Close(m_simConnect);
	if (FAILED(result))
	{
		fprintf(stderr, "%s: SimConnect_Close failed.\n", m_sName);
		return;
	}
}

/// <summary>
/// Run Dispatch callback
/// </summary>
/// <param name="callback"></param>
void SimconnectClient::dispatch(DispatchProc callback)
{
	HRESULT result = SimConnect_CallDispatch(m_simConnect, callback, NULL);
	if (FAILED(result))
	{
		fprintf(stderr, "%s: Module call dispatch failed.\n", m_sName);
		return;
	}
}

/// <summary>
/// Initialize data exchange area
/// </summary>
/// <param name="dataArea"></param>
void SimconnectClient::initDataArea(const s_dataArea& dataArea)
{
	fprintf(stdout, "%s: Init data area (name=%s, data_id=%u, definition_id=%u, request_id=%u)\n", m_sName, dataArea.data_id, dataArea.definition_id, dataArea.request_id);

	HRESULT hr;

	// Étape 1: Mapper le même espace client
	hr = SimConnect_MapClientDataNameToID(m_simConnect, dataArea.name, dataArea.data_id);
	if (FAILED(hr)) 
	{
		fprintf(stderr, "%s: SimConnect_MapClientDataNameToID failed\n", m_sName);
		return;
	}

	// Étape 2: Définir comment les données seront reçues
	hr = SimConnect_AddToClientDataDefinition(m_simConnect, dataArea.definition_id, 0, dataArea.size);
	if (FAILED(hr)) 
	{
		fprintf(stderr, "%s: SimConnect_AddToClientDataDefinition failed\n", m_sName);
		return;
	}
}

/// <summary>
/// Listen data exchange area.
/// Dispatch callback is called when a variable change on the data area
/// </summary>
/// <param name="dataArea"></param>
void SimconnectClient::listen(const s_dataArea& dataArea)
{
	fprintf(stdout, "%s: Listen data area (name=%s, data_id=%u, definition_id=%u, request_id=%u)\n", m_sName, dataArea.data_id, dataArea.definition_id, dataArea.request_id);


	HRESULT result;

	result = SimConnect_RequestClientData(
		m_simConnect, 
		dataArea.data_id, 
		dataArea.request_id, 
		dataArea.definition_id,
		SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET,
		SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT
	);

	if (FAILED(result))
	{
		fprintf(stderr, "%s: SimConnect_RequestClientData failed\n", m_sName);
		return;
	}
}

/// <summary>
/// Write value inside the data area
/// </summary>
/// <param name="dataArea"></param>
/// <param name="data"></param>
void SimconnectClient::write(const s_dataArea& dataArea, void* data)
{
	HRESULT result = SimConnect_SetClientData(
		m_simConnect,
		dataArea.data_id,
		dataArea.definition_id,
		SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT,
		0,
		dataArea.size,
		data
	);

	if (result != S_OK)
	{
		fprintf(stderr, "%s: SimConnect_SetClientData failed\n", m_sName);
		return;
	}
}
