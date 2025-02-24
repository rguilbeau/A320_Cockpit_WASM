#pragma once
#include "sim_connect_event_handler.h"
#include "read_iterator.h"
#include "sim_connect_wrapper.h"


class Loop : public SimConnectEventHandler
{
public:
	explicit Loop(uint16_t nReadPerFrameLimit, uint16_t nBatchReadLimit, const char *sName);
	virtual ~Loop() = default;

	void start();
	void stop();

private:
	struct s_lvar
	{
		ID msfsId;
		int externalId;
		char name[100];
	};

	const char* m_sName;
	SimConnectWrapper m_simConnect;
	uint16_t nReadPerFrameLimit;

	s_dataArea m_subscribeLvarArea;
	s_dataArea m_bachLvarArea;
	s_dataArea m_dataLvarArea;

	std::vector<s_lvar> m_batchLvar;
	std::vector<s_dataLvar> m_lvarsData;

	void onFrameEvent() override;
	void onDataRecevied(SIMCONNECT_RECV_CLIENT_DATA* pData) override;

	ReadIterator<s_lvar> m_readIterator;
};