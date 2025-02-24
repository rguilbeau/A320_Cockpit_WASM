#include "loop.h"

/// <summary>
/// Création de la loop principale du module WASM
/// </summary>
/// <param name="nReadPerFrameLimit"></param>
/// <param name="nBatchReadLimit"></param>
/// <param name="sName"></param>
Loop::Loop(uint16_t nReadPerFrameLimit, uint16_t nBatchReadLimit, const char *sName): 
	m_readIterator{ nReadPerFrameLimit, nBatchReadLimit},
	m_simConnect{sName}
{
	m_sName = sName;

	// Création de l'espace d'échange pour la souscription des Lvars
	m_subscribeLvarArea.name = "A320_Cockpit.SUBSCRIBE_LVAR";
	m_subscribeLvarArea.data_id = (SIMCONNECT_CLIENT_DATA_ID)0;
	m_subscribeLvarArea.definition_id = (SIMCONNECT_CLIENT_DATA_DEFINITION_ID)0;
	m_subscribeLvarArea.request_id = (SIMCONNECT_DATA_REQUEST_ID)0;
	m_subscribeLvarArea.size = sizeof(s_subsrcibeLvar);

	// Création de l'espace d'échange pour les lvars "batch" (lecture prioritaire)
	m_bachLvarArea.name = "A320_Cockpit.BATCH_LVAR";
	m_bachLvarArea.data_id = (SIMCONNECT_CLIENT_DATA_ID)1;
	m_bachLvarArea.definition_id = (SIMCONNECT_CLIENT_DATA_DEFINITION_ID)1;
	m_bachLvarArea.request_id = (SIMCONNECT_DATA_REQUEST_ID)1;
	m_bachLvarArea.size = sizeof(s_subsrcibeLvar) * nReadPerFrameLimit;

	// Création de l'éspace d'échange pour l'envoi des valeurs des lvars
	m_dataLvarArea.name = "A320_Cockpit.DATA_LVAR";
	m_dataLvarArea.data_id = (SIMCONNECT_CLIENT_DATA_ID)2;
	m_dataLvarArea.definition_id = (SIMCONNECT_CLIENT_DATA_DEFINITION_ID)2;
	m_dataLvarArea.request_id = (SIMCONNECT_DATA_REQUEST_ID)2;
	m_dataLvarArea.size = sizeof(s_dataLvar) * nReadPerFrameLimit;

	// Ajout du handler (SimConnectEventHandler)
	m_simConnect.setHandler(this);
}

/// <summary>
/// Démarre la boucle principale du module WASM
/// </summary>
void Loop::start()
{
	// Connexion à SimConnnect
	m_simConnect.open();

	// Création des éspace d'échange
	m_simConnect.initDataArea(m_subscribeLvarArea);
	m_simConnect.initDataArea(m_bachLvarArea);
	m_simConnect.initDataArea(m_dataLvarArea);

	// Mise en écoute des éspace d'échange
	m_simConnect.listen(m_subscribeLvarArea);
	m_simConnect.listen(m_bachLvarArea);
}

/// <summary>
/// Ferme la connection à simconnect
/// </summary>
void Loop::stop()
{
	m_simConnect.close();
}

/// <summary>
/// Appelé par le wrapper simconnect à chaque frame du jeu
/// Lecture des variables de l'avion
/// </summary>
void Loop::onFrameEvent()
{
	// Nettoyage des valeurs précédentes
	m_lvarsData.clear();

	// Lecture de toutes les variables
	for (s_lvar &lvar : m_readIterator)
	{
		if (lvar.msfsId == -1)
		{
			// Cette Lvar n'a jamais été lue, on récupère son ID MSFS
			lvar.msfsId = check_named_variable(lvar.name);
		}

		if (lvar.msfsId != -1)
		{
			// Cette variable existe, on peut la lire
			s_dataLvar data;
			data.externalId = lvar.externalId;
			data.value = get_named_variable_value(lvar.msfsId);

			// On sauvegarde sa valeur pour l'envoyer ensuite
			m_lvarsData.push_back(data);
		}
	}

	// Envoi des valeurs des Lvars
	if (m_lvarsData.size() > 0)
	{
		m_simConnect.write(m_dataLvarArea, m_lvarsData.data());
	}
}

/// <summary>
/// Appelé par le wrapper SimConnect lorsque une donnée à change dans un espace d'échange (listen)
/// </summary>
/// <param name="pData"></param>
void Loop::onDataRecevied(SIMCONNECT_RECV_CLIENT_DATA* pData)
{
	if (pData->dwRequestID == m_subscribeLvarArea.request_id)
	{
		// Demande souscription d'une Lvar
		s_subsrcibeLvar* data = (s_subsrcibeLvar*)&pData->dwData;
		s_lvar lvar;
		lvar.msfsId = -1;
		lvar.externalId = data->externalId;
		strcpy(lvar.name, data->name);

		// On l'ajoute à l'itérateur de variable
		m_readIterator.push_back(lvar);

		fprintf(stdout, "%s: Subscribe lvar: %s\n", m_sName, lvar.name);
	}
	else if (pData->dwRequestID == m_bachLvarArea.request_id)
	{
		// Demande de lecture "batch" (lecture prioritaire)
		s_subsrcibeLvar* data = (s_subsrcibeLvar*)&pData->dwData;
		
		std::vector<s_lvar> batch;
		for (int nIndex = 0; nIndex < nReadPerFrameLimit; nIndex++)
		{
			s_lvar lvar;
			lvar.msfsId = -1;
			lvar.externalId = data[nIndex].externalId;
			strcpy(lvar.name, data[nIndex].name);

			batch.push_back(lvar);

			fprintf(stdout, "%s: Batch lvar: \n", m_sName, lvar.name);
		}

		// Envoi du batch à l'itérateur de variable
		m_readIterator.setBatch(batch);
	}
}
