
#ifndef PYEA_HEADER_EMBEDDED_
#define PYEA_HEADER_EMBEDDED_

/* The EMBEDDED macro is magic as it is scanned for by the buildsystem (in this header file only)
 * and triggers the generation of a variable definition in the generted "embedded.c" source file.
 * It sould be used as a prefix to a variable declaration, and given a file path (starting after
 * the `src` directory). The generated definition will give the content of the file to the defined
 * variable (see "embedded.c" after building to see the results). The macro aslo takes an escape
 * mode parameter, it sould be one of the macros defined below and specify how the file content
 * is turned into a C literal (and for the case of SIZE it does not even embed content but
 * it initialize the variable to the size (in bytes) of the file).
 * The type of the declared variable should be chosen to work with the generated literal.
 * The `buildsystem/embed.py` script is responsible for the handling of the embedding. */
#define EMBEDDED(filename_, escape_mode_) extern
#define TEXT /* Escapes the file content as a string literal. */
#define BINARY /* Escapes the file content as an array of bytes. */
#define SIZE /* Just produces the size in bytes of the file content, as an integer literal. */

EMBEDDED("../assets/test", TEXT) char const g_asset_test[];

EMBEDDED("../assets/sprite_sheet.png", BINARY) unsigned char const g_asset_sprite_sheet_png[];
EMBEDDED("../assets/sprite_sheet.png", SIZE) unsigned int const g_asset_sprite_sheet_png_size;

#endif /* PYEA_HEADER_EMBEDDED_ */
