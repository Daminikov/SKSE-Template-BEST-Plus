#pragma once

// Translations for everything the plugin shows.
//
// Files:  <repo>/translations/<language>.txt   (UTF-16 LE with BOM, CRLF, lines "$Key<TAB>text")
// Deployed: <mod>/Interface/Translations/<PRODUCT_NAME>_<language>.txt
//   - this is Skyrim's own interface-translation location, the same one ApocryphaRealm Menu
//     Framework uses for its own strings, and the one the framework scans to build its font
//     atlas: characters from every <anything>_<language>.txt land in the atlas for that
//     language, so a mod shipping kana/hangul/hanzi/Cyrillic needs no font work of its own.
//
// The active language is the one the player picked for the framework (AMF_GetLanguage());
// without a framework the loader falls back to "english".
namespace Loc
{
	void Init();                          // pick the language, load its file (+ english as fallback)
	bool Refresh();                       // re-read when the language changed; true when reloaded
	const char* Get(const char* a_key);   // translated text, the key itself when nothing is found
	const char* Language();
	const char* Json();                   // {"$Key":"text", ...} - what the PrismaUI view gets
}