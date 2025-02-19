#pragma once

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

class SimconnectClient
{
public:
	struct s_dataArea
	{
		const char* name;
		SIMCONNECT_CLIENT_DATA_ID data_id;
		SIMCONNECT_CLIENT_DATA_DEFINITION_ID definition_id;
		SIMCONNECT_DATA_REQUEST_ID request_id;
		DWORD size;
	};

	explicit SimconnectClient(const char* sName);
	virtual ~SimconnectClient() = default;

	void open();
	void close();
	void dispatch(DispatchProc pfcnDispatch);

	void initDataArea(const s_dataArea& dataArea);
	void listen(const s_dataArea& dataArea);
	void write(const s_dataArea& dataArea, void* data);

private:
	HANDLE m_simConnect;
	const char* m_sName;
};

