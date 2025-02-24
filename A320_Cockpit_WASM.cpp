// WASM_HABI.cpp
#include "A320_Cockpit_WASM.h"

#include <MSFS/MSFS.h>
#include <MSFS/MSFS_WindowsTypes.h>
#include <MSFS/Legacy/gauges.h>
#include <SimConnect.h>

#include <stdio.h>
#include "loop.h"

const char* MODULE_NAME = "[A320_Cockpit]";

Loop loop(50, 30, MODULE_NAME);

/// <summary>
/// Intialisation du module WASM
/// </summary>
/// <param name=""></param>
/// <returns></returns>
extern "C" MSFS_CALLBACK void module_init(void)
{
	fprintf(stdout, "%s: Initialize module...\n", MODULE_NAME);

	loop.start();

	fprintf(stdout, "%s: Module call dispatch...\n", MODULE_NAME);

}

/// <summary>
/// Fermeture du module WASM
/// </summary>
/// <param name=""></param>
/// <returns></returns>
extern "C" MSFS_CALLBACK void module_deinit(void)
{
	fprintf(stdout, "%s: De-initializing", MODULE_NAME);

	loop.stop();

	fprintf(stderr, "%s: De-initialization completed", MODULE_NAME);
}