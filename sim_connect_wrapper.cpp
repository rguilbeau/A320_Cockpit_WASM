#include "sim_connect_wrapper.h"

/// <summary>
/// Création du wrapper SimConnect
/// </summary>
/// <param name="sName"></param>
SimConnectWrapper::SimConnectWrapper(const char* sName)
{
	m_sName = sName;
}

/// <summary>
/// Ouverture de la connexion SimConnect
/// </summary>
void SimConnectWrapper::open()
{
	fprintf(stdout, "%s: Open sim connect...\n", m_sName);

	HRESULT result;

	// Ouverture de la connexion
	result = SimConnect_Open(&m_simConnect, m_sName, (HWND)NULL, 0, 0, 0);
	if (FAILED(result))
	{
		fprintf(stderr, "%s: SimConnect_Open failed\n", m_sName);
		return;
	}
	
	// Création d'une priorité haute pour le module WASM
	result = SimConnect_SetNotificationGroupPriority(m_simConnect, (SIMCONNECT_NOTIFICATION_GROUP_ID)0, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
	if (FAILED(result))
	{
		fprintf(stderr, "%s: SimConnect_SetNotificationGroupPriority failed\n", m_sName);
		return;
	}

	// Appel du dispatch SimConnect (c'est cette méthode qui va appeler le callback lorsque cela est nécessaire)
	result = SimConnect_CallDispatch(m_simConnect, SimConnectWrapper::nativeDispatchSimconnectEvent, this);
	if (FAILED(result))
	{
		fprintf(stderr, "%s: Module call dispatch failed.\n", m_sName);
		return;
	}

	// Souscription à l'évenement "on each frame"
	SimConnect_SubscribeToSystemEvent(m_simConnect, (SIMCONNECT_CLIENT_EVENT_ID)10, "Frame");
}

/// <summary>
/// Fermeture de la connection SimConnect
/// </summary>
void SimConnectWrapper::close()
{
	fprintf(stdout, "%s: Close sim connect\n", m_sName);

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
/// Ajout l'implémentation de l'interface pour les callbacks
/// </summary>
/// <param name="pHandler"></param>
void SimConnectWrapper::setHandler(SimConnectEventHandler* pHandler)
{
	m_pHandler = pHandler;
}

/// <summary>
/// Création d'un éspace d'échange SimConnect
/// </summary>
/// <param name="dataArea"></param>
void SimConnectWrapper::initDataArea(const s_dataArea& dataArea)
{
	fprintf(stdout, "%s: Init data area: %s\n", m_sName, dataArea.name);

	HRESULT hr;

	hr = SimConnect_MapClientDataNameToID(m_simConnect, dataArea.name, dataArea.data_id);
	if (FAILED(hr))
	{
		fprintf(stderr, "%s: SimConnect_MapClientDataNameToID failed\n", m_sName);
		return;
	}

	hr = SimConnect_AddToClientDataDefinition(m_simConnect, dataArea.definition_id, 0, dataArea.size);
	if (FAILED(hr))
	{
		fprintf(stderr, "%s: SimConnect_AddToClientDataDefinition failed\n", m_sName);
		return;
	}
}

/// <summary>
/// Demande de surveillance d'un éspace d'échange SimConnect
/// Dès qu'une valeur change dans cet éspace, la callback SimConnectEventHandler::onDataRecevied sera appelée
/// </summary>
/// <param name="dataArea"></param>
void SimConnectWrapper::listen(const s_dataArea& dataArea)
{
	fprintf(stdout, "%s: Listen data area, name:%s\n", m_sName, dataArea.name);

	HRESULT result = SimConnect_RequestClientData(
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
/// Ecriture dans un éspace d'échange SimConnect
/// </summary>
/// <param name="dataArea"></param>
/// <param name="pData"></param>
void SimConnectWrapper::write(const s_dataArea& dataArea, void* pData)
{
	HRESULT result = SimConnect_SetClientData(
		m_simConnect,
		dataArea.data_id,
		dataArea.definition_id,
		SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT,
		0,
		dataArea.size,
		pData
	);

	if (FAILED(result))
	{
		fprintf(stderr, "%s: SimConnect_SetClientData failed\n", m_sName);
		return;
	}
}

/// <summary>
/// SimConnect Callback, appel les méthodes du handler correspondant au type d'évennement  
/// </summary>
/// <param name="pData"></param>
/// <param name="cbData"></param>
void SimConnectWrapper::onSimconnectEvent(SIMCONNECT_RECV* pData, DWORD cbData)
{
	if (m_pHandler != nullptr)
	{
		if (pData->dwID == SIMCONNECT_RECV_ID_EVENT_FRAME)
		{
			// Simconnect FRAME event
			m_pHandler->onFrameEvent();
		}
		else if (pData->dwID == SIMCONNECT_RECV_ID_CLIENT_DATA)
		{
			// Data area data event
			SIMCONNECT_RECV_CLIENT_DATA* pReceivedData = (SIMCONNECT_RECV_CLIENT_DATA*)pData;
			m_pHandler->onDataRecevied(pReceivedData);
		}
	}
}
