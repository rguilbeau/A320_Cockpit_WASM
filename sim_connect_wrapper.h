#pragma once

#include <MSFS/MSFS.h>
#include <MSFS/MSFS_WindowsTypes.h>
#include <MSFS/Legacy/gauges.h>
#include <SimConnect.h>
#include "sim_connect_event_handler.h"
#include "sim_connect_definition.h"

class SimConnectWrapper
{
public:
	explicit SimConnectWrapper(const char* sName);
	virtual ~SimConnectWrapper() = default;

	void open();
	void close();

	void setHandler(SimConnectEventHandler* pHandler);

	void initDataArea(const s_dataArea& dataArea);
	
	void listen(const s_dataArea& dataArea);
	void write(const s_dataArea& dataArea, void* pData);
	
private:
	const char* m_sName;
	HANDLE m_simConnect;

	SimConnectEventHandler *m_pHandler = nullptr;
	
	// Simconnect CALLBACK must be static, call the instanciated class
	void onSimconnectEvent(SIMCONNECT_RECV* pData, DWORD cbData);
	static void CALLBACK nativeDispatchSimconnectEvent(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
	{
		SimConnectWrapper* instance = static_cast<SimConnectWrapper*>(pContext);
		if (instance)
		{
			instance->onSimconnectEvent(pData, cbData);
		}
	}
};

