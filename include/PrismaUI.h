#pragma once

// PrismaUI web-UI integration (soft dependency: the plugin works without PrismaUI installed).
// The view lives in <mod>/PrismaUI/views/<PRODUCT_NAME>/index.html and is copied there by
// the build; the HTML talks back through the JS listeners registered in PrismaUI.cpp.
namespace Prisma
{
	bool  Init();                                        // create the view; false when PrismaUI is absent
	bool  Toggle();                                      // focus + show / unfocus + hide
	bool  IsAvailable();
	void  SendToView(const char* a_jsCode);              // run JS inside the view
	void  Interop(const char* a_function, const char* a_argument); // call window.<function>(argument)

	// Options round-trip: the view sends "name=value" (window.setOption), the plugin stores it in
	// the INI and pushes the whole set back so the UI can correct itself (window.applyOptions).
	void  SyncOptions();
}