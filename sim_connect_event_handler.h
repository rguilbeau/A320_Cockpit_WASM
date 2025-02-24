#pragma once

#include <MSFS/MSFS_WindowsTypes.h>
#include <MSFS/Legacy/gauges.h>
#include <SimConnect.h>

/// <summary>
/// Interface pour les callbacks du wrapper SimConnect
/// </summary>
class SimConnectEventHandler
{
public:
	explicit SimConnectEventHandler() = default;
	virtual ~SimConnectEventHandler() = default;
	
	/// <summary>
	/// Callback appel� � chaque frame du jeu
	/// </summary>
	virtual void onFrameEvent() = 0;

	/// <summary>
	/// Callback appel� lorsque qu'un donn�e change dans un espace d'�change en mode "listen"
	/// </summary>
	/// <param name="pData"></param>
	virtual void onDataRecevied(SIMCONNECT_RECV_CLIENT_DATA* pData) = 0;
};