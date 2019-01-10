/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

/*

    Exports of some glad utilities that we want to use ourselves for *our*
    OpenGL initialization.

*/

#ifdef __cplusplus
extern "C" {
#endif

GLAPI int open_gl(void);
GLAPI void close_gl(void);
GLAPI void* get_proc(const char *namez);
GLAPI int get_exts(void);
GLAPI int has_ext(const char *ext);
GLAPI void free_exts(void);

#ifdef __cplusplus
}
#endif
